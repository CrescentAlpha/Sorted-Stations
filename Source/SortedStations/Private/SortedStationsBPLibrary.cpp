#include "SortedStationsBPLibrary.h"
#include "SortedStations.h"
#include "FGActorRepresentation.h"
#include "FGActorRepresentationManager.h"
#include "WheeledVehicles/FGWheeledVehicleIdentifier.h"
#include "WheeledVehicles/FGWheeledVehicle.h"
#include "FGTrainStationIdentifier.h"
#include "Buildables/FGBuildableRailroadStation.h"
#include "WheeledVehicles/FGDockingStationIdentifier.h"
#include "Buildables/FGBuildableDockingStation.h"
#include "FGTrain.h"
#include "FGRailroadVehicle.h"
#include "FGLocomotive.h"
#include "FGDroneVehicle.h"
#include "Buildables/FGBuildableDroneStation.h"
#include "FGDroneStationInfo.h"
#include "Buildables/FGBuildable.h"
#include "FGVehicle.h"
#include "FGActorRepresentationInterface.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Dom/JsonObject.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#define LOCTEXT_NAMESPACE "SortedStations"

namespace
{
	FString GetTagsFilePath()
	{
		return FPaths::ProjectSavedDir() / TEXT("Config/SortedStations_CustomTags.json");
	}

	FString GetColorsFilePath()
	{
		return FPaths::ProjectSavedDir() / TEXT("Config/SortedStations_VehicleColors.json");
	}

	FString GetVehicleIdentifierKey(const UFGActorRepresentation* Representation)
	{
		if (!Representation)
		{
			return FString();
		}
		AActor* RealActor = Representation->GetRealActor();
		if (!RealActor)
		{
			return Representation->GetRepresentationText().ToString();
		}
		// 車両
		if (AFGWheeledVehicleIdentifier* Identifier = Cast<AFGWheeledVehicleIdentifier>(RealActor))
		{
			return Identifier->GetName();
		}
		if (AFGWheeledVehicle* Vehicle = Cast<AFGWheeledVehicle>(RealActor))
		{
			if (AFGWheeledVehicleIdentifier* Identifier = Vehicle->GetVehicleIdentifier())
			{
				return Identifier->GetName();
			}
			return Vehicle->GetName();
		}
		// 駅
		if (AFGTrainStationIdentifier* StationId = Cast<AFGTrainStationIdentifier>(RealActor))
		{
			return StationId->GetName();
		}
		if (AFGBuildableRailroadStation* Station = Cast<AFGBuildableRailroadStation>(RealActor))
		{
			if (AFGTrainStationIdentifier* StationId = Station->GetStationIdentifier())
			{
				return StationId->GetName();
			}
			return Station->GetName();
		}
		// トラックステーション
		if (AFGDockingStationIdentifier* DockId = Cast<AFGDockingStationIdentifier>(RealActor))
		{
			return DockId->GetName();
		}
		if (AFGBuildableDockingStation* Dock = Cast<AFGBuildableDockingStation>(RealActor))
		{
			if (AFGDockingStationIdentifier* DockId = Dock->GetStationIdentifier())
			{
				return DockId->GetName();
			}
			return Dock->GetName();
		}
		// ドローンポート
		if (AFGDroneStationInfo* Info = Cast<AFGDroneStationInfo>(RealActor))
		{
			return Info->GetName();
		}
		if (AFGBuildableDroneStation* DroneStation = Cast<AFGBuildableDroneStation>(RealActor))
		{
			if (AFGDroneStationInfo* Info = DroneStation->GetInfo())
			{
				return Info->GetName();
			}
			return DroneStation->GetName();
		}
		// 列車 (AFGRailroadVehicle の場合は親の Train をキーにする)
		if (AFGRailroadVehicle* RailVeh = Cast<AFGRailroadVehicle>(RealActor))
		{
			if (AFGTrain* Train = RailVeh->GetTrain())
			{
				return Train->GetName();
			}
		}
		// 列車・ドローン・その他
		return RealActor->GetName();
	}

	static TMap<FString, FLinearColor> g_VehicleColorsCache;
	static bool g_bVehicleColorsCacheLoaded = false;

	static TArray<FString> g_TagsCache;
	static bool g_bTagsCacheLoaded = false;

