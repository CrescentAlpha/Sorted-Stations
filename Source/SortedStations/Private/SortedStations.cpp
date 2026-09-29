/**
 * Sorted-Stations
 * Created by CrescentAlpha
 *
 * Satisfactory内の駅名、列車名、ドローン港、車両名を自動ソートするMOD。
 * Unicode文字コード順およびWindows自然順（Natural Sort）に対応。
 */

#include "SortedStations.h"
#include "SortedStationsBPLibrary.h"
#include "Patching/NativeHookManager.h"
#include "Patching/WidgetBlueprintHookManager.h"
#include "UI/SortedStationsRenameButton.h"
#include "UI/SortedStationsResetConfigWidget.h"
#include "Configuration/ModConfiguration.h"
#include "Misc/CoreDelegates.h"
#include "Engine/UserDefinedEnum.h"
#include "Internationalization/Internationalization.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "UObject/UObjectIterator.h"

#include "FGRailroadSubsystem.h"
#include "FGTrainStationIdentifier.h"

#include "FGDroneSubsystem.h"
#include "FGDroneStationInfo.h"

#include "FGActorRepresentationManager.h"

#include "Algo/Sort.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#include "Windows/HideWindowsPlatformTypes.h"
#endif

DEFINE_LOG_CATEGORY(LogSortedStations);

// ============================================================================
// static メンバー定義
// ============================================================================
FDelegateHandle          FSortedStationsModule::CultureChangedHandle;
UClass*                  FSortedStationsModule::s_ConfigClass              = nullptr;
UUserDefinedEnum*        FSortedStationsModule::s_SortModesEnum            = nullptr;
UClass*                  FSortedStationsModule::s_MapFilterCategoriesClass = nullptr;
TMap<UClass*, FProperty*> FSortedStationsModule::s_TypePropCache;
TMap<UClass*, FProperty*> FSortedStationsModule::s_ContainerPropCache;
TMap<UClass*, FProperty*> FSortedStationsModule::s_RepPropCache;
TMap<UClass*, FProperty*> FSortedStationsModule::s_NamePropCache;
TMap<UClass*, FProperty*> FSortedStationsModule::s_SubButtonsPropCache;


// ============================================================================
// 文字列比較関数群 (Comparison Functions)
// ============================================================================

/**
 * Unicode文字コード順による比較（従来の標準方式）
 */
bool CompareAlphabetic(const FString& A, const FString& B)
{
	return A.Compare(B) < 0;
}

/**
 * Windows自然順ソートによる比較（StrCmpLogicalW）
 * - 記号類（アンダースコア等）が英数字より前に整列
 * - 数値が文字コード順ではなく大小順（1, 2, 10）で整列
 * - アルファベットの大文字・小文字を同一視
 */
bool CompareNatural(const FString& A, const FString& B)
{
#if PLATFORM_WINDOWS
	return StrCmpLogicalW(*A, *B) < 0;
#else
	return A.Compare(B) < 0;
#endif
}

/**
 * 設定値（0: None, 1: Unicode, 2: Natural）に応じた比較関数を返すディスパッチャ
 * Fix #4: std::function → TFunction（UE 推奨型）
 */
TFunction<bool(const FString&, const FString&)> GetComparator(int32 Mode)
{
	if (Mode == 2)
	{
		return CompareNatural;
	}
	return CompareAlphabetic;
}

/**
 * マップマーカー表示テキストの比較ヘルパー
 * Fix #4: パラメーターを TFunctionRef に変更（即時呼び出しのため参照渡し）
 */
bool CompareMapMarkers(TFunctionRef<bool(const FString&, const FString&)> Comparator, const UFGActorRepresentation& A, const UFGActorRepresentation& B)
{
	return Comparator(A.GetRepresentationText().ToString(), B.GetRepresentationText().ToString());
}

/**
 * 列車駅名の比較ヘルパー
 * Fix #4: パラメーターを TFunctionRef に変更
 */
bool CompareTrainStations(TFunctionRef<bool(const FString&, const FString&)> Comparator, const AFGTrainStationIdentifier* A, const AFGTrainStationIdentifier* B)
{
	return A && B && Comparator(A->GetStationName().ToString(), B->GetStationName().ToString());
}

/**
 * ドローン駅名の比較ヘルパー（参照渡しにより不要なコピーを回避）
 * Fix #4: パラメーターを TFunctionRef に変更
 */
