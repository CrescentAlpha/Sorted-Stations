#include "UI/SortedStationsResetConfigWidget.h"
#include "SortedStationsBPLibrary.h"
#include "SortedStations.h"

#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Spacer.h"
#include "Components/PanelWidget.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"
#include "FGActorRepresentation.h"
#include "FGActorRepresentationManager.h"
#include "Configuration/Properties/ConfigPropertySection.h"
#include "UObject/UObjectIterator.h"

#define LOCTEXT_NAMESPACE "SortedStationsResetConfigWidget"

namespace
{

	FButtonStyle MakeActionButtonStyle(const FLinearColor& NormalColor, const FLinearColor& HoverColor, const FLinearColor& PressColor)
	{
		FButtonStyle Style = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
		Style.SetNormal(FSlateColorBrush(NormalColor));
		Style.SetHovered(FSlateColorBrush(HoverColor));
		Style.SetPressed(FSlateColorBrush(PressColor));
		Style.NormalPadding = FMargin(14.0f, 6.0f);
		Style.PressedPadding = FMargin(14.0f, 7.0f, 14.0f, 5.0f);
		return Style;
	}
}

TSharedRef<SWidget> USortedStationsResetConfigWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		BuildDefaultWidgetTree();
	}
	return Super::RebuildWidget();
}

void USortedStationsResetConfigWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (TriggerButton && !TriggerButton->OnClicked.IsBound())
	{
		TriggerButton->OnClicked.AddDynamic(this, &USortedStationsResetConfigWidget::OnTriggerClicked);
	}
	if (ConfirmButton && !ConfirmButton->OnClicked.IsBound())
	{
		ConfirmButton->OnClicked.AddDynamic(this, &USortedStationsResetConfigWidget::OnConfirmClicked);
	}
	if (CancelButton && !CancelButton->OnClicked.IsBound())
	{
		CancelButton->OnClicked.AddDynamic(this, &USortedStationsResetConfigWidget::OnCancelClicked);
	}

	ResetToDefaultState();
	FSortedStationsModule::ApplyVanillaLocalizationToConfig();
}

void USortedStationsResetConfigWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}

void USortedStationsResetConfigWidget::NativeDestruct()
{
	ResetToDefaultState();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FeedbackTimerHandle);
	}

	Super::NativeDestruct();
}