	TMap<FString, FLinearColor> LoadVehicleColorsFromDisk()
	{
		TMap<FString, FLinearColor> ColorMap;
		const FString FilePath = GetColorsFilePath();

		FString JsonString;
		if (FFileHelper::LoadFileToString(JsonString, *FilePath))
		{
			TSharedPtr<FJsonObject> JsonObject;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
			if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
			{
				for (const auto& Pair : JsonObject->Values)
				{
					if (Pair.Value.IsValid())
					{
						FLinearColor Color;
						if (Color.InitFromString(Pair.Value->AsString()))
						{
							ColorMap.Add(Pair.Key, Color);
						}
						else
						{
							Color = FLinearColor(FColor::FromHex(Pair.Value->AsString()));
							ColorMap.Add(Pair.Key, Color);
						}
					}
				}
			}
		}
		return ColorMap;
	}

	TMap<FString, FLinearColor>& GetVehicleColorsCache()
	{
		if (!g_bVehicleColorsCacheLoaded)
		{
			g_VehicleColorsCache = LoadVehicleColorsFromDisk();
			g_bVehicleColorsCacheLoaded = true;
		}
		return g_VehicleColorsCache;
	}

	void SaveVehicleColorsInternal(const TMap<FString, FLinearColor>& ColorMap)
	{
		g_VehicleColorsCache = ColorMap;
		g_bVehicleColorsCacheLoaded = true;

		TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
		for (const auto& Pair : ColorMap)
		{
			RootObject->SetStringField(Pair.Key, Pair.Value.ToFColor(true).ToHex());
		}

		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		if (FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer))
		{
			FFileHelper::SaveStringToFile(OutputString, *GetColorsFilePath());
		}
	}

	TArray<FString> LoadTagsFromDisk()
	{
		TArray<FString> Tags;
		const FString FilePath = GetTagsFilePath();

		FString JsonString;
		if (FFileHelper::LoadFileToString(JsonString, *FilePath))
		{
			TSharedPtr<FJsonObject> JsonObject;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
			if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
			{
				const TArray<TSharedPtr<FJsonValue>>* JsonTags;
				if (JsonObject->TryGetArrayField(TEXT("tags"), JsonTags))
				{
					for (const TSharedPtr<FJsonValue>& Val : *JsonTags)
					{
						if (Val.IsValid())
						{
							Tags.Add(Val->AsString());
						}
					}
					return Tags;
				}
			}
		}

		// デフォルトタグ一覧（初回時）
		Tags = { TEXT("_IN"), TEXT("_OUT"), TEXT("_To"), TEXT("_From") };
		return Tags;
	}

	TArray<FString>& GetTagsCache()
	{
		if (!g_bTagsCacheLoaded)
		{
			g_TagsCache = LoadTagsFromDisk();
			g_bTagsCacheLoaded = true;
		}
		return g_TagsCache;
	}

	void SaveTagsInternal(const TArray<FString>& Tags)
	{
		g_TagsCache = Tags;
		g_bTagsCacheLoaded = true;

		TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
		TArray<TSharedPtr<FJsonValue>> JsonArray;
		for (const FString& Tag : Tags)
		{
			JsonArray.Add(MakeShared<FJsonValueString>(Tag));
		}
		RootObject->SetArrayField(TEXT("tags"), JsonArray);

		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		if (FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer))
		{
			FFileHelper::SaveStringToFile(OutputString, *GetTagsFilePath());
		}
	}
}

FLinearColor USortedStationsBPLibrary::AdjustColorForContrast(const FLinearColor& InColor)
{
	// アルファがほぼ0（透明/未設定）の場合はそのまま
	if (InColor.A <= 0.01f)
	{
		return InColor;
	}

	// 輝度（Luminance）の計算: 0.299R + 0.587G + 0.114B
	const float Luminance = 0.299f * InColor.R + 0.587f * InColor.G + 0.114f * InColor.B;

	// 明色（Luminance > 0.65）の場合、白シルエットとの同化を防ぐため明度を最適化
	if (Luminance > 0.65f)
	{
		FLinearColor HSV = InColor.LinearRGBToHSV();
		// 白に近い極低彩度の場合（白〜極薄いグレー）は暗いトーンにしてコントラスト確保
		if (HSV.G < 0.20f)
		{
			HSV.B = FMath::Min(HSV.B, 0.50f);
		}
		else
		{
			// 黄色やパステルカラーの場合は彩度を維持したまま明度を0.72以下にクランプ
			HSV.B = FMath::Min(HSV.B, 0.72f);
		}
		FLinearColor Result = HSV.HSVToLinearRGB();
		Result.A = InColor.A;
		return Result;
	}

	return InColor;
}

