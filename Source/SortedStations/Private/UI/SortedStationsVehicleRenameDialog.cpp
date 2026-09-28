#include "UI/SortedStationsVehicleRenameDialog.h"
#include "SortedStationsBPLibrary.h"
#include "FGActorRepresentation.h"

#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"

#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Blueprint/WidgetTree.h"
#include "FGActorRepresentationManager.h"
#include "SortedStations.h"
#include "UI/SortedStationsRenameButton.h"



// ============================================================
// 共通スタイル生成ヘルパー（ファイルスコープ）
// ============================================================
namespace SortedStationsDialogStyle
{
	/** FICSIT標準ダーク背景ブラシ */
	static FSlateColorBrush MakeDarkBg()   { return FSlateColorBrush(FLinearColor(0.06f, 0.07f, 0.09f, 0.97f)); }
	/** Focusありの入力欄背景 */
	static FSlateColorBrush MakeInputBg()  { return FSlateColorBrush(FLinearColor(0.10f, 0.11f, 0.14f, 0.98f)); }
	/** FICSITオレンジ（ホバー） */
	static FSlateColorBrush MakeOrange()   { return FSlateColorBrush(FLinearColor(0.95f, 0.42f, 0.04f, 1.0f)); }
	/** 区切り線ブラシ */
	static FSlateColorBrush MakeSep()      { return FSlateColorBrush(FLinearColor(0.18f, 0.20f, 0.24f, 1.0f)); }

	/** テキスト入力欄スタイルを構成して返す */
	static FEditableTextBoxStyle MakeInputStyle()
	{
		FEditableTextBoxStyle Style = FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox");
		FSlateColorBrush BgBrush   = MakeInputBg();
		FSlateColorBrush FocBrush(FLinearColor(0.12f, 0.14f, 0.18f, 1.0f));
		Style.SetBackgroundImageNormal(BgBrush);
		Style.SetBackgroundImageHovered(FocBrush);
		Style.SetBackgroundImageFocused(FocBrush);
		Style.SetForegroundColor(FSlateColor(FLinearColor::White));
		// パディングを小さめに（ゲームUIらしく）
		Style.Padding = FMargin(6.0f, 4.0f);
		return Style;
	}

	/** 汎用ボタンスタイル（通常:ダーク / ホバー:オレンジ） */
	static FButtonStyle MakeButtonStyle()
	{
		FButtonStyle Style = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
		Style.SetNormal(FSlateColorBrush(FLinearColor(0.14f, 0.16f, 0.20f, 0.90f)));
		Style.SetHovered(MakeOrange());
		Style.SetPressed(FSlateColorBrush(FLinearColor(0.72f, 0.32f, 0.03f, 1.0f)));
		Style.NormalPadding  = FMargin(8.0f, 4.0f);
		Style.PressedPadding = FMargin(8.0f, 5.0f, 8.0f, 3.0f);
		return Style;
	}

	/** 小型ボタン（タグ・削除用） */
	static FButtonStyle MakeSmallButtonStyle()
	{
		FButtonStyle Style = MakeButtonStyle();
		Style.NormalPadding  = FMargin(5.0f, 2.0f);
		Style.PressedPadding = FMargin(5.0f, 3.0f, 5.0f, 1.0f);
		return Style;
	}

	/** 削除ボタンスタイル（赤系） */
	static FButtonStyle MakeDeleteButtonStyle()
	{
		FButtonStyle Style = MakeSmallButtonStyle();
		Style.SetNormal(FSlateColorBrush(FLinearColor(0.45f, 0.08f, 0.08f, 0.85f)));
		Style.SetHovered(FSlateColorBrush(FLinearColor(0.80f, 0.12f, 0.12f, 1.0f)));
		Style.SetPressed(FSlateColorBrush(FLinearColor(0.60f, 0.10f, 0.10f, 1.0f)));
		return Style;
	}

	/** ラベルテキストブロックをセットアップ */
	static void SetupLabel(UTextBlock* TB, float FontSize = 11.0f, FLinearColor Color = FLinearColor(0.78f, 0.82f, 0.88f, 1.0f))
	{
		if (!TB) return;
		TB->SetColorAndOpacity(FSlateColor(Color));
		FSlateFontInfo Font = FCoreStyle::Get().GetFontStyle("NormalFont");
		Font.Size = FMath::RoundToInt(FontSize);
		TB->SetFont(Font);
	}

	/** セクションヘッダラベルをセットアップ（ゴールド色） */
	static void SetupSectionLabel(UTextBlock* TB, float FontSize = 11.0f)
	{
		SetupLabel(TB, FontSize, FLinearColor(1.0f, 0.80f, 0.20f, 1.0f));
	}
}

// ============================================================
// USortedStationsTagActionHelper
// ============================================================

void USortedStationsTagActionHelper::OnClicked()
{
	if (Dialog.IsValid())
	{
		if (bIsDelete)
		{
			Dialog->OnDeleteTagClicked(TagIndex);
		}
		else
		{
			Dialog->OnTagButtonClicked(TagText);
		}
	}
}

// ============================================================
// USortedStationsVehicleRenameDialog
// ============================================================

void USortedStationsVehicleRenameDialog::NativeConstruct()
{
	Super::NativeConstruct();

	if (!NameInputBox)
	{
		BuildDefaultWidgetTree();
	}

	// ボタンイベントのバインド（Blueprint側でバインド済みの場合は二重登録を防ぐ）
	if (ConfirmButton)    ConfirmButton->OnClicked.AddDynamic(this, &USortedStationsVehicleRenameDialog::OnConfirmClicked);
	if (CancelButton)     CancelButton->OnClicked.AddDynamic(this, &USortedStationsVehicleRenameDialog::OnCancelClicked);
	if (ResetColorButton) ResetColorButton->OnClicked.AddDynamic(this, &USortedStationsVehicleRenameDialog::OnResetColorClicked);
	if (ColorSelectButton) ColorSelectButton->OnClicked.AddDynamic(this, &USortedStationsVehicleRenameDialog::OnColorButtonClicked);
	if (TagManageButton)  TagManageButton->OnClicked.AddDynamic(this, &USortedStationsVehicleRenameDialog::OnToggleTagManageMode);
	if (AddTagButton)     AddTagButton->OnClicked.AddDynamic(this, &USortedStationsVehicleRenameDialog::OnAddTagClicked);

	SetIsFocusable(true);
}