bool CompareDroneStations(TFunctionRef<bool(const FString&, const FString&)> Comparator, const FDroneStationData& A, const FDroneStationData& B)
{
	if (!A.Station || !B.Station)
	{
		return false;
	}
	return Comparator(A.Station->GetBuildingTag_Implementation(), B.Station->GetBuildingTag_Implementation());
}


// ============================================================================
// ソートアルゴリズム実装 (Sorting Implementations)
// ============================================================================

/**
 * マップマーカー配列のソート処理
 * Fix #4: std::function → TFunctionRef（テンプレート引数内でも同様）
 */
template <typename RepresentationPtr>
void SortMapMarkers(TFunctionRef<bool(const FString&, const FString&)> Comparator, TArray<RepresentationPtr>& Markers, ERepresentationType Type)
{
	TArray<RepresentationPtr> Filtered;
	for (const RepresentationPtr& MarkerValue : Markers)
	{
		UFGActorRepresentation* Marker = MarkerValue;
		if (Marker && Marker->GetRepresentationType() == Type)
		{
			Filtered.Add(MarkerValue);
		}
	}

	Algo::Sort(Filtered, [&Comparator](const RepresentationPtr& A, const RepresentationPtr& B)
	{
		UFGActorRepresentation* MarkerA = A;
		UFGActorRepresentation* MarkerB = B;
		return MarkerA && MarkerB && CompareMapMarkers(Comparator, *MarkerA, *MarkerB);
	});

	int32 FilteredIndex = 0;
	for (int32 i = 0; i < Markers.Num(); ++i)
	{
		UFGActorRepresentation* Marker = Markers[i];
		if (Marker && Marker->GetRepresentationType() == Type)
		{
			Markers[i] = Filtered[FilteredIndex++];
		}
	}
}

/**
 * 列車駅一覧のソート処理
 * Fix #4: TFunctionRef を使用
 */
void SortTrainStations(TFunctionRef<bool(const FString&, const FString&)> Comparator, TArray<AFGTrainStationIdentifier*>& Stations)
{
	Algo::Sort(Stations, [&Comparator](const AFGTrainStationIdentifier* A, const AFGTrainStationIdentifier* B)
	{
		return CompareTrainStations(Comparator, A, B);
	});
}

/**
 * ドローン駅一覧のソート処理
 * Fix #4: TFunctionRef を使用
 */
void SortDroneStations(TFunctionRef<bool(const FString&, const FString&)> Comparator, TArray<FDroneStationData>& Stations)
{
	TArray<FDroneStationData> Filtered;
	for (const FDroneStationData& Station : Stations)
	{
		if (Station.Station)
		{
			Filtered.Add(Station);
		}
	}

	Algo::Sort(Filtered, [&Comparator](const FDroneStationData& A, const FDroneStationData& B)
	{
		return CompareDroneStations(Comparator, A, B);
	});

	int32 FilteredIndex = 0;
	for (int32 i = 0; i < Stations.Num(); ++i)
	{
		if (Stations[i].Station)
		{
			Stations[i] = Filtered[FilteredIndex++];
		}
	}
}

/**
 * マップ上の全対象マーカーを一括ソートする共通関数
 */
void FSortedStationsModule::SortAllMapRepresentations(AFGActorRepresentationManager* Manager, const FSortedStationsModule_ConfigStruct_Map& Config)
{
	if (!Manager)
	{
		return;
	}

	// 列車マーカー
	if (Config.map_sort_trains != 0)
	{
		TFunction<bool(const FString&, const FString&)> Comp = GetComparator(Config.map_sort_trains);
		SortMapMarkers(Comp, Manager->mAllRepresentations, ERepresentationType::RT_Train);
	}

	// 列車駅マーカー
	if (Config.map_sort_train_stations != 0)
	{
		TFunction<bool(const FString&, const FString&)> Comp = GetComparator(Config.map_sort_train_stations);
		SortMapMarkers(Comp, Manager->mAllRepresentations, ERepresentationType::RT_TrainStation);
	}

	// ドローンマーカー
	if (Config.map_sort_drones != 0)
	{
		TFunction<bool(const FString&, const FString&)> Comp = GetComparator(Config.map_sort_drones);
		SortMapMarkers(Comp, Manager->mAllRepresentations, ERepresentationType::RT_Drone);
	}

	// ドローン港マーカー
	if (Config.map_sort_drone_stations != 0)
	{
		TFunction<bool(const FString&, const FString&)> Comp = GetComparator(Config.map_sort_drone_stations);
		SortMapMarkers(Comp, Manager->mAllRepresentations, ERepresentationType::RT_DronePort);
	}

	// 自動車駅（トラックステーション）マーカー
	if (Config.map_sort_vehicle_stations != 0)
	{
		TFunction<bool(const FString&, const FString&)> Comp = GetComparator(Config.map_sort_vehicle_stations);
		SortMapMarkers(Comp, Manager->mAllRepresentations, ERepresentationType::RT_VehicleDockingStation);
	}

	// 車両マーカー（Update 1.2 カスタム命名対応）
	if (Config.map_sort_vehicles != 0)
	{
		TFunction<bool(const FString&, const FString&)> Comp = GetComparator(Config.map_sort_vehicles);
		SortMapMarkers(Comp, Manager->mAllRepresentations, ERepresentationType::RT_Vehicle);
	}
}