void USortedStationsBPLibrary::RequestImmediateMapSort(UObject* WorldContextObject)
{
	FSortedStationsModule::RequestImmediateMapSort(WorldContextObject);
}

bool USortedStationsBPLibrary::IsSupportedRepresentationType(ERepresentationType Type)
{
	return (Type == ERepresentationType::RT_Vehicle ||
	        Type == ERepresentationType::RT_VehicleDockingStation ||
	        Type == ERepresentationType::RT_TrainStation ||
	        Type == ERepresentationType::RT_Train ||
	        Type == ERepresentationType::RT_DronePort ||
	        Type == ERepresentationType::RT_Drone);
}

IFGActorRepresentationInterface* USortedStationsBPLibrary::ResolveRepresentationInterface(AActor* RealActor)
{
	if (!RealActor) return nullptr;

	if (IFGActorRepresentationInterface* Rep = Cast<IFGActorRepresentationInterface>(RealActor))
	{
		return Rep;
	}

	// 駅: 建築物 -> 駅Identifier
	if (AFGBuildableRailroadStation* Station = Cast<AFGBuildableRailroadStation>(RealActor))
	{
		return Cast<IFGActorRepresentationInterface>(Station->GetStationIdentifier());
	}

	// トラックステーション: 建築物 -> ドッキングIdentifier
	if (AFGBuildableDockingStation* Dock = Cast<AFGBuildableDockingStation>(RealActor))
	{
		return Cast<IFGActorRepresentationInterface>(Dock->GetStationIdentifier());
	}

	// ドローンポート: 情報オブジェクト -> ドローン港建築物
	if (AFGDroneStationInfo* Info = Cast<AFGDroneStationInfo>(RealActor))
	{
		return Cast<IFGActorRepresentationInterface>(Info->GetStation());
	}
	if (AFGBuildableDroneStation* DroneStation = Cast<AFGBuildableDroneStation>(RealActor))
	{
		return Cast<IFGActorRepresentationInterface>(DroneStation);
	}

	// 車両: 車両本体 -> 車両Identifier
	if (AFGWheeledVehicle* Vehicle = Cast<AFGWheeledVehicle>(RealActor))
	{
		return Cast<IFGActorRepresentationInterface>(Vehicle->GetVehicleIdentifier());
	}

	// 列車: 個別車両 (貨物車/機関車) -> 列車編成
	if (AFGRailroadVehicle* RailVeh = Cast<AFGRailroadVehicle>(RealActor))
	{
		return Cast<IFGActorRepresentationInterface>(RailVeh->GetTrain());
	}

	return nullptr;
}

