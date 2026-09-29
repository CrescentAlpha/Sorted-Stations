#include "UI/SortedStationsRenameButton.h"
#include "UI/SortedStationsVehicleRenameDialog.h"
#include "SortedStationsBPLibrary.h"
#include "SortedStations.h"
#include "FGActorRepresentation.h"
#include "Components/Button.h"
#include "Blueprint/WidgetTree.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

TSharedRef<SWidget> USortedStationsRenameButton::RebuildWidget()
{
	// ===================================================================
	// ✎ 文字グリフによるアイコン（テクスチャ不要・Shipping完全対応）
	// Noto Sans等のゲーム内フォントに含まれる鉛筆グリフ U+270E を使用
	// ===================================================================

	// ボタン背景スタイル
	// 通常時: 半透明ダークグレー / ホバー時: FICSITオレンジ
	static FSlateColorBrush NormalBgBrush(FLinearColor(0.10f, 0.10f, 0.12f, 0.6f));
	static FSlateColorBrush HoveredBgBrush(FLinearColor(0.95f, 0.45f, 0.05f, 0.90f));
	static FSlateColorBrush PressedBgBrush(FLinearColor(0.75f, 0.32f, 0.04f, 1.0f));

	static FButtonStyle ButtonStyle;
	static bool bStyleInitialized = false;
	if (!bStyleInitialized)
	{
		ButtonStyle = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
		ButtonStyle.SetNormal(NormalBgBrush);
		ButtonStyle.SetHovered(HoveredBgBrush);
		ButtonStyle.SetPressed(PressedBgBrush);
		ButtonStyle.SetDisabled(NormalBgBrush);
		// ボタン内余白を最小化（アイコン専用ボタン）
		ButtonStyle.NormalPadding  = FMargin(0.0f);
		ButtonStyle.PressedPadding = FMargin(0.0f, 1.0f, 0.0f, -1.0f);
		bStyleInitialized = true;
	}

	// フォントスタイル（ゲームデフォルトフォントを使用し、グリフの存在に依存）
	static FSlateFontInfo IconFont;
	static bool bFontInitialized = false;
	if (!bFontInitialized)
	{
		// まずSatisfactory UIフォントを試みる（Noto Sansはグリフが豊富）
		IconFont = FCoreStyle::Get().GetFontStyle("NormalFont");
		IconFont.Size = 12;
		bFontInitialized = true;
	}

	TSharedRef<SWidget> Content =
		SNew(SBox)
		.WidthOverride(24.0f)
		.HeightOverride(24.0f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Visibility_UObject(this, &USortedStationsRenameButton::GetButtonVisibility)
		[
			SNew(SButton)
			.ButtonStyle(&ButtonStyle)
			// バニラ公式StringTableキー "Edit" を使用（言語設定に応じて日本語「編集」等に自動翻訳）
			.ToolTipText(FText::FromStringTable(TEXT("Menus_UI"), TEXT("Options/OptionsColor/Edit/Button")))
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.ContentPadding(FMargin(0.0f))
			.OnClicked_Lambda([this]() -> FReply
			{
				OnButtonClicked();
				return FReply::Handled();
			})
			[
				// ✎ (U+270E) LOWER RIGHT PENCIL — 鉛筆アイコン
				// フォントにない場合は □ や ? で表示される（許容範囲）
				// より安全な代替: ✏ (U+270F) / ✍ (U+270D)
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("\u270E")))
				.Font(IconFont)
				.ColorAndOpacity(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.95f)))
				.Justification(ETextJustify::Center)
			]
		];

	return Content;
}