void FSortedStationsModule::RequestImmediateMapSort(UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return;
	}
	if (AFGActorRepresentationManager* Manager = AFGActorRepresentationManager::Get(WorldContextObject))
	{
		const FSortedStationsModule_ConfigStruct_Map Config = FSortedStationsModule_ConfigStruct::GetActiveConfig(WorldContextObject).Map;
		SortAllMapRepresentations(Manager, Config);
	}
	// マップUI画面の各カテゴリー行コンテナ（mContainer）内の子ボタンも直接再整列
	SortMapCategoryWidgets(WorldContextObject);

	// バニラUIのOnMapFilterButtonUpdatedによる末尾AddChildを完全に打ち消すため、次フレームでも確実に再整列
	// Fix #2: CreateLambda → CreateWeakLambda（UObject* の生キャプチャによる GC 後クラッシュを防止）
	if (UWorld* World = WorldContextObject->GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateWeakLambda(WorldContextObject, [WorldContextObject]()
			{
				SortMapCategoryWidgets(WorldContextObject);
			})
		);
	}
}

void FSortedStationsModule::SortMapCategoryWidgets(UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return;
	}

	// Fix #5: GetWorld() フィルターを取得しておく
	UWorld* TargetWorld = WorldContextObject->GetWorld();
	if (!TargetWorld)
	{
		return;
	}

	const FSortedStationsModule_ConfigStruct_Map Config = FSortedStationsModule_ConfigStruct::GetActiveConfig(WorldContextObject).Map;

	for (TObjectIterator<UUserWidget> It; It; ++It)
	{
		UUserWidget* Widget = *It;

		// Fix #5: CDO と別ワールドのウィジェットをスキップ
		if (!Widget || Widget->HasAnyFlags(RF_ClassDefaultObject))
		{
			continue;
		}
		if (Widget->GetWorld() != TargetWorld)
		{
			continue;
		}

		UClass* WidgetClass = Widget->GetClass();

		// Fix #8: UClass* キャッシュによるクラス判別（文字列マッチングを廃止）
		if (s_MapFilterCategoriesClass)
		{
			// キャッシュ済み: IsA で高速判別
			if (!Widget->IsA(s_MapFilterCategoriesClass))
			{
				continue;
			}
		}
		else
		{
			// 初回のみ文字列チェック → クラスをキャッシュ
			if (!WidgetClass->GetName().Contains(TEXT("BPW_MapFilterCategories")))
			{
				continue;
			}
			s_MapFilterCategoriesClass = WidgetClass;
		}

		// representationType を取得（Fix #9: キャッシュ利用）
		FProperty** TypePropPtr = s_TypePropCache.Find(WidgetClass);
		FByteProperty* TypeProp = nullptr;
		if (TypePropPtr)
		{
			TypeProp = CastField<FByteProperty>(*TypePropPtr);
		}
		else
		{
			TypeProp = FindFProperty<FByteProperty>(WidgetClass, TEXT("representationType"));
			s_TypePropCache.Add(WidgetClass, TypeProp);
		}
		if (!TypeProp)
		{
			continue;
		}
		const uint8 RawType = TypeProp->GetPropertyValue_InContainer(Widget);
		const ERepresentationType RepType = (ERepresentationType)RawType;

		int32 SortMode = 0;
		switch (RepType)
		{
			case ERepresentationType::RT_TrainStation:          SortMode = Config.map_sort_train_stations; break;
			case ERepresentationType::RT_Train:                 SortMode = Config.map_sort_trains; break;
			case ERepresentationType::RT_DronePort:             SortMode = Config.map_sort_drone_stations; break;
			case ERepresentationType::RT_Drone:                 SortMode = Config.map_sort_drones; break;
			case ERepresentationType::RT_VehicleDockingStation: SortMode = Config.map_sort_vehicle_stations; break;
			case ERepresentationType::RT_Vehicle:               SortMode = Config.map_sort_vehicles; break;
			default: break;
		}
		if (SortMode == 0)
		{
			continue;
		}

		// mContainer プロパティ取得（Fix #9: キャッシュ利用）
		FProperty** ContainerPropPtr = s_ContainerPropCache.Find(WidgetClass);
		FObjectProperty* ContainerProp = nullptr;
		if (ContainerPropPtr)
		{
			ContainerProp = CastField<FObjectProperty>(*ContainerPropPtr);
		}
		else
		{
			ContainerProp = FindFProperty<FObjectProperty>(WidgetClass, TEXT("mContainer"));
			s_ContainerPropCache.Add(WidgetClass, ContainerProp);
		}
		if (!ContainerProp)
		{
			continue;
		}
		UPanelWidget* Container = Cast<UPanelWidget>(ContainerProp->GetObjectPropertyValue_InContainer(Widget));
		if (!Container)
		{
			continue;
		}

		// ボタンのテキスト取得ラムダ（Fix #9: クラスごとにキャッシュしたプロパティを利用）
		auto ExtractButtonText = [](const UWidget* W) -> FString
		{
			if (const UUserWidget* UW = Cast<UUserWidget>(W))
			{
				UClass* UWClass = UW->GetClass();

				// mActorRepresentation キャッシュ
				FProperty** RepPropPtr = FSortedStationsModule::s_RepPropCache.Find(UWClass);
				FObjectProperty* RepProp = RepPropPtr ? CastField<FObjectProperty>(*RepPropPtr) : nullptr;
				if (!RepPropPtr)
				{
					RepProp = FindFProperty<FObjectProperty>(UWClass, TEXT("mActorRepresentation"));
					FSortedStationsModule::s_RepPropCache.Add(UWClass, RepProp);
				}
				if (RepProp)
				{
					if (UFGActorRepresentation* Rep = Cast<UFGActorRepresentation>(RepProp->GetObjectPropertyValue_InContainer(UW)))
					{
						return Rep->GetRepresentationText().ToString();
					}
				}

				// mActorName キャッシュ
				FProperty** NamePropPtr = FSortedStationsModule::s_NamePropCache.Find(UWClass);
				FObjectProperty* NameProp = NamePropPtr ? CastField<FObjectProperty>(*NamePropPtr) : nullptr;
				if (!NamePropPtr)
				{
					NameProp = FindFProperty<FObjectProperty>(UWClass, TEXT("mActorName"));
					FSortedStationsModule::s_NamePropCache.Add(UWClass, NameProp);
				}
				if (NameProp)
				{
					if (UTextBlock* TB = Cast<UTextBlock>(NameProp->GetObjectPropertyValue_InContainer(UW)))
					{
						return TB->GetText().ToString();
					}
				}
			}
			return FString();
		};

		// Fix #7: ClearChildren + AddChild → ShiftChild による順序保持（スロット設定を破壊しない）
		auto SortButtonsInContainer = [SortMode, &ExtractButtonText](UPanelWidget* TargetContainer)
		{
			if (!TargetContainer || TargetContainer->GetChildrenCount() <= 1)
			{
				return;
			}
			const int32 Count = TargetContainer->GetChildrenCount();

			// 現在の子リストとテキストを収集
			TArray<UWidget*> Buttons;
			Buttons.Reserve(Count);
			for (int32 i = 0; i < Count; ++i)
			{
				if (UWidget* Child = TargetContainer->GetChildAt(i))
				{
					Buttons.Add(Child);
				}
			}

			// ソート済みインデックスを算出（実体ではなくインデックスを操作）
			TArray<int32> SortedIndices;
			SortedIndices.Reserve(Buttons.Num());
			for (int32 i = 0; i < Buttons.Num(); ++i)
			{
				SortedIndices.Add(i);
			}
			auto Comparator = GetComparator(SortMode);
			Algo::Sort(SortedIndices, [&Comparator, &Buttons, &ExtractButtonText](int32 A, int32 B)
			{
				return Comparator(ExtractButtonText(Buttons[A]), ExtractButtonText(Buttons[B]));
			});

			// ShiftChild で順序を変更（スロット設定を保持）
			for (int32 DestIdx = 0; DestIdx < SortedIndices.Num(); ++DestIdx)
			{
				UWidget* TargetWidget = Buttons[SortedIndices[DestIdx]];
				TargetContainer->ShiftChild(DestIdx, TargetWidget);
			}
		};

		// 1. mContainer 直下の子が BPW_MapFiltersSubCategory の場合、各サブカテゴリーの mMapFilterButtonsContainer をソート
		bool bHandledSubCategories = false;
		for (int32 SubIdx = 0; SubIdx < Container->GetChildrenCount(); ++SubIdx)
		{
			if (UUserWidget* SubCat = Cast<UUserWidget>(Container->GetChildAt(SubIdx)))
			{
				UClass* SubCatClass = SubCat->GetClass();

				// Fix #9: サブカテゴリーのコンテナプロパティもキャッシュ
				FProperty** SubPropPtr = s_SubButtonsPropCache.Find(SubCatClass);
				FObjectProperty* SubButtonsProp = SubPropPtr ? CastField<FObjectProperty>(*SubPropPtr) : nullptr;
				if (!SubPropPtr)
				{
					SubButtonsProp = FindFProperty<FObjectProperty>(SubCatClass, TEXT("mMapFilterButtonsContainer"));
					s_SubButtonsPropCache.Add(SubCatClass, SubButtonsProp);
				}
				if (SubButtonsProp)
				{
					if (UPanelWidget* SubButtonsContainer = Cast<UPanelWidget>(SubButtonsProp->GetObjectPropertyValue_InContainer(SubCat)))
					{
						bHandledSubCategories = true;
						SortButtonsInContainer(SubButtonsContainer);
					}
				}
			}
		}

		// 2. もしサブカテゴリーがなく mContainer に直接ボタンが格納されている構成の場合は直接ソート
		if (!bHandledSubCategories)
		{
			SortButtonsInContainer(Container);
		}
	}
}

