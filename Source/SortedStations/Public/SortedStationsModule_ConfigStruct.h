#pragma once
#include "CoreMinimal.h"
#include "Configuration/ConfigManager.h"
#include "Engine/Engine.h"
#include "SortedStationsModule_ConfigStruct.generated.h"

struct FSortedStationsModule_ConfigStruct_Map;
struct FSortedStationsModule_ConfigStruct_Trains;
struct FSortedStationsModule_ConfigStruct_Drones;

/**
 * マップ上のマーカーソート設定
 * 0: None（ソート無効）
 * 1: Unicode（従来の文字コード順、初期デフォルト）
 * 2: Natural（Windows自然順ソート）
 */
USTRUCT(BlueprintType)
struct FSortedStationsModule_ConfigStruct_Map {
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite)
    int32 map_sort_train_stations{1};

    UPROPERTY(BlueprintReadWrite)
    int32 map_sort_trains{1};

    UPROPERTY(BlueprintReadWrite)
    int32 map_sort_drone_stations{1};

    UPROPERTY(BlueprintReadWrite)
    int32 map_sort_drones{1};

    UPROPERTY(BlueprintReadWrite)
    int32 map_sort_vehicle_stations{1};

    UPROPERTY(BlueprintReadWrite)
    int32 map_sort_vehicles{1};
};

/**
 * 列車時刻表のソート設定
 */
USTRUCT(BlueprintType)
struct FSortedStationsModule_ConfigStruct_Trains {
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite)
    int32 trains_sort_trains{1};

    UPROPERTY(BlueprintReadWrite)
    int32 trains_sort_train_stations{1};
};

/**
 * ドローン駅検索のソート設定
 */
USTRUCT(BlueprintType)
struct FSortedStationsModule_ConfigStruct_Drones {
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite)
    int32 drones_sort_drone_stations{1};
};

/**
 * Sorted-Stations ルート設定構造体
 * Mod Configuration アセット '/SortedStations/SortedStationsModule_Config' に対応
 */
USTRUCT(BlueprintType)
struct FSortedStationsModule_ConfigStruct {
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite)
    FSortedStationsModule_ConfigStruct_Map Map{};

    UPROPERTY(BlueprintReadWrite)
    FSortedStationsModule_ConfigStruct_Trains Trains{};

    UPROPERTY(BlueprintReadWrite)
    FSortedStationsModule_ConfigStruct_Drones Drones{};

    /** アクティブな設定値を取得して構造体に格納して返す */
    static FSortedStationsModule_ConfigStruct GetActiveConfig(UObject* WorldContext) {
        FSortedStationsModule_ConfigStruct ConfigStruct{};
        FConfigId ConfigId{"SortedStations", ""};
        if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull)) {
            UConfigManager* ConfigManager = World->GetGameInstance()->GetSubsystem<UConfigManager>();
            ConfigManager->FillConfigurationStruct(ConfigId, FDynamicStructInfo{FSortedStationsModule_ConfigStruct::StaticStruct(), &ConfigStruct});
        }
        return ConfigStruct;
    }
};
