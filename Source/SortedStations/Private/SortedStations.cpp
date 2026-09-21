/**
 * Sorted-Stations
 * Created by CrescentAlpha
 *
 * Satisfactory内の駅名、列車名、ドローン港、車両名を自動ソートするMOD。
 * Unicode文字コード順およびWindows自然順（Natural Sort）に対応。
 */

#include "SortedStations.h"
#include "Patching/NativeHookManager.h"

#include "FGRailroadSubsystem.h"
#include "FGTrainStationIdentifier.h"

#include "FGDroneSubsystem.h"
#include "FGDroneStationInfo.h"

#include "FGActorRepresentationManager.h"

#include "Algo/Sort.h"
#include <functional>

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#define LOCTEXT_NAMESPACE "FSortedStationsModule"

DEFINE_LOG_CATEGORY(LogSortedStations);


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
 */
std::function<bool(const FString&, const FString&)> GetComparator(int32 Mode)
{
	if (Mode == 2)
	{
		return CompareNatural;
	}
	return CompareAlphabetic;
}

/**
 * マップマーカー表示テキストの比較ヘルパー
 */
bool CompareMapMarkers(std::function<bool(const FString&, const FString&)> Comparator, const UFGActorRepresentation& A, const UFGActorRepresentation& B)
{
	return Comparator(A.GetRepresentationText().ToString(), B.GetRepresentationText().ToString());
}

/**
 * 列車駅名の比較ヘルパー
 */
bool CompareTrainStations(std::function<bool(const FString&, const FString&)> Comparator, const AFGTrainStationIdentifier* A, const AFGTrainStationIdentifier* B)
{
	return A && B && Comparator(A->GetStationName().ToString(), B->GetStationName().ToString());
}

/**
 * ドローン駅名の比較ヘルパー（参照渡しにより不要なコピーを回避）
 */
bool CompareDroneStations(std::function<bool(const FString&, const FString&)> Comparator, const FDroneStationData& A, const FDroneStationData& B)
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
 * - マップ全体のマーカー配列から、指定されたType（駅、車両等）のみを抽出してソート
 * - ソート後のマーカーを元のインデックス位置へと順番に再配置
 * - TArray::Sort ではなく Algo::Sort を使用し、UE 5.6 の非推奨警告（C4996）を解消
 */