void FSortedStationsModule::ApplySavedVehicleColorIfAvailable(UFGActorRepresentation* Representation)
{
	if (!Representation || !USortedStationsBPLibrary::IsSupportedRepresentationType(Representation->GetRepresentationType()))
	{
		return;
	}

	// WorldContextObject: RealActor が最も確実なコンテキスト。なければRepresentation自身にフォールバック
	UObject* WorldContextObject = Representation->GetRealActor();
	if (!WorldContextObject)
	{
		WorldContextObject = Representation;
	}

	const FLinearColor SavedColor = USortedStationsBPLibrary::GetVehicleColor(WorldContextObject, Representation);
	if (SavedColor != Representation->GetRepresentationColor())
	{
		// mRepresentationColorのみ書き換え（UpdateRepresentationを呼ばないことで再帰を防止）
		Representation->mRepresentationColor = SavedColor;
	}

	// アクター側の表現インターフェースにも保存色を同期
	if (AActor* RealActor = Representation->GetRealActor())
	{
		if (IFGActorRepresentationInterface* RepInterface = USortedStationsBPLibrary::ResolveRepresentationInterface(RealActor))
		{
			RepInterface->SetActorRepresentationColor(SavedColor);
		}
	}
}


// ============================================================================
// モジュールライフサイクル & SMLネイティブフック (Module Lifecycle & Hooks)
// ============================================================================

