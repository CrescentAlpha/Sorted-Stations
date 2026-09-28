#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SortedStationsVehicleRenameDialog.generated.h"

class UFGActorRepresentation;
class UEditableTextBox;
class UButton;
class UTextBlock;
class UBorder;
class UScrollBox;
class UWrapBox;
class UImage;
class UOverlay;
class USizeBox;
class UHorizontalBox;
class UVerticalBox;
class USortedStationsVehicleRenameDialog;

/** タグ挿入・削除ボタンのクリックイベントを安全にディスパッチするヘルパー */
UCLASS()
class SORTEDSTATIONS_API USortedStationsTagActionHelper : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TWeakObjectPtr<USortedStationsVehicleRenameDialog> Dialog;

	FString TagText;
	int32 TagIndex = -1;
	bool bIsDelete = false;

	UFUNCTION()
	void OnClicked();
};

/**
 * 車両名リネーム・カラー変更・タグクイック挿入用モーダルダイアログ
 * バニラ標準のローカライズ文字列を活用し、1画面に全機能が美しく収まる設計
 */
UCLASS()
class SORTEDSTATIONS_API USortedStationsVehicleRenameDialog : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** 起動元ボタンを設定 */
	void SetParentButton(class USortedStationsRenameButton* InButton) { ParentButton = InButton; }

	/** ダイアログを初期化 */
	UFUNCTION(BlueprintCallable, Category = "Sorted Stations")
	void InitDialog(UFGActorRepresentation* InRepresentation);

	UFUNCTION()
	void OnTagButtonClicked(FString TagText);

	void OnDeleteTagClicked(int32 TagIndex);

	friend class USortedStationsTagActionHelper;

protected:
	/** ウィジェットツリーをプログラム的に構築（Blueprint側未バインド時の完全フォールバック） */
	void BuildDefaultWidgetTree();

	/** タグ一覧を再構築 */
	void RefreshTagsList();

	/** 現在のカラープレビューを更新 */
	void UpdateColorPreview(FLinearColor NewColor);

	UFUNCTION()
	void OnConfirmClicked();

	UFUNCTION()
	void OnCancelClicked();

	UFUNCTION()
	void OnResetColorClicked();

	UFUNCTION()
	void OnColorButtonClicked();

	UFUNCTION()
	void OnToggleTagManageMode();

	UFUNCTION()
	void OnAddTagClicked();

	void OpenColorPickerModal();
	void CloseColorPickerModal();

	/** 選択カラーをRepresentationとマップアイコンに即時適用 */
	void ApplyColorToRepresentation(FLinearColor Color);

	UFUNCTION()
	void OnColorPickerApplyClicked();

	UFUNCTION()
	void OnColorPickerCancelClicked();

	UPROPERTY(BlueprintReadOnly, Category = "Sorted Stations")
	TObjectPtr<UFGActorRepresentation> TargetRepresentation;

	UPROPERTY(BlueprintReadOnly, Category = "Sorted Stations")
	FLinearColor CurrentSelectedColor;

	UPROPERTY(BlueprintReadOnly, Category = "Sorted Stations")
	bool bIsTagManageMode = false;

	// UIコンポーネント
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> NameInputBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> TagsContainer;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> NewTagInputBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> AddTagButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> ColorSectionBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> ColorPreviewBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ColorHexText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ConfirmButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CancelButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ResetColorButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ColorSelectButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> TagManageButton;

	// モーダルカラーピッカー用
	UPROPERTY()
	TObjectPtr<UOverlay> ModalColorPickerOverlay;

	UPROPERTY()
	TObjectPtr<UUserWidget> ActiveColorPickerWidget;

	// タグアクションヘルパー保持用（GC保護）
	UPROPERTY()
	TArray<TObjectPtr<USortedStationsTagActionHelper>> TagHelpers;

	// ウィンドウドラッグ移動用
	bool bIsDraggingDialog = false;
	FVector2D DialogDragStartMousePos = FVector2D::ZeroVector;
	FVector2D DialogCurrentTranslation = FVector2D::ZeroVector;

	bool bIsDraggingModal = false;
	FVector2D ModalDragStartMousePos = FVector2D::ZeroVector;
	FVector2D ModalCurrentTranslation = FVector2D::ZeroVector;

	UPROPERTY()
	TObjectPtr<USizeBox> DialogSizeBox;

	UPROPERTY()
	TObjectPtr<USizeBox> ModalSizeBox;

	UPROPERTY()
	TObjectPtr<UHorizontalBox> MainHeaderBox;

	UPROPERTY()
	TObjectPtr<UHorizontalBox> ModalHeaderBox;

	UPROPERTY()
	TObjectPtr<UTextBlock> TitleLabel;

	TWeakObjectPtr<class USortedStationsRenameButton> ParentButton;
};