void USortedStationsRenameButton::NativeConstruct()
{
	Super::NativeConstruct();

	// ウィジェット自体は HitTest を通すが描画は子に任せる
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	// 親コンテナ（mCustomButtonsContainer）を確実に可視化
	if (UWidget* ParentWidget = GetParent())
	{
		ParentWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	ResolveTargetRepresentation();

	if (EditButton)
	{
		EditButton->OnClicked.AddDynamic(this, &USortedStationsRenameButton::OnButtonClicked);
	}

	// 親のBPW_MapFilterButton内の行ボタン（mButton）のOnUnhoveredにバインドし、
	// マウスが離れた際にも親コンテナ（mCustomButtonsContainer）の可視性を即座に維持する
	if (UUserWidget* ParentUW = GetTypedOuter<UUserWidget>())
	{
		if (FObjectProperty* BtnProp = FindFProperty<FObjectProperty>(ParentUW->GetClass(), TEXT("mButton")))
		{
			if (UButton* RowBtn = Cast<UButton>(BtnProp->GetObjectPropertyValue_InContainer(ParentUW)))
			{
				RowBtn->OnUnhovered.AddDynamic(this, &USortedStationsRenameButton::OnParentUnhovered);
			}
		}
	}

	// ウィジェット構築時に一度だけ親コンテナの可視性を設定（Slateループ中ではなく初期化時のみ）
	if (UWidget* ParentWidget = GetParent())
	{
		ParentWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	// マップ画面（フィルターボタン）構築時に1回だけ全体ソートをリクエスト（1秒以内の重複呼び出しは安全に抑制）
	static double LastMapSortTime = 0.0;
	const double CurrentTime = FPlatformTime::Seconds();
	if (CurrentTime - LastMapSortTime > 1.0)
	{
		LastMapSortTime = CurrentTime;
		FSortedStationsModule::RequestImmediateMapSort(this);
	}

	if (TargetRepresentation)
	{
		FSortedStationsModule::ApplySavedVehicleColorIfAvailable(TargetRepresentation);
	}

	// ウィジェットのTick停止に左右されないワールドタイマーで親コンテナの可視性を強制維持
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			VisibilityLoopTimerHandle,
			FTimerDelegate::CreateUObject(this, &USortedStationsRenameButton::ForceMaintainParentVisibility),
			0.05f,
			true
		);
	}
}

void USortedStationsRenameButton::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Tick内でも親コンテナの可視性を復元
	ForceMaintainParentVisibility();
}

void USortedStationsRenameButton::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(VisibilityLoopTimerHandle);
		World->GetTimerManager().ClearTimer(VisibilityMaintainTimerHandle);
	}

	if (EditButton)
	{
		EditButton->OnClicked.RemoveAll(this);
	}

	// 開いているダイアログがあれば道連れ消去
	if (SpawnedDialog.IsValid())
	{
		SpawnedDialog->RemoveFromParent();
		SpawnedDialog.Reset();
	}

	Super::NativeDestruct();
}

void USortedStationsRenameButton::OnParentUnhovered()
{
	// バニラBlueprintがOnUnhovered後に複数フレーム可視性を上書きする可能性があるため、
	// 短いインターバルで繰り返し可視性を強制維持する（最大5回、0.05秒間隔）
	MaintainParentVisibility();

	if (UWorld* World = GetWorld())
	{
		// 既存タイマーをキャンセルして新規開始
		World->GetTimerManager().ClearTimer(VisibilityMaintainTimerHandle);

		struct FVisibilityGuard
		{
			TWeakObjectPtr<USortedStationsRenameButton> WeakThis;
			int32 RemainingCount;
		};
		TSharedRef<FVisibilityGuard> Guard = MakeShared<FVisibilityGuard>();
		Guard->WeakThis = this;
		Guard->RemainingCount = 5;

		// ラムダをFTimerDelegateに包む（カウント分だけ繰り返す）
		FTimerDelegate Del;
		// 単純にNextTick×3でカバー（SetTimerForNextTickを3回連鎖）
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([this, World]()
		{
			MaintainParentVisibility();
			if (World && IsValid(this))
			{
				World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([this, World]()
				{
					MaintainParentVisibility();
					if (World && IsValid(this))
					{
						World->GetTimerManager().SetTimerForNextTick(
							FTimerDelegate::CreateUObject(this, &USortedStationsRenameButton::MaintainParentVisibility));
					}
				}));
			}
		}));
	}
}

