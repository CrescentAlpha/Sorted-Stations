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

private:
	/** マップ上の全対象マーカーを一括ソート（AccessTransformersのfriend権限でアクセス） */
	static void SortAllMapRepresentations(class AFGActorRepresentationManager* Manager, const struct FSortedStationsModule_ConfigStruct_Map& Config);
};