template <typename RepresentationPtr>
void SortMapMarkers(std::function<bool(const FString&, const FString&)> Comparator, TArray<RepresentationPtr>& Markers, ERepresentationType Type)
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

	Algo::Sort(Filtered, [Comparator](const RepresentationPtr& A, const RepresentationPtr& B)
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
 */
void SortTrainStations(std::function<bool(const FString&, const FString&)> Comparator, TArray<AFGTrainStationIdentifier*>& Stations)
{
	Algo::Sort(Stations, [Comparator](const AFGTrainStationIdentifier* A, const AFGTrainStationIdentifier* B)
	{
		return CompareTrainStations(Comparator, A, B);
	});
}

/**
 * ドローン駅一覧のソート処理
 */
void SortDroneStations(std::function<bool(const FString&, const FString&)> Comparator, TArray<FDroneStationData>& Stations)
{
	TArray<FDroneStationData> Filtered;
	for (const FDroneStationData& Station : Stations)
	{
		if (Station.Station)
		{
			Filtered.Add(Station);
		}
	}

	Algo::Sort(Filtered, [Comparator](const FDroneStationData& A, const FDroneStationData& B)
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
 * AddRepresentation と UpdateRepresentation の重複を解消
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
		SortMapMarkers(GetComparator(Config.map_sort_trains), Manager->mAllRepresentations, ERepresentationType::RT_Train);
	}

	// 列車駅マーカー
	if (Config.map_sort_train_stations != 0)
	{
		SortMapMarkers(GetComparator(Config.map_sort_train_stations), Manager->mAllRepresentations, ERepresentationType::RT_TrainStation);
	}

	// ドローンマーカー
	if (Config.map_sort_drones != 0)
	{
		SortMapMarkers(GetComparator(Config.map_sort_drones), Manager->mAllRepresentations, ERepresentationType::RT_Drone);
	}

	// ドローン港マーカー
	if (Config.map_sort_drone_stations != 0)
	{
		SortMapMarkers(GetComparator(Config.map_sort_drone_stations), Manager->mAllRepresentations, ERepresentationType::RT_DronePort);
	}

	// 自動車駅（トラックステーション）マーカー
	if (Config.map_sort_vehicle_stations != 0)
	{
		SortMapMarkers(GetComparator(Config.map_sort_vehicle_stations), Manager->mAllRepresentations, ERepresentationType::RT_VehicleDockingStation);
	}

	// 車両マーカー（Update 1.2 カスタム命名対応）
	if (Config.map_sort_vehicles != 0)
	{
		SortMapMarkers(GetComparator(Config.map_sort_vehicles), Manager->mAllRepresentations, ERepresentationType::RT_Vehicle);
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
		// --------------------------------------------------------------------
		// 1. マップ表示マーカーのソートフック
		// --------------------------------------------------------------------
		SUBSCRIBE_METHOD_AFTER(AFGActorRepresentationManager::AddRepresentation, [](AFGActorRepresentationManager* Self, UFGActorRepresentation* ActorRepresentation)
		{
			const FSortedStationsModule_ConfigStruct_Map Config = FSortedStationsModule_ConfigStruct::GetActiveConfig(Self).Map;
			SortAllMapRepresentations(Self, Config);
		});

		SUBSCRIBE_METHOD_AFTER(AFGActorRepresentationManager::UpdateRepresentation, [](bool Result, AFGActorRepresentationManager* Self, UFGActorRepresentation* ActorRepresentation)
		{
			const FSortedStationsModule_ConfigStruct_Map Config = FSortedStationsModule_ConfigStruct::GetActiveConfig(Self).Map;
			SortAllMapRepresentations(Self, Config);
		});

		// --------------------------------------------------------------------
		// 2. 列車時刻表画面の駅一覧ソートフック
		// --------------------------------------------------------------------
		SUBSCRIBE_METHOD_AFTER(AFGRailroadSubsystem::GetTrainStations, [](const AFGRailroadSubsystem* Self, int TrackId, TArray<AFGTrainStationIdentifier*>& OutStations)
		{
			const FSortedStationsModule_ConfigStruct_Trains Config = FSortedStationsModule_ConfigStruct::GetActiveConfig(Self->GetGameInstance()).Trains;
			if (Config.trains_sort_train_stations != 0)
			{
				SortTrainStations(GetComparator(Config.trains_sort_train_stations), OutStations);
			}
		});

		SUBSCRIBE_METHOD_AFTER(AFGRailroadSubsystem::GetAllTrainStations, [](const AFGRailroadSubsystem* Self, TArray<AFGTrainStationIdentifier*>& OutStations)
		{
			const FSortedStationsModule_ConfigStruct_Trains Config = FSortedStationsModule_ConfigStruct::GetActiveConfig(Self->GetGameInstance()).Trains;
			if (Config.trains_sort_train_stations != 0)
			{
				SortTrainStations(GetComparator(Config.trains_sort_train_stations), OutStations);
			}
		});

		// --------------------------------------------------------------------
		// 3. ドローン駅検索画面のソートフック
		// --------------------------------------------------------------------
		SUBSCRIBE_METHOD_AFTER(AFGDroneSubsystem::SearchStations, [](AFGDroneSubsystem* Self, AFGDroneStationInfo* OriginStation, AFGDroneStationInfo* HostStation, FString Filter, bool ConnectionsOnly, bool ExcludeOrigin, bool PairedFirst, bool IncludeEmptyStation, TArray<FDroneStationData>& Result)
		{
			const FSortedStationsModule_ConfigStruct_Drones Config = FSortedStationsModule_ConfigStruct::GetActiveConfig(Self->GetGameInstance()).Drones;
			if (Config.drones_sort_drone_stations != 0)
			{
				SortDroneStations(GetComparator(Config.drones_sort_drone_stations), Result);
			}
		});
	}
}

void FSortedStationsModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSortedStationsModule, SortedStations)