FText USortedStationsBPLibrary::GetVanillaDisplayName(const UFGActorRepresentation* Representation)
{
	if (!Representation)
	{
		return FText::GetEmpty();
	}


	// 列車 (RT_Train) の場合は個別車両名（貨物車等）ではなく公式の「列車」を表示
	if (Representation->GetRepresentationType() == ERepresentationType::RT_Train)
	{
		return GetVanillaString(TEXT("Transportation_UI"), TEXT("Trains/RepresentationText"), FText::FromString(TEXT("Train")));
	}

	AActor* RealActor = Representation->GetRealActor();
	if (RealActor)
	{
		// 1. 車両 (トラクター、トラック、サイバートラック、エクスプローラー等)
		if (AFGWheeledVehicle* Vehicle = Cast<AFGWheeledVehicle>(RealActor))
		{
			if (!Vehicle->mDisplayName.IsEmpty()) return Vehicle->mDisplayName;
		}
		if (AFGWheeledVehicleIdentifier* VehicleId = Cast<AFGWheeledVehicleIdentifier>(RealActor))
		{
			if (AFGWheeledVehicle* Vehicle = VehicleId->GetOwnerVehicle())
			{
				if (!Vehicle->mDisplayName.IsEmpty()) return Vehicle->mDisplayName;
			}
			else if (VehicleId->GetVehicleClass())
			{
				if (AFGWheeledVehicle* CDO = VehicleId->GetVehicleClass()->GetDefaultObject<AFGWheeledVehicle>())
				{
					if (!CDO->mDisplayName.IsEmpty()) return CDO->mDisplayName;
				}
			}
		}

		// 2. 建築物（電車の駅、トラックステーション、ドローン港等）
		if (AFGBuildable* Buildable = Cast<AFGBuildable>(RealActor))
		{
			if (!Buildable->mDisplayName.IsEmpty()) return Buildable->mDisplayName;
		}
		if (AFGTrainStationIdentifier* StationId = Cast<AFGTrainStationIdentifier>(RealActor))
		{
			if (AFGBuildableRailroadStation* Station = StationId->GetStation())
			{
				if (!Station->mDisplayName.IsEmpty()) return Station->mDisplayName;
			}
		}
		if (AFGDockingStationIdentifier* DockId = Cast<AFGDockingStationIdentifier>(RealActor))
		{
			if (AFGBuildableDockingStation* Dock = DockId->GetStation())
			{
				if (!Dock->mDisplayName.IsEmpty()) return Dock->mDisplayName;
			}
		}

		// 3. ドローン
		if (AFGDroneVehicle* Drone = Cast<AFGDroneVehicle>(RealActor))
		{
			if (!Drone->mDisplayName.IsEmpty()) return Drone->mDisplayName;
		}

		// 4. 列車 (AFGTrain または AFGRailroadVehicle)
		AFGTrain* TargetTrain = Cast<AFGTrain>(RealActor);
		if (!TargetTrain)
		{
			if (AFGRailroadVehicle* RailVeh = Cast<AFGRailroadVehicle>(RealActor))
			{
				TargetTrain = RailVeh->GetTrain();
			}
		}
		if (TargetTrain)
		{
			// 機関車の公式表示名（電気機関車）を最優先で取得
			if (TargetTrain->mMultipleUnitMaster && !TargetTrain->mMultipleUnitMaster->mDisplayName.IsEmpty())
			{
				return TargetTrain->mMultipleUnitMaster->mDisplayName;
			}
			if (TargetTrain->FirstVehicle && !TargetTrain->FirstVehicle->mDisplayName.IsEmpty())
			{
				return TargetTrain->FirstVehicle->mDisplayName;
			}
		}

		// 5. 汎用 AFGVehicle
		if (AFGVehicle* GenVehicle = Cast<AFGVehicle>(RealActor))
		{
			if (!GenVehicle->mDisplayName.IsEmpty()) return GenVehicle->mDisplayName;
		}
	}

	// 6. フォールバック: RepresentationType の公式DisplayName
	UEnum* EnumPtr = StaticEnum<ERepresentationType>();
	if (EnumPtr)
	{
		return EnumPtr->GetDisplayNameTextByValue((int64)Representation->GetRepresentationType());
	}

	return FText::GetEmpty();
}

