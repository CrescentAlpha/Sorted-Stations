#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SortedStationsRenameButton.generated.h"

class UFGActorRepresentation;
class UButton;
class USortedStationsVehicleRenameDialog;

/**
 * マップ画面の各行（BPW_MapFilterButton）に追加される編集アイコンボタン
 * 車両マーカー（RT_Vehicle）の行のみ可視化され、クリックでリネームダイアログを開く
 *
 * アイコン: ✎ (U+270E) をSlateテキストグリフとして描画（テクスチャ不要・Shipping安定）
 */
UCLASS()
class SORTEDSTATIONS_API USortedStationsRenameButton : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** 対象のRepresentationを設定 */
	UFUNCTION(BlueprintCallable, Category = "Sorted Stations")
	void SetTargetRepresentation(UFGActorRepresentation* InRepresentation);

	/** 現在のターゲットRepresentationを取得 */
	UFUNCTION(BlueprintPure, Category = "Sorted Stations")
	UFGActorRepresentation* GetTargetRepresentation() const { return TargetRepresentation; }

	UFUNCTION()
	void OnParentUnhovered();

	void ForceMaintainParentVisibility();

protected:
	UFUNCTION()
	void OnButtonClicked();

	/** 親のBPW_MapFilterButtonからRepresentationを自動解決 */
	void ResolveTargetRepresentation();

	/** Slate 動的 Visibility 判定（RT_Vehicle のみ Visible を返す） */
	EVisibility GetButtonVisibility() const;

	void MaintainParentVisibility();

	UPROPERTY(BlueprintReadOnly, Category = "Sorted Stations")
	TObjectPtr<UFGActorRepresentation> TargetRepresentation;

	// Blueprint側がバインドする場合のオプションプロパティ（C++では未使用）
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> EditButton;

	FTimerHandle VisibilityMaintainTimerHandle;
	FTimerHandle VisibilityLoopTimerHandle;
	TWeakObjectPtr<USortedStationsVehicleRenameDialog> SpawnedDialog;
};
