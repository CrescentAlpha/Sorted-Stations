#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SortedStationsResetConfigWidget.generated.h"

class UVerticalBox;
class UHorizontalBox;
class UButton;
class UTextBlock;
class UBorder;

/**
 * Mod設定画面（Mods > Sorted Stations）に配置されるModデータ初期化ウィジェット
 * UMG（WidgetTree）ベースで構築され、バニラの自動翻訳StringTableに対応
 * 押しやすいボタンから2段階確認ダイアログを展開し、安全に初期化を実行
 */
UCLASS()
class SORTEDSTATIONS_API USortedStationsResetConfigWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	UPROPERTY()
	TObjectPtr<UButton> TriggerButton;

	UPROPERTY()
	TObjectPtr<UBorder> ConfirmBorder;

	UPROPERTY()
	TObjectPtr<UButton> ConfirmButton;

	UPROPERTY()
	TObjectPtr<UButton> CancelButton;

	UPROPERTY()
	TObjectPtr<UTextBlock> FeedbackText;

	bool bIsConfirmOpen = false;
	bool bLocalizationApplied = false;
	FTimerHandle FeedbackTimerHandle;


	UFUNCTION()
	void OnTriggerClicked();

	UFUNCTION()
	void OnConfirmClicked();

	UFUNCTION()
	void OnCancelClicked();

	void ResetToDefaultState();
	void BuildDefaultWidgetTree();
};