void USortedStationsResetConfigWidget::ResetToDefaultState()
{
	bIsConfirmOpen = false;
	if (ConfirmBorder)
	{
		ConfirmBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (FeedbackText)
	{
		FeedbackText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void USortedStationsResetConfigWidget::OnTriggerClicked()
{
	bIsConfirmOpen = !bIsConfirmOpen;
	if (ConfirmBorder)
	{
		ConfirmBorder->SetVisibility(bIsConfirmOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void USortedStationsResetConfigWidget::OnConfirmClicked()
{
	USortedStationsBPLibrary::ResetAllModSettings();
	USortedStationsBPLibrary::ResetAllVehicleColorsInWorld(this);
	FSortedStationsModule::RequestImmediateMapSort(this);

	bIsConfirmOpen = false;
	if (ConfirmBorder)
	{
		ConfirmBorder->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (FeedbackText)
	{
		FeedbackText->SetVisibility(ESlateVisibility::Visible);

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(FeedbackTimerHandle);
			World->GetTimerManager().SetTimer(FeedbackTimerHandle, [this]()
			{
				if (FeedbackText)
				{
					FeedbackText->SetVisibility(ESlateVisibility::Collapsed);
				}
			}, 3.0f, false);
		}
	}
}

void USortedStationsResetConfigWidget::OnCancelClicked()
{
	ResetToDefaultState();
}

void USortedStationsResetConfigWidget::BuildDefaultWidgetTree()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	// バニラ公式ローカライズテキストの取得（言語ハードコード完全排除）
	const FText LabelResetButton = USortedStationsBPLibrary::GetVanillaString(TEXT("Menus_UI"), TEXT("Options/OptionsColor/Reset/Button"), LOCTEXT("ResetBtn", "Reset"));
	const FText LabelCategory    = USortedStationsBPLibrary::GetVanillaString(TEXT("Menus_UI"), TEXT("Options/OptionsColor/Reset/aTitle"), LOCTEXT("ResetColor", "Reset Color"));
	const FText BodyConfirm      = USortedStationsBPLibrary::GetVanillaString(TEXT("Menus_UI"), TEXT("Options/OptionsColor/Reset/Body"), LOCTEXT("ResetBody", "Are you sure you wish to reset this color to its default setting?"));
	const FText LabelConfirm     = USortedStationsBPLibrary::GetVanillaString(TEXT("General_UI"), TEXT("GenericUIStrings/Confirm/Button"), LOCTEXT("Confirm", "Confirm"));
	const FText LabelCancel      = USortedStationsBPLibrary::GetVanillaString(TEXT("General_UI"), TEXT("GenericUIStrings/Cancel/Button"), LOCTEXT("Cancel", "Cancel"));

	FSlateFontInfo FontNormal = FCoreStyle::Get().GetFontStyle("NormalFont");
	FontNormal.Size = 12;

	FSlateFontInfo FontBody = FontNormal;
	FontBody.Size = 11;

	// ボタンスタイル
	const FButtonStyle TriggerBtnStyle = MakeActionButtonStyle(
		FLinearColor(0.20f, 0.23f, 0.28f, 0.95f),
		FLinearColor(0.28f, 0.32f, 0.40f, 1.0f),
		FLinearColor(0.14f, 0.16f, 0.20f, 1.0f)
	);

	const FButtonStyle ConfirmBtnStyle = MakeActionButtonStyle(
		FLinearColor(0.75f, 0.15f, 0.12f, 0.95f),
		FLinearColor(0.90f, 0.22f, 0.18f, 1.0f),
		FLinearColor(0.55f, 0.10f, 0.08f, 1.0f)
	);

	const FButtonStyle CancelBtnStyle = MakeActionButtonStyle(
		FLinearColor(0.22f, 0.24f, 0.28f, 0.90f),
		FLinearColor(0.32f, 0.35f, 0.40f, 1.0f),
		FLinearColor(0.15f, 0.16f, 0.20f, 1.0f)
	);

	// ルートコンテナ
	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootBox"));
	WidgetTree->RootWidget = RootBox;

	// 1. トリガー行（リセットボタン ＋ 補足説明 ＋ 完了フィードバック）
	UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeaderRow"));
	UVerticalBoxSlot* HeaderRowSlot = RootBox->AddChildToVerticalBox(HeaderRow);
	HeaderRowSlot->SetPadding(FMargin(0.0f, 16.0f, 0.0f, 6.0f));

	// トリガーボタン
	TriggerButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("TriggerButton"));
	TriggerButton->SetStyle(TriggerBtnStyle);

	UTextBlock* TriggerBtnLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TriggerBtnLabel"));
	TriggerBtnLabel->SetText(LabelResetButton);
	TriggerBtnLabel->SetFont(FontNormal);
	TriggerBtnLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	TriggerButton->AddChild(TriggerBtnLabel);

	UHorizontalBoxSlot* TriggerSlot = HeaderRow->AddChildToHorizontalBox(TriggerButton);
	TriggerSlot->SetVerticalAlignment(VAlign_Center);

	// 補足説明テキスト（ボタンの右側に配置、右下のソート用「デフォルトに戻す」と明確に差別化）
	UTextBlock* DescLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DescLabel"));
	const FText DescText = FText::Format(FText::FromString(TEXT("({0})")), LabelCategory);
	DescLabel->SetText(DescText);
	DescLabel->SetFont(FontBody);
	DescLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.70f, 0.75f, 0.82f, 1.0f)));

	UHorizontalBoxSlot* DescSlot = HeaderRow->AddChildToHorizontalBox(DescLabel);
	DescSlot->SetPadding(FMargin(10.0f, 0.0f, 0.0f, 0.0f));
	DescSlot->SetVerticalAlignment(VAlign_Center);

	// 完了フィードバックテキスト
	FeedbackText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FeedbackText"));
	const FText CompletedText = USortedStationsBPLibrary::GetVanillaString(TEXT("Buildables_UI"), TEXT("SpaceElevator/Status/Completed"), FText::FromString(TEXT("Completed")));
	FeedbackText->SetText(FText::Format(FText::FromString(TEXT("\u2713 {0}")), CompletedText));
	FeedbackText->SetFont(FontNormal);
	FeedbackText->SetColorAndOpacity(FSlateColor(FLinearColor(0.20f, 0.90f, 0.35f, 1.0f)));
	FeedbackText->SetVisibility(ESlateVisibility::Collapsed);

	UHorizontalBoxSlot* FeedSlot = HeaderRow->AddChildToHorizontalBox(FeedbackText);
	FeedSlot->SetPadding(FMargin(16.0f, 0.0f, 0.0f, 0.0f));
	FeedSlot->SetVerticalAlignment(VAlign_Center);

	// 2. 確認ダイアログパネル（ボタン押下時のみ展開）
	ConfirmBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ConfirmBorder"));
	ConfirmBorder->SetBrushColor(FLinearColor(0.12f, 0.04f, 0.04f, 0.95f));
	ConfirmBorder->SetPadding(FMargin(16.0f, 12.0f));
	ConfirmBorder->SetVisibility(ESlateVisibility::Collapsed);

	UVerticalBoxSlot* BorderSlot = RootBox->AddChildToVerticalBox(ConfirmBorder);
	BorderSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 8.0f));

	UVerticalBox* ConfirmContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ConfirmContentBox"));
	ConfirmBorder->AddChild(ConfirmContentBox);

	// 警告本文
	UTextBlock* BodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BodyText"));
	BodyText->SetText(BodyConfirm);
	BodyText->SetFont(FontBody);
	BodyText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.85f, 0.85f, 1.0f)));
	BodyText->SetAutoWrapText(true);

	UVerticalBoxSlot* BodySlot = ConfirmContentBox->AddChildToVerticalBox(BodyText);
	BodySlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));

	// アクションボタン行（決定 / キャンセル）
	UHorizontalBox* ActionRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ActionRow"));
	ConfirmContentBox->AddChildToVerticalBox(ActionRow);

	// 決定ボタン
	ConfirmButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ConfirmButton"));
	ConfirmButton->SetStyle(ConfirmBtnStyle);

	UTextBlock* ConfLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ConfLabel"));
	ConfLabel->SetText(LabelConfirm);
	ConfLabel->SetFont(FontBody);
	ConfLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ConfirmButton->AddChild(ConfLabel);

	UHorizontalBoxSlot* ConfSlot = ActionRow->AddChildToHorizontalBox(ConfirmButton);
	ConfSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));

	// キャンセルボタン
	CancelButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CancelButton"));
	CancelButton->SetStyle(CancelBtnStyle);

	UTextBlock* CancelLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CancelLabel"));
	CancelLabel->SetText(LabelCancel);
	CancelLabel->SetFont(FontBody);
	CancelLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	CancelButton->AddChild(CancelLabel);

	ActionRow->AddChildToHorizontalBox(CancelButton);
}

#undef LOCTEXT_NAMESPACE