FText USortedStationsBPLibrary::GetRawActorName(const UFGActorRepresentation* Representation)
{
	if (!Representation)
	{
		return FText::GetEmpty();
	}

	AActor* RealActor = Representation->GetRealActor();
	if (RealActor)
	{
		// 1. 駅 (生の名前を取得して "駅: " の二重付加を防ぐ)
		if (AFGTrainStationIdentifier* StationId = Cast<AFGTrainStationIdentifier>(RealActor))
		{
			return StationId->GetStationName();
		}
		if (AFGBuildableRailroadStation* Station = Cast<AFGBuildableRailroadStation>(RealActor))
		{
			if (AFGTrainStationIdentifier* StationId = Station->GetStationIdentifier())
			{
				return StationId->GetStationName();
			}
		}

		// 2. トラックステーション
		if (AFGDockingStationIdentifier* DockId = Cast<AFGDockingStationIdentifier>(RealActor))
		{
			return DockId->GetStationName();
		}
		if (AFGBuildableDockingStation* Dock = Cast<AFGBuildableDockingStation>(RealActor))
		{
			if (AFGDockingStationIdentifier* DockId = Dock->GetStationIdentifier())
			{
				return DockId->GetStationName();
			}
		}

		// 3. 車両
		if (AFGWheeledVehicleIdentifier* VehicleId = Cast<AFGWheeledVehicleIdentifier>(RealActor))
		{
			return VehicleId->GetVehicleName();
		}
		if (AFGWheeledVehicle* Vehicle = Cast<AFGWheeledVehicle>(RealActor))
		{
			if (AFGWheeledVehicleIdentifier* Identifier = Vehicle->GetVehicleIdentifier())
			{
				return Identifier->GetVehicleName();
			}
		}

		// 4. 列車
		AFGTrain* Train = Cast<AFGTrain>(RealActor);
		if (!Train)
		{
			if (AFGRailroadVehicle* RailVeh = Cast<AFGRailroadVehicle>(RealActor))
			{
				Train = RailVeh->GetTrain();
			}
		}
		if (Train)
		{
			return Train->GetTrainName();
		}

		// 5. ドローンポート (BuildingTagから生の名前を取得)
		if (AFGBuildableDroneStation* DroneStation = Cast<AFGBuildableDroneStation>(RealActor))
		{
			if (AFGDroneStationInfo* Info = DroneStation->GetInfo())
			{
				const FString Tag = Info->GetBuildingTag_Implementation();
				if (!Tag.IsEmpty())
				{
					return FText::FromString(Tag);
				}
			}
			FString RepStr = Representation->GetRepresentationText().ToString();
			TArray<FString> Prefixes;
			if (!DroneStation->mDisplayName.IsEmpty())
			{
				Prefixes.Add(DroneStation->mDisplayName.ToString());
			}
			Prefixes.Add(TEXT("Drone Port"));
			Prefixes.Add(TEXT("ドローンポート"));
			Prefixes.Add(TEXT("ドローン港"));
			Prefixes.Add(TEXT("无人机平台"));
			Prefixes.Add(TEXT("無人機平台"));

			for (const FString& Prefix : Prefixes)
			{
				if (RepStr.StartsWith(Prefix, ESearchCase::IgnoreCase))
				{
					RepStr.RightChopInline(Prefix.Len());
					break;
				}
			}
			while (RepStr.StartsWith(TEXT(":")) || RepStr.StartsWith(TEXT("：")) || RepStr.StartsWith(TEXT(" ")))
			{
				RepStr.RightChopInline(1);
			}
			return FText::FromString(RepStr.TrimStartAndEnd());
		}

		// 6. ドローン (アクター表示名から動的にプレフィックス・コロン・空白を除去)
		if (AFGDroneVehicle* Drone = Cast<AFGDroneVehicle>(RealActor))
		{
			FString RawStr = !Drone->mMapText.IsEmpty() ? Drone->mMapText.ToString() : Representation->GetRepresentationText().ToString();
			TArray<FString> Prefixes;
			if (!Drone->mDisplayName.IsEmpty())
			{
				Prefixes.Add(Drone->mDisplayName.ToString());
			}
			Prefixes.Add(TEXT("Drone"));
			Prefixes.Add(TEXT("ドローン"));
			Prefixes.Add(TEXT("无人机"));
			Prefixes.Add(TEXT("無人機"));

			for (const FString& Prefix : Prefixes)
			{
				if (RawStr.StartsWith(Prefix, ESearchCase::IgnoreCase))
				{
					RawStr.RightChopInline(Prefix.Len());
					break;
				}
			}
			while (RawStr.StartsWith(TEXT(":")) || RawStr.StartsWith(TEXT("：")) || RawStr.StartsWith(TEXT(" ")))
			{
				RawStr.RightChopInline(1);
			}
			return FText::FromString(RawStr.TrimStartAndEnd());
		}
	}

	return Representation->GetRepresentationText();
}

