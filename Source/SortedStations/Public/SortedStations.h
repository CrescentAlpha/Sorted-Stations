#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "SortedStationsModule_ConfigStruct.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSortedStations, Verbose, All);

/**
 * Sorted-Stations モジュールクラス
 * Satisfactory内の駅名、列車名、ドローン港、車両名を自動ソートするMOD
 */
class FSortedStationsModule : public IModuleInterface
{
public:
	/** IModuleInterface 実装 */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** マップマーカーの再ソートを実行（AccessTransformersのフレンド経由） */
	static void RequestImmediateMapSort(UObject* WorldContextObject);

	/** マップUI上の各カテゴリウィジェット内の子ボタンを行コンテナ内で直接ソート・再配置 */
	static void SortMapCategoryWidgets(UObject* WorldContextObject);

	/** 保存されたカスタム車両カラーをRepresentationに適用 */
	static void ApplySavedVehicleColorIfAvailable(class UFGActorRepresentation* Representation);

	/** バニラ公式StringTableから各公式翻訳テキストを取得しMod設定画面に動的適用 */
	static void ApplyVanillaLocalizationToConfig();

	static FDelegateHandle CultureChangedHandle;

	// ---------------------------------------------------------
	// キャッシュ（繰り返し LoadClass / LoadObject / FindFProperty を避けるため）
	// ---------------------------------------------------------
	/** SortedStationsModule_Config の UClass*（起動時にキャッシュ） */
	static UClass* s_ConfigClass;
	/** SortModes UUserDefinedEnum*（起動時にキャッシュ） */
	static class UUserDefinedEnum* s_SortModesEnum;
	/** BPW_MapFilterCategories の UClass*（初回マッチ時にキャッシュ） */
	static UClass* s_MapFilterCategoriesClass;

	/** UClass* → FProperty* のキャッシュ（SortMapCategoryWidgets 用） */
	static TMap<UClass*, FProperty*> s_TypePropCache;
	static TMap<UClass*, FProperty*> s_ContainerPropCache;
	static TMap<UClass*, FProperty*> s_RepPropCache;
	static TMap<UClass*, FProperty*> s_NamePropCache;
	static TMap<UClass*, FProperty*> s_SubButtonsPropCache;

private:
	/** マップ上の全対象マーカーを一括ソート（AccessTransformersのfriend権限でアクセス） */
	static void SortAllMapRepresentations(class AFGActorRepresentationManager* Manager, const struct FSortedStationsModule_ConfigStruct_Map& Config);
};