void USortedStationsRenameButton::MaintainParentVisibility()
{
	if (UWidget* ParentWidget = GetParent())
	{
		ParentWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void USortedStationsRenameButton::ForceMaintainParentVisibility()
{
	if (!TargetRepresentation)
	{
		ResolveTargetRepresentation();
	}
	if (TargetRepresentation && USortedStationsBPLibrary::IsSupportedRepresentationType(TargetRepresentation->GetRepresentationType()))
	{
		if (UWidget* ParentWidget = GetParent())
		{
			if (ParentWidget->GetVisibility() != ESlateVisibility::SelfHitTestInvisible)
			{
				ParentWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
		}
	}
}

EVisibility USortedStationsRenameButton::GetButtonVisibility() const
{
	if (!TargetRepresentation)
	{
		const_cast<USortedStationsRenameButton*>(this)->ResolveTargetRepresentation();
	}

	if (TargetRepresentation && USortedStationsBPLibrary::IsSupportedRepresentationType(TargetRepresentation->GetRepresentationType()))
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}

void USortedStationsRenameButton::ResolveTargetRepresentation()
{
	// 1. GetTypedOuter<UUserWidget>() で親の BPW_MapFilterButton を直接取得
	UUserWidget* ParentUserWidget = GetTypedOuter<UUserWidget>();

	// 2. フォールバック: WidgetTree 経由
	if (!ParentUserWidget)
	{
		if (UWidgetTree* Tree = GetTypedOuter<UWidgetTree>())
		{
			ParentUserWidget = Tree->GetTypedOuter<UUserWidget>();
		}
	}

	// 3. フォールバック: Outerチェーンを段階的に辿る
	if (!ParentUserWidget)
	{
		UObject* CurrentOuter = GetOuter();
		while (CurrentOuter)
		{
			if (UUserWidget* UW = Cast<UUserWidget>(CurrentOuter))
			{
				ParentUserWidget = UW;
				break;
			}
			CurrentOuter = CurrentOuter->GetOuter();
		}
	}

	if (ParentUserWidget)
	{
		// BPW_MapFilterButton のプロパティ mActorRepresentation を取得
		if (FObjectProperty* RepProp = FindFProperty<FObjectProperty>(ParentUserWidget->GetClass(), TEXT("mActorRepresentation")))
		{
			if (UFGActorRepresentation* Rep = Cast<UFGActorRepresentation>(RepProp->GetObjectPropertyValue_InContainer(ParentUserWidget)))
			{
				SetTargetRepresentation(Rep);
				return;
			}
		}

		// フォールバックプロパティ mRepresentation
		if (FObjectProperty* RepProp = FindFProperty<FObjectProperty>(ParentUserWidget->GetClass(), TEXT("mRepresentation")))
		{
			if (UFGActorRepresentation* Rep = Cast<UFGActorRepresentation>(RepProp->GetObjectPropertyValue_InContainer(ParentUserWidget)))
			{
				SetTargetRepresentation(Rep);
				return;
			}
		}

		UE_LOG(LogSortedStations, Warning,
			TEXT("RenameButton: Could not find mActorRepresentation on parent widget '%s'. "
			     "Button will remain hidden for non-vehicle entries."),
			*ParentUserWidget->GetClass()->GetName());
	}
}

void USortedStationsRenameButton::SetTargetRepresentation(UFGActorRepresentation* InRepresentation)
{
	TargetRepresentation = InRepresentation;
	if (TargetRepresentation)
	{
		FSortedStationsModule::ApplySavedVehicleColorIfAvailable(TargetRepresentation);
		UE_LOG(LogSortedStations, Log, TEXT("RenameButton bound to Representation: %s (Type: %d)"),
			*TargetRepresentation->GetRepresentationText().ToString(), (int32)TargetRepresentation->GetRepresentationType());
	}
}

void USortedStationsRenameButton::OnButtonClicked()
{
	if (!TargetRepresentation)
	{
		ResolveTargetRepresentation();
	}

	if (!TargetRepresentation)
	{
		UE_LOG(LogSortedStations, Warning, TEXT("RenameButton clicked but TargetRepresentation is null."));
		return;
	}

	// ダイアログを作成してViewportに表示（ZOrder 100 で最前面）
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	}

	if (PC)
	{
		USortedStationsVehicleRenameDialog* Dialog =
			CreateWidget<USortedStationsVehicleRenameDialog>(PC, USortedStationsVehicleRenameDialog::StaticClass());
		if (Dialog)
		{
			Dialog->SetParentButton(this);
			SpawnedDialog = Dialog;

			Dialog->InitDialog(TargetRepresentation);
			Dialog->AddToViewport(100);
			UE_LOG(LogSortedStations, Log, TEXT("RenameDialog opened for: %s"),
				*TargetRepresentation->GetRepresentationText().ToString());
		}
	}
}