bool USortedStationsBPLibrary::RenameVehicle(UObject* WorldContextObject, UFGActorRepresentation* Representation, const FText& NewName)
{
	if (!Representation)
	{
		return false;
	}

	AActor* RealActor = Representation->GetRealActor();

	// 1. 車両 (RT_Vehicle)
	if (AFGWheeledVehicleIdentifier* VehicleIdentifier = Cast<AFGWheeledVehicleIdentifier>(RealActor))
	{
		VehicleIdentifier->SetVehicleName(NewName);
		VehicleIdentifier->UpdateRepresentation_Local();
		RequestImmediateMapSort(WorldContextObject);
		return true;
	}
	if (AFGWheeledVehicle* Vehicle = Cast<AFGWheeledVehicle>(RealActor))
	{
		if (AFGWheeledVehicleIdentifier* Identifier = Vehicle->GetVehicleIdentifier())
		{
			Identifier->SetVehicleName(NewName);
			Identifier->UpdateRepresentation_Local();
			RequestImmediateMapSort(WorldContextObject);
			return true;
		}
	}

	// 2. 駅 (RT_TrainStation): SetStationName のみを呼び、バニラの自動フォーマットに任せる
	if (AFGTrainStationIdentifier* StationId = Cast<AFGTrainStationIdentifier>(RealActor))
	{
		StationId->SetStationName(NewName);
		RequestImmediateMapSort(WorldContextObject);
		return true;
	}
	if (AFGBuildableRailroadStation* Station = Cast<AFGBuildableRailroadStation>(RealActor))
	{
		if (AFGTrainStationIdentifier* StationId = Station->GetStationIdentifier())
		{
			StationId->SetStationName(NewName);
			RequestImmediateMapSort(WorldContextObject);
			return true;
		}
	}

	// 3. トラックステーション (RT_VehicleDockingStation)
	if (AFGDockingStationIdentifier* DockId = Cast<AFGDockingStationIdentifier>(RealActor))
	{
		DockId->SetStationName(NewName);
		RequestImmediateMapSort(WorldContextObject);
		return true;
	}
	if (AFGBuildableDockingStation* Dock = Cast<AFGBuildableDockingStation>(RealActor))
	{
		if (AFGDockingStationIdentifier* DockId = Dock->GetStationIdentifier())
		{
			DockId->SetStationName(NewName);
			RequestImmediateMapSort(WorldContextObject);
			return true;
		}
	}

	// 4. 列車 (RT_Train): AFGTrain または AFGRailroadVehicle から親 Train を取得して更新
	AFGTrain* TargetTrain = Cast<AFGTrain>(RealActor);
	if (!TargetTrain)
	{
		if (AFGRailroadVehicle* RailVeh = Cast<AFGRailroadVehicle>(RealActor))
		{
			TargetTrain = RailVeh->GetTrain();
		}
	}
	if (TargetTrain)
	{
		TargetTrain->SetTrainName(NewName);
		TargetTrain->UpdateRepresentation_Local();
		RequestImmediateMapSort(WorldContextObject);
		return true;
	}

	// 5. ドローンポート (RT_DronePort): BuildingTag を更新し、バニラ多言語接頭語を動的付与
	if (AFGBuildableDroneStation* DroneStation = Cast<AFGBuildableDroneStation>(RealActor))
	{
		if (AFGDroneStationInfo* Info = DroneStation->GetInfo())
		{
			Info->SetBuildingTag_Implementation(NewName.ToString());
		}
		FText DisplayText;
		if (!DroneStation->mDisplayName.IsEmpty() && !NewName.IsEmpty())
		{
			DisplayText = FText::Format(FText::FromString(TEXT("{0}: {1}")), DroneStation->mDisplayName, NewName);
		}
		else
		{
			DisplayText = NewName;
		}
		DroneStation->SetActorRepresentationText(DisplayText);
		Representation->mRepresentationText = DisplayText;
		if (AFGActorRepresentationManager* Manager = AFGActorRepresentationManager::Get(WorldContextObject))
		{
			Manager->mOnActorRepresentationUpdated.Broadcast(Representation);
		}
		RequestImmediateMapSort(WorldContextObject);
		return true;
	}

	// 6. ドローン (RT_Drone): mMapText を更新し、バニラ多言語接頭語を動的付与
	if (AFGDroneVehicle* Drone = Cast<AFGDroneVehicle>(RealActor))
	{
		Drone->mMapText = NewName;
		FText DisplayText;
		if (!Drone->mDisplayName.IsEmpty() && !NewName.IsEmpty())
		{
			DisplayText = FText::Format(FText::FromString(TEXT("{0}: {1}")), Drone->mDisplayName, NewName);
		}
		else
		{
			DisplayText = NewName;
		}
		Drone->SetActorRepresentationText(DisplayText);
		Representation->mRepresentationText = DisplayText;
		if (AFGActorRepresentationManager* Manager = AFGActorRepresentationManager::Get(WorldContextObject))
		{
			Manager->mOnActorRepresentationUpdated.Broadcast(Representation);
		}
		RequestImmediateMapSort(WorldContextObject);
		return true;
	}

	// 7. 汎用フォールバック (IFGActorRepresentationInterface)
	if (IFGActorRepresentationInterface* RepInterface = Cast<IFGActorRepresentationInterface>(RealActor))
	{
		RepInterface->SetActorRepresentationText(NewName);
		RequestImmediateMapSort(WorldContextObject);
		return true;
	}

	return false;
}