void FSortedStationsModule::StartupModule()
{
	// 必ず !WITH_EDITOR でガードして実機プレイ時のみフックを適用する。
	if (!WITH_EDITOR)
	{
		// Fix #6: 起動時に UClass* / UUserDefinedEnum* をキャッシュ（以降 LoadClass/LoadObject 不要）
		s_ConfigClass = LoadClass<UModConfiguration>(nullptr, TEXT("/SortedStations/SortedStationsModule_Config.SortedStationsModule_Config_C"));
		if (s_ConfigClass)
		{
			if (UModConfiguration* CDO = s_ConfigClass->GetDefaultObject<UModConfiguration>())
			{
				CDO->CustomWidget = USortedStationsResetConfigWidget::StaticClass();
				UE_LOG(LogSortedStations, Display, TEXT("Registered USortedStationsResetConfigWidget as CustomWidget on SortedStationsModule_Config"));
			}
		}
		s_SortModesEnum = LoadObject<UUserDefinedEnum>(nullptr, TEXT("/SortedStations/SortModes.SortModes"));

		// Mod設定画面へのバニラ公式翻訳テキスト動的適用（初回）
		ApplyVanillaLocalizationToConfig();

		// Fix #1: OnCultureChanged の登録は1回のみ（AddLambda のみ使用、AddStatic は廃止）
		// 古いハンドルが残っている場合は確実に除去してから登録
		if (CultureChangedHandle.IsValid())
		{
			FInternationalization::Get().OnCultureChanged().Remove(CultureChangedHandle);
			CultureChangedHandle.Reset();
		}
		CultureChangedHandle = FInternationalization::Get().OnCultureChanged().AddLambda([]()
		{
			FSortedStationsModule::ApplyVanillaLocalizationToConfig();
		});

		// --------------------------------------------------------------------
		// 1. マップ表示マーカーの初期追加フック（アクター設置・スポーン時）
		// ※UpdateRepresentationフックは高頻度毎フレーム呼び出しによる負荷防止のため排除
		// --------------------------------------------------------------------
		SUBSCRIBE_METHOD_AFTER(AFGActorRepresentationManager::AddRepresentation, [](AFGActorRepresentationManager* Self, UFGActorRepresentation* ActorRepresentation)
		{
			ApplySavedVehicleColorIfAvailable(ActorRepresentation);
			const FSortedStationsModule_ConfigStruct_Map Config = FSortedStationsModule_ConfigStruct::GetActiveConfig(Self).Map;
			SortAllMapRepresentations(Self, Config);
		});

		// --------------------------------------------------------------------
		// 2. 列車時刻表画面の駅一覧ソートフック（UI表示時のみ実行）
		// --------------------------------------------------------------------
		SUBSCRIBE_METHOD_AFTER(AFGRailroadSubsystem::GetTrainStations, [](const AFGRailroadSubsystem* Self, int TrackId, TArray<AFGTrainStationIdentifier*>& OutStations)
		{
			const FSortedStationsModule_ConfigStruct_Trains Config = FSortedStationsModule_ConfigStruct::GetActiveConfig(Self->GetGameInstance()).Trains;
			if (Config.trains_sort_train_stations != 0)
			{
				TFunction<bool(const FString&, const FString&)> Comp = GetComparator(Config.trains_sort_train_stations);
				SortTrainStations(Comp, OutStations);
			}
		});

		SUBSCRIBE_METHOD_AFTER(AFGRailroadSubsystem::GetAllTrainStations, [](const AFGRailroadSubsystem* Self, TArray<AFGTrainStationIdentifier*>& OutStations)
		{
			const FSortedStationsModule_ConfigStruct_Trains Config = FSortedStationsModule_ConfigStruct::GetActiveConfig(Self->GetGameInstance()).Trains;
			if (Config.trains_sort_train_stations != 0)
			{
				TFunction<bool(const FString&, const FString&)> Comp = GetComparator(Config.trains_sort_train_stations);
				SortTrainStations(Comp, OutStations);
			}
		});

		// --------------------------------------------------------------------
		// 3. ドローン駅検索画面のソートフック（UI検索時のみ実行）
		// --------------------------------------------------------------------
		SUBSCRIBE_METHOD_AFTER(AFGDroneSubsystem::SearchStations, [](AFGDroneSubsystem* Self, AFGDroneStationInfo* OriginStation, AFGDroneStationInfo* HostStation, FString Filter, bool ConnectionsOnly, bool ExcludeOrigin, bool PairedFirst, bool IncludeEmptyStation, TArray<FDroneStationData>& Result)
		{
			const FSortedStationsModule_ConfigStruct_Drones Config = FSortedStationsModule_ConfigStruct::GetActiveConfig(Self->GetGameInstance()).Drones;
			if (Config.drones_sort_drone_stations != 0)
			{
				TFunction<bool(const FString&, const FString&)> Comp = GetComparator(Config.drones_sort_drone_stations);
				SortDroneStations(Comp, Result);
			}
		});

		// --------------------------------------------------------------------
		// 4. マップ画面への鉛筆ボタン自動注入（OnPostEngineInit で登録）
		// --------------------------------------------------------------------
		FCoreDelegates::OnPostEngineInit.AddLambda([]()
		{
			if (GEngine)
			{
				if (UWidgetBlueprintHookManager* HookManager = GEngine->GetEngineSubsystem<UWidgetBlueprintHookManager>())
				{
					UWidgetBlueprintHookData* HookData = NewObject<UWidgetBlueprintHookData>();
					HookData->WidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/FactoryGame/Interface/UI/Minimap/MapFilters/BPW_MapFilterButton.BPW_MapFilterButton_C")));
					HookData->NewWidgetClass = USortedStationsRenameButton::StaticClass();
					HookData->NewWidgetName = FName(TEXT("SortedStations_RenameButton"));
					HookData->ParentWidgetName = TEXT("mCustomButtonsContainer");
					HookData->ParentSlotIndex = 0;
					HookManager->RegisterWidgetBlueprintHook(HookData);
					UE_LOG(LogSortedStations, Display, TEXT("Registered WidgetBlueprintHook for BPW_MapFilterButton (SortedStations_RenameButton)"));
				}
			}

			// Fix #6: CDO への適用は起動時キャッシュ済みクラスを使用
			if (FSortedStationsModule::s_ConfigClass)
			{
				ApplyVanillaLocalizationToConfig();
			}
		});
	}
}