void USortedStationsVehicleRenameDialog::NativeDestruct()
{
	CloseColorPickerModal();

	if (ConfirmButton)     ConfirmButton->OnClicked.RemoveAll(this);
	if (CancelButton)      CancelButton->OnClicked.RemoveAll(this);
	if (ResetColorButton)  ResetColorButton->OnClicked.RemoveAll(this);
	if (ColorSelectButton) ColorSelectButton->OnClicked.RemoveAll(this);
	if (TagManageButton)   TagManageButton->OnClicked.RemoveAll(this);
	if (AddTagButton)      AddTagButton->OnClicked.RemoveAll(this);

	Super::NativeDestruct();
}

void USortedStationsVehicleRenameDialog::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 1. 起動元ボタンの生存を監視（マップ画面が閉じられたらボタンがDestructされ無効化する）
	if (ParentButton.IsStale() || !ParentButton.IsValid())
	{
		RemoveFromParent();
		return;
	}

	// 2. マウスカーソルの表示状態を監視（マップやメニューが閉じられて通常ゲームプレイに戻った場合）
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (!PC->bShowMouseCursor)
		{
			RemoveFromParent();
			return;
		}
	}
}

FReply USortedStationsVehicleRenameDialog::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	const bool bIsCancelKey = (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::Gamepad_Special_Right);

	// カラーピッカー表示中はカラーピッカーのみ閉じる
	if (ModalColorPickerOverlay)
	{
		if (bIsCancelKey)
		{
			CloseColorPickerModal();
			return FReply::Handled();
		}
	}
	else if (bIsCancelKey)
	{
		OnCancelClicked();
		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply USortedStationsVehicleRenameDialog::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	const bool bIsCancelKey = (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::Gamepad_Special_Right);

	// カラーピッカー表示中はキャンセルキーで閉じる、他のキーはブロック
	if (ModalColorPickerOverlay)
	{
		if (bIsCancelKey)
		{
			CloseColorPickerModal();
			return FReply::Handled();
		}
		return FReply::Handled();
	}

	if (Key == EKeys::Enter)
	{
		OnConfirmClicked();
		return FReply::Handled();
	}
	else if (bIsCancelKey)
	{
		OnCancelClicked();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply USortedStationsVehicleRenameDialog::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const FVector2D MousePos = InMouseEvent.GetScreenSpacePosition();

		// カラーピッカー表示中: モーダルヘッダーをクリックしたか判定
		if (ModalColorPickerOverlay && ModalHeaderBox)
		{
			const FGeometry HeaderGeom = ModalHeaderBox->GetTickSpaceGeometry();
			if (HeaderGeom.IsUnderLocation(MousePos))
			{
				bIsDraggingModal = true;
				return FReply::Handled().CaptureMouse(TakeWidget());
			}
		}

		// メインダイアログ: メインヘッダーをクリックしたか判定
		if (MainHeaderBox)
		{
			const FGeometry HeaderGeom = MainHeaderBox->GetTickSpaceGeometry();
			if (HeaderGeom.IsUnderLocation(MousePos))
			{
				bIsDraggingDialog = true;
				return FReply::Handled().CaptureMouse(TakeWidget());
			}
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply USortedStationsVehicleRenameDialog::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (bIsDraggingDialog || bIsDraggingModal)
		{
			bIsDraggingDialog = false;
			bIsDraggingModal = false;
			return FReply::Handled().ReleaseMouseCapture();
		}
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply USortedStationsVehicleRenameDialog::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDraggingModal && ModalSizeBox)
	{
		ModalCurrentTranslation += InMouseEvent.GetCursorDelta();
		ModalSizeBox->SetRenderTranslation(ModalCurrentTranslation);
		return FReply::Handled();
	}
	else if (bIsDraggingDialog && DialogSizeBox)
	{
		DialogCurrentTranslation += InMouseEvent.GetCursorDelta();
		DialogSizeBox->SetRenderTranslation(DialogCurrentTranslation);
		return FReply::Handled();
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void USortedStationsVehicleRenameDialog::InitDialog(UFGActorRepresentation* InRepresentation)
{
	TargetRepresentation = InRepresentation;
	if (!TargetRepresentation)
	{
		return;
	}

	// 表示位置を中央（オフセット0）にリセット
	DialogCurrentTranslation = FVector2D::ZeroVector;
	if (DialogSizeBox)
	{
		DialogSizeBox->SetRenderTranslation(FVector2D::ZeroVector);
	}

	if (!NameInputBox)
	{
		BuildDefaultWidgetTree();
	}

	// タイトルにバニラ公式表示名（電車の駅、トラックステーション、ドローン港、車両種別等）を設定
	const FText VanillaName = USortedStationsBPLibrary::GetVanillaDisplayName(TargetRepresentation);
	if (TitleLabel)
	{
		TitleLabel->SetText(!VanillaName.IsEmpty() ? VanillaName : USortedStationsBPLibrary::GetVanillaString(TEXT("Menus_UI"), TEXT("Menu/Pause/Options"), NSLOCTEXT("SortedStations", "Settings", "Settings")));
	}

	// 現在の名前をセット（駅の "駅: " などのプレフィックスを除去した生の名前）
	const FText CurrentName = USortedStationsBPLibrary::GetRawActorName(TargetRepresentation);
	if (NameInputBox)
	{
		NameInputBox->SetText(CurrentName);
		NameInputBox->SetKeyboardFocus();
	}

	// カラーセクションの表示/非表示（車両・列車のみ対応。トラックステーション、駅、ドローンポート、ドローンは非表示）
	if (ColorSectionBox)
	{
		const ERepresentationType RepType = TargetRepresentation->GetRepresentationType();
		const bool bSupportColor = (RepType == ERepresentationType::RT_Vehicle || RepType == ERepresentationType::RT_Train);
		ColorSectionBox->SetVisibility(bSupportColor ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	// 現在のカラーを読み込み
	CurrentSelectedColor = USortedStationsBPLibrary::GetVehicleColor(this, TargetRepresentation);
	UpdateColorPreview(CurrentSelectedColor);

	// タグ一覧をリフレッシュ
	RefreshTagsList();
}

void USortedStationsVehicleRenameDialog::UpdateColorPreview(FLinearColor NewColor)
{
	const FLinearColor AdjustedColor = USortedStationsBPLibrary::AdjustColorForContrast(NewColor);
	CurrentSelectedColor = AdjustedColor;

	if (ColorPreviewBorder)
	{
		// アルファが0に近い場合は未着色/デフォルト表示
		if (AdjustedColor.A <= 0.01f)
		{
			ColorPreviewBorder->SetBrushColor(FLinearColor(0.85f, 0.85f, 0.85f, 1.0f));
		}
		else
		{
			ColorPreviewBorder->SetBrushColor(AdjustedColor);
		}
	}
	if (ColorHexText)
	{
		if (AdjustedColor.A <= 0.01f)
		{
			ColorHexText->SetText(USortedStationsBPLibrary::GetVanillaString(TEXT("Customization_Data"), TEXT("Colors/Default"), NSLOCTEXT("SortedStations", "Default", "Default")));
		}
		else
		{
			const FString HexStr = AdjustedColor.ToFColor(true).ToHex();
			ColorHexText->SetText(FText::FromString(FString::Printf(TEXT("#%s"), *HexStr.Left(6))));
		}
	}
}

void USortedStationsVehicleRenameDialog::RefreshTagsList()
{
	if (!TagsContainer || !WidgetTree)
	{
		return;
	}

	TagsContainer->ClearChildren();
	TagHelpers.Empty();

	const TArray<FString> Tags = USortedStationsBPLibrary::GetCustomTags();

	// タグ上限（20個）に達した場合は追加ボタンを無効化
	if (AddTagButton)
	{
		AddTagButton->SetIsEnabled(Tags.Num() < 20);
	}

	const FButtonStyle TagBtnStyle = SortedStationsDialogStyle::MakeSmallButtonStyle();
	const FButtonStyle DelBtnStyle = SortedStationsDialogStyle::MakeDeleteButtonStyle();

	FSlateFontInfo TagFont = FCoreStyle::Get().GetFontStyle("NormalFont");
	TagFont.Size = 10;

	for (int32 i = 0; i < Tags.Num(); ++i)
	{
		const FString TagText = Tags[i];
		const int32 TagIndex  = i;

		// --- タグ行（タグボタン + 必要に応じて×ボタン） ---
		UHorizontalBox* ItemBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

		// タグ挿入ボタン
		UButton* TagBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		TagBtn->SetStyle(TagBtnStyle);

		UTextBlock* TagLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		TagLabel->SetText(FText::FromString(TagText));
		TagLabel->SetFont(TagFont);
		TagLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		TagLabel->SetClipping(EWidgetClipping::ClipToBounds);
		TagBtn->AddChild(TagLabel);

		UHorizontalBoxSlot* TagBtnSlot = ItemBox->AddChildToHorizontalBox(TagBtn);
		TagBtnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

		// タグ挿入ヘルパー登録
		USortedStationsTagActionHelper* InsertHelper = NewObject<USortedStationsTagActionHelper>(this);
		InsertHelper->Dialog    = this;
		InsertHelper->TagText   = TagText;
		InsertHelper->bIsDelete = false;
		TagHelpers.Add(InsertHelper);
		TagBtn->OnClicked.AddDynamic(InsertHelper, &USortedStationsTagActionHelper::OnClicked);

		// 管理モード時: × 削除ボタンを追加
		if (bIsTagManageMode)
		{
			UButton* DelBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
			DelBtn->SetStyle(DelBtnStyle);

			UTextBlock* DelLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			DelLabel->SetText(FText::FromString(TEXT("\u00D7"))); // ×
			DelLabel->SetFont(TagFont);
			DelLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			DelBtn->AddChild(DelLabel);

			UHorizontalBoxSlot* DelBtnSlot = ItemBox->AddChildToHorizontalBox(DelBtn);
			DelBtnSlot->SetPadding(FMargin(2.0f, 0.0f, 0.0f, 0.0f));
			DelBtnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

			USortedStationsTagActionHelper* DelHelper = NewObject<USortedStationsTagActionHelper>(this);
			DelHelper->Dialog    = this;
			DelHelper->TagIndex  = TagIndex;
			DelHelper->bIsDelete = true;
			TagHelpers.Add(DelHelper);
			DelBtn->OnClicked.AddDynamic(DelHelper, &USortedStationsTagActionHelper::OnClicked);
		}

		UWrapBoxSlot* WrapSlot = TagsContainer->AddChildToWrapBox(ItemBox);
		if (WrapSlot)
		{
			WrapSlot->SetPadding(FMargin(2.0f, 2.0f));
		}
	}
}

// ============================================================
// イベントハンドラ
// ============================================================

void USortedStationsVehicleRenameDialog::OnTagButtonClicked(FString TagText)
{
	if (!NameInputBox)
	{
		return;
	}

	const FString Current = NameInputBox->GetText().ToString();
	const FString NewText = Current + TagText;

	// 30文字制限ガード
	if (NewText.Len() <= 30)
	{
		NameInputBox->SetText(FText::FromString(NewText));
	}
}

void USortedStationsVehicleRenameDialog::OnToggleTagManageMode()
{
	bIsTagManageMode = !bIsTagManageMode;

	if (TagManageButton)
	{
		if (UTextBlock* Label = Cast<UTextBlock>(TagManageButton->GetChildAt(0)))
		{
			Label->SetText(bIsTagManageMode
				? USortedStationsBPLibrary::GetVanillaString(TEXT("General_UI"), TEXT("GenericUIStrings/Confirm/Button"), FText::FromString(TEXT("Confirm")))
				: USortedStationsBPLibrary::GetVanillaString(TEXT("Menus_UI"), TEXT("Options/OptionsColor/Edit/Button"), FText::FromString(TEXT("Edit"))));
		}
	}

	RefreshTagsList();
}

void USortedStationsVehicleRenameDialog::OnAddTagClicked()
{
	if (!NewTagInputBox)
	{
		return;
	}

	const TArray<FString> ExistingTags = USortedStationsBPLibrary::GetCustomTags();
	if (ExistingTags.Num() >= 20)
	{
		return;
	}

	FString RawTag = NewTagInputBox->GetText().ToString().TrimStartAndEnd();
	if (RawTag.IsEmpty())
	{
		return;
	}

	// ユーザーが入力した文字そのまま登録（最大長15文字ガード）
	if (RawTag.Len() > 15)
	{
		RawTag = RawTag.Left(15);
	}

	USortedStationsBPLibrary::AddCustomTag(RawTag);
	NewTagInputBox->SetText(FText::GetEmpty());
	RefreshTagsList();
}

void USortedStationsVehicleRenameDialog::OnDeleteTagClicked(int32 TagIndex)
{
	USortedStationsBPLibrary::RemoveCustomTag(TagIndex);
	RefreshTagsList();
}

void USortedStationsVehicleRenameDialog::OnColorButtonClicked()
{
	OpenColorPickerModal();
}

void USortedStationsVehicleRenameDialog::OnResetColorClicked()
{
	if (TargetRepresentation)
	{
		// 保存された色設定を削除し、バニラ本来のRepresentation更新を実行
		USortedStationsBPLibrary::ClearVehicleColor(this, TargetRepresentation);

		// バニラ本来の色を取得してプレビューに反映（白で再保存しない）
		const FLinearColor DefaultColor = TargetRepresentation->GetRepresentationColor();
		UpdateColorPreview(DefaultColor);
	}
}

void USortedStationsVehicleRenameDialog::OnConfirmClicked()
{
	if (TargetRepresentation && NameInputBox)
	{
		const FText NewName = NameInputBox->GetText();
		USortedStationsBPLibrary::RenameVehicle(this, TargetRepresentation, NewName);
		USortedStationsBPLibrary::SetVehicleColor(this, TargetRepresentation, CurrentSelectedColor);
		// マップアイコン色を即時反映
		ApplyColorToRepresentation(CurrentSelectedColor);

		// 親行のテキストを即時更新
		if (ParentButton.IsValid())
		{
			ParentButton->OnParentUnhovered();

			if (UUserWidget* ParentRow = ParentButton->GetTypedOuter<UUserWidget>())
			{
				if (FObjectProperty* NameProp = FindFProperty<FObjectProperty>(ParentRow->GetClass(), TEXT("mActorName")))
				{
					if (UTextBlock* NameBlock = Cast<UTextBlock>(NameProp->GetObjectPropertyValue_InContainer(ParentRow)))
					{
						NameBlock->SetText(NewName);
					}
				}
				if (FObjectProperty* BlurProp = FindFProperty<FObjectProperty>(ParentRow->GetClass(), TEXT("mActorNameBlur")))
				{
					if (UTextBlock* BlurBlock = Cast<UTextBlock>(BlurProp->GetObjectPropertyValue_InContainer(ParentRow)))
					{
						BlurBlock->SetText(NewName);
					}
				}

				// 対象のアクター表現タイプに応じたソート設定（自然順/Unicode順）を取得
				int32 SortMode = 0;
				const auto& MapConfig = FSortedStationsModule_ConfigStruct::GetActiveConfig(this).Map;
				if (TargetRepresentation)
				{
					switch (TargetRepresentation->GetRepresentationType())
					{
						case ERepresentationType::RT_TrainStation:          SortMode = MapConfig.map_sort_train_stations; break;
						case ERepresentationType::RT_Train:                 SortMode = MapConfig.map_sort_trains; break;
						case ERepresentationType::RT_DronePort:             SortMode = MapConfig.map_sort_drone_stations; break;
						case ERepresentationType::RT_Drone:                 SortMode = MapConfig.map_sort_drones; break;
						case ERepresentationType::RT_VehicleDockingStation: SortMode = MapConfig.map_sort_vehicle_stations; break;
						case ERepresentationType::RT_Vehicle:               SortMode = MapConfig.map_sort_vehicles; break;
						default: break;
					}
				}

				// SortMode が 0 (None) の場合は即時ソートを行わず、バニラの並び順を維持
				// SortMode が 1 (Unicode) または 2 (Natural) の場合のみ即時ソートを反映
				if (SortMode != 0)
				{
					if (UPanelWidget* Container = ParentRow->GetParent())
					{
						const int32 ChildCount = Container->GetChildrenCount();
						TArray<UWidget*> Rows;
						for (int32 i = 0; i < ChildCount; ++i)
						{
							if (UWidget* Child = Container->GetChildAt(i))
							{
								Rows.Add(Child);
							}
						}

						auto GetRowText = [](const UWidget* RowWidget) -> FString {
							if (const UUserWidget* RowUW = Cast<UUserWidget>(RowWidget))
							{
								if (FObjectProperty* RepProp = FindFProperty<FObjectProperty>(RowUW->GetClass(), TEXT("mActorRepresentation")))
								{
									if (UFGActorRepresentation* Rep = Cast<UFGActorRepresentation>(RepProp->GetObjectPropertyValue_InContainer(RowUW)))
									{
										return Rep->GetRepresentationText().ToString();
									}
								}
								if (FObjectProperty* P = FindFProperty<FObjectProperty>(RowUW->GetClass(), TEXT("mActorName")))
								{
									if (UTextBlock* TB = Cast<UTextBlock>(P->GetObjectPropertyValue_InContainer(RowUW)))
									{
										return TB->GetText().ToString();
									}
								}
							}
							return FString();
						};

						Rows.Sort([SortMode, &GetRowText](const UWidget& A, const UWidget& B) {
							return USortedStationsBPLibrary::CompareStrings(GetRowText(&A), GetRowText(&B), SortMode);
						});

						Container->ClearChildren();
						for (UWidget* SortedRow : Rows)
						{
							Container->AddChild(SortedRow);
						}
					}
				}
			}
		}

		// アクター表現の更新をゲーム全体にブロードキャスト
		AFGActorRepresentationManager* Manager = AFGActorRepresentationManager::Get(this);
		if (Manager)
		{
			Manager->mOnActorRepresentationUpdated.Broadcast(TargetRepresentation);
		}

		// データ層およびマップ全体の整合性を維持
		UObject* ContextObj = Manager ? static_cast<UObject*>(Manager) : static_cast<UObject*>(this);
		FSortedStationsModule::RequestImmediateMapSort(ContextObj);
	}

	RemoveFromParent();
}

void USortedStationsVehicleRenameDialog::OnCancelClicked()
{
	RemoveFromParent();
}

// ============================================================
// カラー反映（マップアイコンへの即時適用）
// ============================================================

void USortedStationsVehicleRenameDialog::ApplyColorToRepresentation(FLinearColor Color)
{
	if (!TargetRepresentation)
	{
		return;
	}

	// SetVehicleColor() が mRepresentationColor の書き換えと UpdateRepresentation() の両方を行う
	// （直接の protected メンバアクセスはダイアログクラスから不可のため、BPLibrary経由で実施）
	USortedStationsBPLibrary::SetVehicleColor(this, TargetRepresentation, Color);
}

// ============================================================
// カラーピッカーモーダル
// ============================================================

void USortedStationsVehicleRenameDialog::OpenColorPickerModal()
{
	// 重複オープン防止
	if (ModalColorPickerOverlay)
	{
		return;
	}

	UOverlay* RootOverlay = Cast<UOverlay>(WidgetTree ? WidgetTree->RootWidget : nullptr);
	if (!RootOverlay)
	{
		return;
	}

	// --- モーダルルートオーバーレイ ---
	ModalColorPickerOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ModalColorPickerOverlay"));
	UOverlaySlot* ModalRootSlot = RootOverlay->AddChildToOverlay(ModalColorPickerOverlay);
	ModalRootSlot->SetHorizontalAlignment(HAlign_Fill);
	ModalRootSlot->SetVerticalAlignment(VAlign_Fill);

	// 1. 半透明ディマー（背景クリックブロック）
	UBorder* ModalDimmer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ModalDimmer"));
	ModalDimmer->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.65f));
	UOverlaySlot* DimmerSlot = ModalColorPickerOverlay->AddChildToOverlay(ModalDimmer);
	DimmerSlot->SetHorizontalAlignment(HAlign_Fill);
	DimmerSlot->SetVerticalAlignment(VAlign_Fill);

	// 2. 中央配置モーダルウィンドウ（内部ウィジェットBPW_ColorPickerPopupのDesiredSizeに自動追従）
	ModalSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ModalSizeBox"));
	ModalSizeBox->SetMaxDesiredHeight(600.0f);
	ModalCurrentTranslation = FVector2D::ZeroVector;
	ModalSizeBox->SetRenderTranslation(FVector2D::ZeroVector);
	UOverlaySlot* BoxSlot = ModalColorPickerOverlay->AddChildToOverlay(ModalSizeBox);
	BoxSlot->SetHorizontalAlignment(HAlign_Center);
	BoxSlot->SetVerticalAlignment(VAlign_Center);

	UBorder* WindowFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ModalFrame"));
	WindowFrame->SetBrushColor(FLinearColor(0.05f, 0.06f, 0.08f, 0.98f));
	WindowFrame->SetPadding(FMargin(14.0f));
	ModalSizeBox->AddChild(WindowFrame);

	UVerticalBox* Layout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ModalLayout"));
	WindowFrame->AddChild(Layout);

	// --- ヘッダー（タイトル + 閉じるボタン） ---
	ModalHeaderBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ModalHeader"));
	UVerticalBoxSlot* HeaderSlot = Layout->AddChildToVerticalBox(ModalHeaderBox);
	HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ModalTitle"));
	Title->SetText(USortedStationsBPLibrary::GetVanillaString(TEXT("Buildables_UI"), TEXT("Lighting/LightControlPanel/Label/Preview/Color"), NSLOCTEXT("SortedStations", "Color", "Color")));
	SortedStationsDialogStyle::SetupSectionLabel(Title, 12.0f);
	ModalHeaderBox->AddChildToHorizontalBox(Title);

	USpacer* HeaderSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	UHorizontalBoxSlot* SpacerSlot = ModalHeaderBox->AddChildToHorizontalBox(HeaderSpacer);
	SpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// 閉じる（✕）ボタン
	const FButtonStyle CloseStyle = SortedStationsDialogStyle::MakeSmallButtonStyle();
	UButton* CloseBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ModalCloseBtn"));
	CloseBtn->SetStyle(CloseStyle);
	CloseBtn->OnClicked.AddDynamic(this, &USortedStationsVehicleRenameDialog::OnColorPickerCancelClicked);
	UTextBlock* CloseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CloseText->SetText(FText::FromString(TEXT("\u2715"))); // ✕
	SortedStationsDialogStyle::SetupLabel(CloseText, 11.0f);
	CloseBtn->AddChild(CloseText);
	ModalHeaderBox->AddChildToHorizontalBox(CloseBtn);

	// 区切り線
	UBorder* Sep = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Sep->SetBrushColor(FLinearColor(0.18f, 0.20f, 0.24f, 1.0f));
	UVerticalBoxSlot* SepSlot = Layout->AddChildToVerticalBox(Sep);
	SepSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

	// --- カラーピッカー本体（BPW_ColorPickerPopup） ---
	UClass* PickerClass = LoadClass<UUserWidget>(nullptr,
		TEXT("/Game/FactoryGame/Interface/UI/InGame/ColorPicker/BPW_ColorPickerPopup.BPW_ColorPickerPopup_C"));
	if (PickerClass)
	{
		APlayerController* PC = GetOwningPlayer();
		if (!PC)
		{
			PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
		}
		ActiveColorPickerWidget = CreateWidget<UUserWidget>(PC, PickerClass);
		if (ActiveColorPickerWidget)
		{
			// 初期カラーをプロパティ mStartColor にセット（リフレクション経由）
			if (FProperty* StartColorProp = PickerClass->FindPropertyByName(TEXT("mStartColor")))
			{
				if (FStructProperty* StructProp = CastField<FStructProperty>(StartColorProp))
				{
					if (StructProp->Struct == TBaseStructure<FLinearColor>::Get())
					{
						*StructProp->ContainerPtrToValuePtr<FLinearColor>(ActiveColorPickerWidget) = CurrentSelectedColor;
					}
				}
			}

			UVerticalBoxSlot* ContentSlot = Layout->AddChildToVerticalBox(ActiveColorPickerWidget);
			ContentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
	}

	// --- フッター（キャンセル + 決定） ---
	UHorizontalBox* Footer = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ModalFooter"));
	UVerticalBoxSlot* FooterSlot = Layout->AddChildToVerticalBox(Footer);
	FooterSlot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 0.0f));

	USpacer* FooterSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	UHorizontalBoxSlot* FootSpacerSlot = Footer->AddChildToHorizontalBox(FooterSpacer);
	FootSpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	const FButtonStyle FootBtnStyle = SortedStationsDialogStyle::MakeButtonStyle();

	UButton* CancelBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ModalCancelBtn"));
	CancelBtn->SetStyle(FootBtnStyle);
	CancelBtn->OnClicked.AddDynamic(this, &USortedStationsVehicleRenameDialog::OnColorPickerCancelClicked);
	UTextBlock* CancelLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CancelLabel->SetText(USortedStationsBPLibrary::GetVanillaString(TEXT("General_UI"), TEXT("GenericUIStrings/Cancel/Button"), NSLOCTEXT("SortedStations", "Cancel", "Cancel")));
	SortedStationsDialogStyle::SetupLabel(CancelLabel);
	CancelBtn->AddChild(CancelLabel);
	Footer->AddChildToHorizontalBox(CancelBtn);

	UButton* ApplyBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ModalApplyBtn"));
	ApplyBtn->SetStyle(FootBtnStyle);
	ApplyBtn->OnClicked.AddDynamic(this, &USortedStationsVehicleRenameDialog::OnColorPickerApplyClicked);
	UTextBlock* ApplyLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ApplyLabel->SetText(USortedStationsBPLibrary::GetVanillaString(TEXT("Buildables_UI"), TEXT("BlueprintMenu/Category/Edit/Apply/Button"), NSLOCTEXT("SortedStations", "Apply", "Apply")));
	SortedStationsDialogStyle::SetupLabel(ApplyLabel);
	ApplyBtn->AddChild(ApplyLabel);
	UHorizontalBoxSlot* ApplySlot = Footer->AddChildToHorizontalBox(ApplyBtn);
	ApplySlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
}

void USortedStationsVehicleRenameDialog::CloseColorPickerModal()
{
	if (ModalColorPickerOverlay)
	{
		ModalColorPickerOverlay->RemoveFromParent();
		ModalColorPickerOverlay  = nullptr;
		ActiveColorPickerWidget  = nullptr;
		ModalSizeBox             = nullptr;
		ModalHeaderBox           = nullptr;
		bIsDraggingModal         = false;
	}
}

void USortedStationsVehicleRenameDialog::OnColorPickerApplyClicked()
{
	if (ActiveColorPickerWidget)
	{
		// ブループリント関数 GetColor() をリフレクションで実行して選択色を取得
		if (UFunction* GetColorFunc = ActiveColorPickerWidget->FindFunction(TEXT("GetColor")))
		{
			struct FGetColorParams { FLinearColor ReturnValue; };
			FGetColorParams Params;
			ActiveColorPickerWidget->ProcessEvent(GetColorFunc, &Params);
			UpdateColorPreview(Params.ReturnValue);
		}
	}

	CloseColorPickerModal();
}

void USortedStationsVehicleRenameDialog::OnColorPickerCancelClicked()
{
	CloseColorPickerModal();
}

// ============================================================
// BuildDefaultWidgetTree — プログラマティックUI構築
// ============================================================

void USortedStationsVehicleRenameDialog::BuildDefaultWidgetTree()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	// --- スタイル定数 ---
	const FEditableTextBoxStyle InputStyle = SortedStationsDialogStyle::MakeInputStyle();
	const FButtonStyle BtnStyle            = SortedStationsDialogStyle::MakeButtonStyle();
	const FButtonStyle SmallBtnStyle       = SortedStationsDialogStyle::MakeSmallButtonStyle();

	FSlateFontInfo SmallFont = FCoreStyle::Get().GetFontStyle("NormalFont");
	SmallFont.Size = 10;
	FSlateFontInfo NormalFont = FCoreStyle::Get().GetFontStyle("NormalFont");
	NormalFont.Size = 11;

	// ===================================================
	// 1. ルートオーバーレイ（ダイアログ全体のコンテナ）
	// ===================================================
	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RootOverlay"));
	WidgetTree->RootWidget = RootOverlay;

	// 暗転背景ディマー
	UBorder* DimmerBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DimmerBorder"));
	DimmerBorder->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.60f));
	UOverlaySlot* DimmerSlot = RootOverlay->AddChildToOverlay(DimmerBorder);
	DimmerSlot->SetHorizontalAlignment(HAlign_Fill);
	DimmerSlot->SetVerticalAlignment(VAlign_Fill);

	// ダイアログ本体ウィンドウ（中央寄せ）
	DialogSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DialogSizeBox"));
	DialogSizeBox->SetWidthOverride(480.0f);
	DialogSizeBox->SetMaxDesiredHeight(580.0f);
	DialogCurrentTranslation = FVector2D::ZeroVector;
	DialogSizeBox->SetRenderTranslation(FVector2D::ZeroVector);
	UOverlaySlot* WindowSlot = RootOverlay->AddChildToOverlay(DialogSizeBox);
	WindowSlot->SetHorizontalAlignment(HAlign_Center);
	WindowSlot->SetVerticalAlignment(VAlign_Center);

	// FICSIT風ウィンドウフレーム
	UBorder* WindowFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("WindowFrame"));
	WindowFrame->SetBrushColor(FLinearColor(0.05f, 0.06f, 0.08f, 0.98f));
	WindowFrame->SetPadding(FMargin(16.0f));
	DialogSizeBox->AddChild(WindowFrame);

	// ===================================================
	// 2. メインレイアウト（縦並び）
	// ===================================================
	UVerticalBox* MainLayout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainLayout"));
	WindowFrame->AddChild(MainLayout);

	// --- ダイアログタイトル（ヘッダー行・ドラッグハンドル） ---
	MainHeaderBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MainHeaderBox"));
	UVerticalBoxSlot* TitleSlot = MainLayout->AddChildToVerticalBox(MainHeaderBox);
	TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));

	TitleLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleLabel"));
	TitleLabel->SetText(USortedStationsBPLibrary::GetVanillaString(TEXT("Menus_UI"), TEXT("Menu/Pause/Options"), NSLOCTEXT("SortedStations", "Settings", "Settings")));
	SortedStationsDialogStyle::SetupSectionLabel(TitleLabel, 13.0f);
	MainHeaderBox->AddChildToHorizontalBox(TitleLabel);

	// 区切り線
	auto MakeSep = [&](UVerticalBox* VBox, float TopPad = 0.0f, float BotPad = 8.0f)
	{
		UBorder* Sep = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Sep->SetBrushColor(FLinearColor(0.18f, 0.20f, 0.24f, 1.0f));
		UVerticalBoxSlot* Slot = VBox->AddChildToVerticalBox(Sep);
		Slot->SetPadding(FMargin(0.0f, TopPad, 0.0f, BotPad));
		return Sep;
	};
	MakeSep(MainLayout, 0.0f, 10.0f);

	// ===================================================
	// 3. スクロールコンテンツ
	// ===================================================
	UScrollBox* ContentScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ContentScroll"));
	UVerticalBoxSlot* ScrollSlot = MainLayout->AddChildToVerticalBox(ContentScroll);
	ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UVerticalBox* ScrollContent = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ScrollContent"));
	ContentScroll->AddChild(ScrollContent);

	// --- [A] 名前セクション ---
	UTextBlock* InputHeader = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InputHeader"));
	InputHeader->SetText(USortedStationsBPLibrary::GetVanillaString(TEXT("Player_UI"), TEXT("InventorySlot/Settings/Name"), NSLOCTEXT("SortedStations", "Name", "Name")));
	SortedStationsDialogStyle::SetupLabel(InputHeader, 10.0f);
	ScrollContent->AddChildToVerticalBox(InputHeader);

	NameInputBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("NameInputBox"));
	NameInputBox->SetHintText(USortedStationsBPLibrary::GetVanillaString(TEXT("Map_UI"), TEXT("MapMarker/Name/TextBoxHint"), FText::FromString(TEXT("Enter Name..."))));
	NameInputBox->WidgetStyle = InputStyle;
	UVerticalBoxSlot* InputSlot = ScrollContent->AddChildToVerticalBox(NameInputBox);
	InputSlot->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 12.0f));

	// --- [B] タグセクション ---
	// ヘッダー行（"Tags" ラベル + 入力欄 + ＋ボタン + Manageボタン）
	UHorizontalBox* TagHeaderBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TagHeaderBox"));
	ScrollContent->AddChildToVerticalBox(TagHeaderBox);

	UTextBlock* TagHeaderLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TagHeaderLabel"));
	TagHeaderLabel->SetText(USortedStationsBPLibrary::GetVanillaString(TEXT("Map_UI"), TEXT("MapMarker/Subcategory"), FText::FromString(TEXT("Subcategory"))));
	SortedStationsDialogStyle::SetupLabel(TagHeaderLabel, 10.0f);
	UHorizontalBoxSlot* TagLabelSlot = TagHeaderBox->AddChildToHorizontalBox(TagHeaderLabel);
	TagLabelSlot->SetVerticalAlignment(VAlign_Center);

	USpacer* TagSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	UHorizontalBoxSlot* TagSpacerSlot = TagHeaderBox->AddChildToHorizontalBox(TagSpacer);
	TagSpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// 新規タグ入力欄（小型）
	NewTagInputBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("NewTagInputBox"));
	NewTagInputBox->SetHintText(USortedStationsBPLibrary::GetVanillaString(TEXT("Map_UI"), TEXT("MapMarker/Subcategory/TextBoxHint"), FText::FromString(TEXT("Subcategory..."))));
	// 小型の入力欄スタイル
	FEditableTextBoxStyle SmallInputStyle = InputStyle;
	SmallInputStyle.Padding = FMargin(5.0f, 3.0f);
	NewTagInputBox->WidgetStyle = SmallInputStyle;
	UHorizontalBoxSlot* NewTagInputSlot = TagHeaderBox->AddChildToHorizontalBox(NewTagInputBox);
	NewTagInputSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	NewTagInputSlot->SetPadding(FMargin(6.0f, 0.0f));
	NewTagInputSlot->SetVerticalAlignment(VAlign_Center);

	// ＋ボタン（タグ追加）
	AddTagButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("AddTagButton"));
	AddTagButton->SetStyle(SmallBtnStyle);
	UTextBlock* AddTagText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	AddTagText->SetText(FText::FromString(TEXT("+")));
	AddTagText->SetFont(SmallFont);
	AddTagText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	AddTagButton->AddChild(AddTagText);
	UHorizontalBoxSlot* AddSlot = TagHeaderBox->AddChildToHorizontalBox(AddTagButton);
	AddSlot->SetVerticalAlignment(VAlign_Center);

	// Manageボタン（タグ管理モード切替）
	TagManageButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("TagManageButton"));
	TagManageButton->SetStyle(SmallBtnStyle);
	UTextBlock* ManageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ManageText->SetText(USortedStationsBPLibrary::GetVanillaString(TEXT("Menus_UI"), TEXT("Options/OptionsColor/Edit/Button"), FText::FromString(TEXT("Edit"))));
	ManageText->SetFont(SmallFont);
	ManageText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	TagManageButton->AddChild(ManageText);
	UHorizontalBoxSlot* ManageSlot = TagHeaderBox->AddChildToHorizontalBox(TagManageButton);
	ManageSlot->SetPadding(FMargin(4.0f, 0.0f, 0.0f, 0.0f));
	ManageSlot->SetVerticalAlignment(VAlign_Center);

	// タグ一覧（WrapBox）
	TagsContainer = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("TagsContainer"));
	TagsContainer->SetInnerSlotPadding(FVector2D(2.0f, 2.0f));
	UVerticalBoxSlot* TagsSlot = ScrollContent->AddChildToVerticalBox(TagsContainer);
	TagsSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 12.0f));

	// --- [C] カラーセクション ---
	ColorSectionBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ColorSectionBox"));
	ScrollContent->AddChildToVerticalBox(ColorSectionBox);

	// "Color" ラベル
	UTextBlock* ColorSectionLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ColorSectionLabel"));
	ColorSectionLabel->SetText(USortedStationsBPLibrary::GetVanillaString(TEXT("Buildables_UI"), TEXT("Lighting/LightControlPanel/Label/Preview/Color"), NSLOCTEXT("SortedStations", "Color", "Color")));
	SortedStationsDialogStyle::SetupLabel(ColorSectionLabel, 10.0f);
	ColorSectionBox->AddChildToVerticalBox(ColorSectionLabel);

	// カラー行（色見本 | Hex | スペーサー | Changeボタン | Resetボタン）
	UHorizontalBox* ColorRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ColorRow"));
	UVerticalBoxSlot* ColorRowSlot = ColorSectionBox->AddChildToVerticalBox(ColorRow);
	ColorRowSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 4.0f));

	// 色見本（固定サイズ 24x24）
	USizeBox* PreviewSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	PreviewSizeBox->SetWidthOverride(24.0f);
	PreviewSizeBox->SetHeightOverride(24.0f);
	ColorPreviewBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ColorPreviewBorder"));
	ColorPreviewBorder->SetBrushColor(FLinearColor::White);
	PreviewSizeBox->AddChild(ColorPreviewBorder);
	UHorizontalBoxSlot* PrevSlot = ColorRow->AddChildToHorizontalBox(PreviewSizeBox);
	PrevSlot->SetVerticalAlignment(VAlign_Center);
	PrevSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));

	// Hex表示
	ColorHexText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ColorHexText"));
	ColorHexText->SetText(FText::FromString(TEXT("#FFFFFF")));
	SortedStationsDialogStyle::SetupLabel(ColorHexText, 10.0f);
	UHorizontalBoxSlot* HexSlot = ColorRow->AddChildToHorizontalBox(ColorHexText);
	HexSlot->SetVerticalAlignment(VAlign_Center);
	HexSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));

	// スペーサー
	USpacer* ColorSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	UHorizontalBoxSlot* ColorSpacerSlot = ColorRow->AddChildToHorizontalBox(ColorSpacer);
	ColorSpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// Changeボタン
	ColorSelectButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ColorSelectButton"));
	ColorSelectButton->SetStyle(SmallBtnStyle);
	UTextBlock* SelectText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	SelectText->SetText(USortedStationsBPLibrary::GetVanillaString(TEXT("Menus_UI"), TEXT("Options/OptionsColor/Edit/Button"), NSLOCTEXT("SortedStations", "Change", "Change...")));
	SelectText->SetFont(SmallFont);
	SelectText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ColorSelectButton->AddChild(SelectText);
	UHorizontalBoxSlot* SelectSlot = ColorRow->AddChildToHorizontalBox(ColorSelectButton);
	SelectSlot->SetVerticalAlignment(VAlign_Center);

	// Resetボタン
	ResetColorButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ResetColorButton"));
	ResetColorButton->SetStyle(SmallBtnStyle);
	UTextBlock* ResetText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ResetText->SetText(USortedStationsBPLibrary::GetVanillaString(TEXT("Menus_UI"), TEXT("Options/OptionsColor/Reset/Button"), NSLOCTEXT("SortedStations", "Reset", "Reset")));
	ResetText->SetFont(SmallFont);
	ResetText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ResetColorButton->AddChild(ResetText);
	UHorizontalBoxSlot* ResetSlot = ColorRow->AddChildToHorizontalBox(ResetColorButton);
	ResetSlot->SetPadding(FMargin(6.0f, 0.0f, 0.0f, 0.0f));
	ResetSlot->SetVerticalAlignment(VAlign_Center);

	// ===================================================
	// 4. フッター（Cancel + Confirm）
	// ===================================================
	MakeSep(MainLayout, 10.0f, 10.0f);

	UHorizontalBox* FooterBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("FooterBox"));
	MainLayout->AddChildToVerticalBox(FooterBox);

	USpacer* FootSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	UHorizontalBoxSlot* FootSpacerSlot = FooterBox->AddChildToHorizontalBox(FootSpacer);
	FootSpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	CancelButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CancelButton"));
	CancelButton->SetStyle(BtnStyle);
	UTextBlock* CancelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CancelText->SetText(USortedStationsBPLibrary::GetVanillaString(TEXT("General_UI"), TEXT("GenericUIStrings/Cancel/Button"), NSLOCTEXT("SortedStations", "Cancel", "Cancel")));
	SortedStationsDialogStyle::SetupLabel(CancelText, 11.0f);
	CancelButton->AddChild(CancelText);
	FooterBox->AddChildToHorizontalBox(CancelButton);

	ConfirmButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ConfirmButton"));
	ConfirmButton->SetStyle(BtnStyle);
	UTextBlock* ConfirmText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ConfirmText->SetText(USortedStationsBPLibrary::GetVanillaString(TEXT("Buildables_UI"), TEXT("BlueprintMenu/Category/Edit/Apply/Button"), NSLOCTEXT("SortedStations", "Apply", "Apply")));
	SortedStationsDialogStyle::SetupLabel(ConfirmText, 11.0f);
	ConfirmButton->AddChild(ConfirmText);
	UHorizontalBoxSlot* ConfSlot = FooterBox->AddChildToHorizontalBox(ConfirmButton);
	ConfSlot->SetPadding(FMargin(10.0f, 0.0f, 0.0f, 0.0f));
}