bool USortedStationsBPLibrary::SetVehicleColor(UObject* WorldContextObject, UFGActorRepresentation* Representation, FLinearColor NewColor)
{
	if (!Representation)
	{
		return false;
	}

	const FString Key = GetVehicleIdentifierKey(Representation);
	if (Key.IsEmpty())
	{
		return false;
	}

	// 輝度（Luminance）に応じたコントラスト自動補正を適用
	const FLinearColor AdjustedColor = AdjustColorForContrast(NewColor);

	TMap<FString, FLinearColor>& Colors = GetVehicleColorsCache();
	Colors.Add(Key, AdjustedColor);
	SaveVehicleColorsInternal(Colors);

	// Representationプロパティを書き換え
	Representation->mRepresentationColor = AdjustedColor;

	// アクター側が IFGActorRepresentationInterface を実装していればカラーを設定
	if (AActor* RealActor = Representation->GetRealActor())
	{
		if (IFGActorRepresentationInterface* RepInterface = ResolveRepresentationInterface(RealActor))
		{
			RepInterface->SetActorRepresentationColor(AdjustedColor);
		}
	}

	// AFGActorRepresentationManager経由でマップUIを即時更新
	if (AFGActorRepresentationManager* Manager = AFGActorRepresentationManager::Get(WorldContextObject))
	{
		Manager->mOnActorRepresentationUpdated.Broadcast(Representation);
	}

	return true;
}

FLinearColor USortedStationsBPLibrary::GetVehicleColor(UObject* WorldContextObject, UFGActorRepresentation* Representation)
{
	if (!Representation)
	{
		return FLinearColor::White;
	}

	const FString Key = GetVehicleIdentifierKey(Representation);
	if (!Key.IsEmpty())
	{
		const TMap<FString, FLinearColor>& Colors = GetVehicleColorsCache();
		if (const FLinearColor* FoundColor = Colors.Find(Key))
		{
			return *FoundColor;
		}
	}

	return Representation->GetRepresentationColor();
}

bool USortedStationsBPLibrary::TryGetSavedVehicleColor(const UFGActorRepresentation* Representation, FLinearColor& OutColor)
{
	if (!Representation)
	{
		return false;
	}

	const FString Key = GetVehicleIdentifierKey(Representation);
	if (!Key.IsEmpty())
	{
		const TMap<FString, FLinearColor>& Colors = GetVehicleColorsCache();
		if (const FLinearColor* FoundColor = Colors.Find(Key))
		{
			OutColor = *FoundColor;
			return true;
		}
	}

	return false;
}

void USortedStationsBPLibrary::ClearVehicleColor(UObject* WorldContextObject, UFGActorRepresentation* Representation)
{
	if (!Representation)
	{
		return;
	}

	const FString Key = GetVehicleIdentifierKey(Representation);
	if (!Key.IsEmpty())
	{
		TMap<FString, FLinearColor>& Colors = GetVehicleColorsCache();
		if (Colors.Remove(Key) > 0)
		{
			SaveVehicleColorsInternal(Colors);
		}
	}

	// バニラ本来の表現色に復元
	AActor* RealActor = Representation->GetRealActor();
	if (AFGWheeledVehicleIdentifier* VehicleIdentifier = Cast<AFGWheeledVehicleIdentifier>(RealActor))
	{
		VehicleIdentifier->UpdateRepresentation_Local();
	}
	else if (AFGWheeledVehicle* Vehicle = Cast<AFGWheeledVehicle>(RealActor))
	{
		if (AFGWheeledVehicleIdentifier* Identifier = Vehicle->GetVehicleIdentifier())
		{
			Identifier->UpdateRepresentation_Local();
		}
	}
	else if (AFGTrain* Train = Cast<AFGTrain>(RealActor))
	{
		Train->UpdateRepresentation_Local();
	}
	else if (AFGRailroadVehicle* RailVeh = Cast<AFGRailroadVehicle>(RealActor))
	{
		if (AFGTrain* ParentTrain = RailVeh->GetTrain())
		{
			ParentTrain->UpdateRepresentation_Local();
		}
	}
	else if (IFGActorRepresentationInterface* RepInterface = ResolveRepresentationInterface(RealActor))
	{
		const FLinearColor DefaultColor = RepInterface->GetActorRepresentationColor();
		RepInterface->SetActorRepresentationColor(DefaultColor);
		Representation->mRepresentationColor = DefaultColor;
	}
	else if (AFGActorRepresentationManager* Manager = AFGActorRepresentationManager::Get(WorldContextObject))
	{
		Manager->UpdateRepresentation(Representation);
	}

	// マップUI側へ再描画を直接通知
	if (AFGActorRepresentationManager* Manager = AFGActorRepresentationManager::Get(WorldContextObject))
	{
		Manager->mOnActorRepresentationUpdated.Broadcast(Representation);
	}
}