void FSortedStationsModule::ShutdownModule()
{
	if (CultureChangedHandle.IsValid())
	{
		FInternationalization::Get().OnCultureChanged().Remove(CultureChangedHandle);
		CultureChangedHandle.Reset();
	}
}

void FSortedStationsModule::ApplyVanillaLocalizationToConfig()
{
	const FText StrSort          = USortedStationsBPLibrary::GetVanillaString(TEXT("General_UI"), TEXT("GenericUIStrings/Sort/Button"), FText::FromString(TEXT("Sort")));
	const FText StrMap           = USortedStationsBPLibrary::GetVanillaString(TEXT("Map_UI"), TEXT("MapMenu/WindowHeader"), FText::FromString(TEXT("Map")));
	const FText StrTrainStations = USortedStationsBPLibrary::GetVanillaString(TEXT("Map_UI"), TEXT("MapMenu/Filter/Categories/TrainStations"), FText::FromString(TEXT("Train Stations")));
	const FText StrTrains        = USortedStationsBPLibrary::GetVanillaString(TEXT("Map_UI"), TEXT("MapMenu/Filter/Categories/Trains"), FText::FromString(TEXT("Trains")));
	const FText StrDronePorts    = USortedStationsBPLibrary::GetVanillaString(TEXT("Buildables_Data"), TEXT("Transport/Drones/DronePort"), FText::FromString(TEXT("Drone Port")));
	const FText StrDrones        = USortedStationsBPLibrary::GetVanillaString(TEXT("Map_UI"), TEXT("MapMenu/Filter/Categories/Drones"), FText::FromString(TEXT("Drones")));
	const FText StrTruckStations = USortedStationsBPLibrary::GetVanillaString(TEXT("Buildables_Data"), TEXT("Transport/RoadVehicles/Infrastructure/TruckStation"), FText::FromString(TEXT("Truck Station")));
	const FText StrVehicles      = USortedStationsBPLibrary::GetVanillaString(TEXT("Map_UI"), TEXT("MapMenu/Filter/Categories/Vehicles"), FText::FromString(TEXT("Vehicles")));
	const FText StrTimetable     = USortedStationsBPLibrary::GetVanillaString(TEXT("Transportation_UI"), TEXT("Trains/Locomotive/Menu/TimeTable"), FText::FromString(TEXT("Timetable")));

	auto MakeSortLabel = [&StrSort](const FText& Target) -> FText {
		return FText::Format(FText::FromString(TEXT("{0}: {1}")), StrSort, Target);
	};

	auto UpdateSectionProps = [&](UConfigPropertySection* TargetSec)
	{
		if (!TargetSec) return;
		for (auto& Pair : TargetSec->SectionProperties)
		{
			const FString& Key = Pair.Key;
			UConfigProperty* Prop = Pair.Value;
			if (!Prop) continue;

			if (UConfigPropertySection* SubSec = Cast<UConfigPropertySection>(Prop))
			{
				if      (Key.Equals(TEXT("Map"), ESearchCase::IgnoreCase))    SubSec->DisplayName = StrMap;
				else if (Key.Equals(TEXT("Trains"), ESearchCase::IgnoreCase)) SubSec->DisplayName = StrTrains;
				else if (Key.Equals(TEXT("Drones"), ESearchCase::IgnoreCase)) SubSec->DisplayName = StrDrones;

				for (auto& SubPair : SubSec->SectionProperties)
				{
					const FString& SubKey = SubPair.Key;
					UConfigProperty* SubProp = SubPair.Value;
					if (!SubProp) continue;

					if      (SubKey == TEXT("map_sort_train_stations"))   SubProp->DisplayName = MakeSortLabel(StrTrainStations);
					else if (SubKey == TEXT("map_sort_trains"))           SubProp->DisplayName = MakeSortLabel(StrTrains);
					else if (SubKey == TEXT("map_sort_drone_stations"))   SubProp->DisplayName = MakeSortLabel(StrDronePorts);
					else if (SubKey == TEXT("map_sort_drones"))           SubProp->DisplayName = MakeSortLabel(StrDrones);
					else if (SubKey == TEXT("map_sort_vehicle_stations")) SubProp->DisplayName = MakeSortLabel(StrTruckStations);
					else if (SubKey == TEXT("map_sort_vehicles"))         SubProp->DisplayName = MakeSortLabel(StrVehicles);
					else if (SubKey == TEXT("trains_sort_trains"))
					{
						SubProp->DisplayName = FText::Format(FText::FromString(TEXT("{0}: {1} ({2})")), StrSort, StrTrains, StrTimetable);
						SubProp->Tooltip = FText::GetEmpty();
					}
					else if (SubKey == TEXT("trains_sort_train_stations"))
					{
						SubProp->DisplayName = FText::Format(FText::FromString(TEXT("{0}: {1} ({2})")), StrSort, StrTrainStations, StrTimetable);
						SubProp->Tooltip = FText::GetEmpty();
					}
					else if (SubKey == TEXT("drones_sort_drone_stations"))
					{
						SubProp->DisplayName = MakeSortLabel(StrDronePorts);
						SubProp->Tooltip = FText::GetEmpty();
					}
				}
			}
		}
	};

	// 1. CDOのRootSectionを直接更新（Fix #6: キャッシュ済みクラスを使用）
	if (s_ConfigClass)
	{
		if (UModConfiguration* CDO = s_ConfigClass->GetDefaultObject<UModConfiguration>())
		{
			if (CDO->RootSection)
			{
				UpdateSectionProps(CDO->RootSection);
			}
		}
	}

	// 2. メモリ上の全実行時セクションインスタンスも更新
	for (TObjectIterator<UConfigPropertySection> It; It; ++It)
	{
		UConfigPropertySection* Sec = *It;
		if (!Sec) continue;

		if (Sec->SectionProperties.Contains(TEXT("Map")) || Sec->SectionProperties.Contains(TEXT("Trains")) || Sec->SectionProperties.Contains(TEXT("Drones")))
		{
			UpdateSectionProps(Sec);
		}
	}

	// Fix #3: UUserDefinedEnum::DisplayNameMap の実行時変更
	// 注意: 公式には非サポートだが、パッケージビルド（ゲーム実行時）では Blueprint コンパイルが発生しないため
	// エディタ環境での副作用（アセット汚染・コンパイル破損）は生じない。
	// SML が列挙値のローカライズ API を提供していない以上、現状ではこのアプローチが唯一実用的な方法。
	// CDO は変更しない（RF_ClassDefaultObject ガード）。
	if (s_SortModesEnum && !s_SortModesEnum->HasAnyFlags(RF_ClassDefaultObject))
	{
		const FText StrNone = USortedStationsBPLibrary::GetVanillaString(TEXT("Buildables_UI"), TEXT("Fluids/Module/None"), FText::FromString(TEXT("None")));
		for (auto& Pair : s_SortModesEnum->DisplayNameMap)
		{
			if (Pair.Value.ToString() == TEXT("None") || Pair.Value.ToString() == TEXT("なし") || Pair.Value.ToString() == TEXT("无"))
			{
				Pair.Value = StrNone;
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSortedStationsModule, SortedStations)