TArray<FString> USortedStationsBPLibrary::GetCustomTags()
{
	return GetTagsCache();
}

void USortedStationsBPLibrary::AddCustomTag(const FString& NewTag)
{
	if (NewTag.IsEmpty())
	{
		return;
	}
	TArray<FString>& Tags = GetTagsCache();
	// タグ上限20個ガード
	if (Tags.Num() >= 20)
	{
		return;
	}
	if (!Tags.Contains(NewTag))
	{
		Tags.Add(NewTag);
		SaveTagsInternal(Tags);
	}
}

void USortedStationsBPLibrary::UpdateCustomTag(int32 Index, const FString& NewTag)
{
	if (NewTag.IsEmpty())
	{
		return;
	}
	TArray<FString>& Tags = GetTagsCache();
	if (Tags.IsValidIndex(Index))
	{
		Tags[Index] = NewTag;
		SaveTagsInternal(Tags);
	}
}

void USortedStationsBPLibrary::RemoveCustomTag(int32 Index)
{
	TArray<FString>& Tags = GetTagsCache();
	if (Tags.IsValidIndex(Index))
	{
		Tags.RemoveAt(Index);
		SaveTagsInternal(Tags);
	}
}

void USortedStationsBPLibrary::ResetAllModSettings()
{
	// 1. カスタムタグを初期デフォルト（_IN, _OUT, _To, _From）で安全に上書き
	SaveTagsInternal({ TEXT("_IN"), TEXT("_OUT"), TEXT("_To"), TEXT("_From") });

	// 2. 車両色を空マップ（{}）で安全に上書き
	SaveVehicleColorsInternal(TMap<FString, FLinearColor>());

	UE_LOG(LogSortedStations, Display, TEXT("SortedStations: All settings have been safely reset to default values."));
}

FText USortedStationsBPLibrary::GetVanillaString(FName TableId, const FString& Key, const FText& Fallback)
{
	FText Result = FText::FromStringTable(TableId, Key);
	if (!Result.IsEmpty() && !Result.ToString().StartsWith(TEXT("<MISSING")))
	{
		return Result;
	}
	return Fallback;
}

bool USortedStationsBPLibrary::CompareStrings(const FString& A, const FString& B, int32 Mode)
{
	if (Mode == 2)
	{
#if PLATFORM_WINDOWS
		return StrCmpLogicalW(*A, *B) < 0;
#else
		return A.Compare(B) < 0;
#endif
	}
	return A.Compare(B) < 0;
}

void USortedStationsBPLibrary::ResetAllVehicleColorsInWorld(UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return;
	}

	// 全サポートタイプの表現色をデフォルト（白）にリセット
	auto ResetRep = [WorldContextObject](UFGActorRepresentation* Rep, AFGActorRepresentationManager* Manager)
	{
		if (!Rep) return;
		if (!IsSupportedRepresentationType(Rep->GetRepresentationType())) return;

		Rep->mRepresentationColor = FLinearColor::White;
		if (AActor* RealActor = Rep->GetRealActor())
		{
			if (IFGActorRepresentationInterface* RepInterface = ResolveRepresentationInterface(RealActor))
			{
				RepInterface->SetActorRepresentationColor(FLinearColor::White);
			}
		}
		if (Manager)
		{
			Manager->mOnActorRepresentationUpdated.Broadcast(Rep);
		}
	};

	AFGActorRepresentationManager* Manager = AFGActorRepresentationManager::Get(WorldContextObject);

	// プライマリ: Manager経由で全表現リスト取得
	if (Manager)
	{
		TArray<UFGActorRepresentation*> Reps;
		Manager->GetAllActorRepresentations(Reps);
		for (UFGActorRepresentation* Rep : Reps)
		{
			ResetRep(Rep, Manager);
		}
	}
	else
	{
		// フォールバック: TObjectIteratorで全UFGActorRepresentationを走査
		for (TObjectIterator<UFGActorRepresentation> It; It; ++It)
		{
			if (It && It->GetWorld() == WorldContextObject->GetWorld())
			{
				ResetRep(*It, nullptr);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
