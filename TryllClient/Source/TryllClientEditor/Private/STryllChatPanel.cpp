// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "STryllChatPanel.h"
#include "TryllVoiceProfile.h"

#include "TryllRuntimeSettings.h"
#include "TryllWorkflowAsset.h"
#include "TryllAgentComponent.h"
#include "TryllVoiceInputComponent.h"
#include "TryllGraphDescription.h"
#include "TryllVariableOverrideUtils.h"
#include "TryllModelDownloadGuard.h"
#include "TryllToolStateReset.h"
#include "TryllClientEditorModule.h"
#include "TryllDialogLabCompareFromChat.h"
#include "TryllChatConfig.h"
#include "TryllChatTurnState.h"
#include "TryllTurnInspectorModels.h"
#include "STryllTurnInspectorView.h"
#include "UI/STryllInspectorSplit.h"
#include "UI/STryllServerModeBadge.h"
#include "UI/STryllStatusBar.h"
#include "UI/STryllBanner.h"
#include "UI/TryllEditorStyle.h"
#include "UI/TryllTailFollow.h"
#include "TryllStatusPresentation.h"
#include "TryllEditorUiContract.h"
#include "TryllModelManifest.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Types/ISlateMetaData.h"
#include "Styling/CoreStyle.h"
#include "TryllSubsystem.h"
#include "TryllEditorSubsystem.h"
#include "TryllEditorConnectionPolicy.h"
#include "TryllAgent.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Selection.h"

#include "AudioCaptureCore.h"
#include "TryllVoiceInput.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "STryllChatPanel"

/** Microphone capture: filled on the audio thread, drained on the game thread. */
struct FTryllChatMicCapture
{
	Audio::FAudioCapture Capture;
	FCriticalSection     BufferLock;
	TArray<int16>        PendingPcm;
};

namespace
{
	uint32 ResolveCaptureRate()
	{
		Audio::FAudioCapture Probe;
		Audio::FCaptureDeviceInfo Info;
		if (Probe.GetCaptureDeviceInfo(Info) && Info.PreferredSampleRate > 0)
		{
			return static_cast<uint32>(Info.PreferredSampleRate);
		}
		return 48000;
	}
}

// ── Construction ──────────────────────────────────────────────────────────────

STryllChatPanel::STryllChatPanel() = default;

void STryllChatPanel::Construct(const FArguments& InArgs)
{
	EditorSubsystemOverride = InArgs._EditorSubsystemOverride;
	bPersistLayout = InArgs._PersistLayout;
	AutofindSettings();
	if (bPersistLayout)
	{
		LoadChatConfig();
	}
	RefreshAgentCandidates();
	if (UTryllAgentComponent* Pinned = InArgs._AgentComponentOverride)
	{
		SelectActor(Pinned->GetOwner());
		SelectComponent(Pinned);
	}
	else
	{
		SyncSelectionFromEditor();
	}

	if (GEditor)
	{
		MapChangeHandle = FEditorDelegates::MapChange.AddLambda([this](uint32)
		{
			RefreshAgentCandidates();
			if (!IsStartable(State) && !SelectedComponent.IsValid())
			{
				Stop();
				Fail(TEXT("Selected agent component was removed (level change)"));
			}
		});
		if (GEngine)
		{
			ActorDeletedHandle = GEngine->OnLevelActorDeleted().AddLambda([this](AActor* Actor)
			{
				if (SelectedActor.Get() == Actor ||
					(SelectedComponent.IsValid() && SelectedComponent->GetOwner() == Actor))
				{
					SelectedComponent.Reset();
					SelectedActor.Reset();
					SelectedVoiceComponent.Reset();
					if (!IsStartable(State))
					{
						Stop();
						Fail(TEXT("Selected agent component was destroyed"));
					}
				}
				RefreshAgentCandidates();
			});
		}
	}

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(8,8,8,4)[ BuildConfigRow() ]
		+ SVerticalBox::Slot().AutoHeight().Padding(8,4)[ BuildVoiceRow() ]
		+ SVerticalBox::Slot().AutoHeight().Padding(8,4)[ BuildStartStopRow() ]
		+ SVerticalBox::Slot().AutoHeight().Padding(8,0,8,0)
		[
			SAssignNew(Banner, STryllBanner)
		]
		+ SVerticalBox::Slot().AutoHeight()[ SNew(SSeparator) ]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(8,4)[ BuildChatArea() ]
		+ SVerticalBox::Slot().AutoHeight().Padding(8,4,8,8)[ BuildInputRow() ]
	];

	ChatTail = FTryllTailFollow(
		[this] { return MeasureChatDistance(); },
		[this] { ScrollChatToEnd(); });

	RefreshInspector();
	HostColorsHandle = FTryllEditorStyle::OnHostColorsRefreshed.AddRaw(
		this, &STryllChatPanel::HandleHostColorsRefreshed);
}

void STryllChatPanel::HandleHostColorsRefreshed()
{
	RebuildChatScroll();
	RefreshInspector();
	RefreshStatusChrome();
}

STryllChatPanel::~STryllChatPanel()
{
	if (HostColorsHandle.IsValid())
	{
		FTryllEditorStyle::OnHostColorsRefreshed.Remove(HostColorsHandle);
		HostColorsHandle.Reset();
	}
	if (State != EState::Idle)
	{
		Stop(false);
	}
	if (MapChangeHandle.IsValid())
	{
		FEditorDelegates::MapChange.Remove(MapChangeHandle);
		MapChangeHandle = FDelegateHandle();
	}
	if (ActorDeletedHandle.IsValid() && GEngine)
	{
		GEngine->OnLevelActorDeleted().Remove(ActorDeletedHandle);
		ActorDeletedHandle = FDelegateHandle();
	}
}

// ── Layout ────────────────────────────────────────────────────────────────────

TSharedRef<SWidget> STryllChatPanel::BuildConfigRow()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0,2)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SettingsHint",
				"Settings: Edit → Project Settings → Plugins → Tryll Client"))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0,2)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("AgentHint",
				"Select a scene UTryllAgentComponent. Chat creates an editor agent from that "
				"component's graph, variables, and overrides; Details edits apply live via a preview link."))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			.AutoWrapText(true)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0,4)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,8,0)
			[ SNew(STextBlock).Text(LOCTEXT("ActorLabel", "Actor:")).MinDesiredWidth(110.f) ]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[ BuildActorPicker() ]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0,4)
		[
			SNew(SHorizontalBox)
			.Visibility_Lambda([this]
			{
				return CandidateComponents.Num() > 1 ? EVisibility::Visible : EVisibility::Collapsed;
			})
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,8,0)
			[ SNew(STextBlock).Text(LOCTEXT("ComponentLabel", "Agent Component:")).MinDesiredWidth(110.f) ]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[ BuildComponentPicker() ]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0,2)
		[
			SNew(STextBlock)
			.Text_Lambda([this]
			{
				if (!SelectedComponent.IsValid())
				{
					return LOCTEXT("NoComponent", "No agent component selected.");
				}
				UTryllAgentComponent* Comp = SelectedComponent.Get();
				const FString GraphSource = Comp->WorkflowAsset
					? Comp->WorkflowAsset->GetName()
					: FString(TEXT("(no workflow asset)"));
				return FText::FromString(FString::Printf(
					TEXT("Selected: %s → %s"),
					*GetSelectedComponentLabel().ToString(), *GraphSource));
			})
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];
}

TSharedRef<SWidget> STryllChatPanel::BuildVoiceRow()
{
	RebuildSttModelOptions();

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0,2)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,8,0)
			[
				SNew(STextBlock).Text(LOCTEXT("VoiceInputLabel", "Voice Input:")).MinDesiredWidth(110.f)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(SComboButton)
				.IsEnabled_Lambda([this]{ return IsStartable(State); })
				.OnGetMenuContent(FOnGetContent::CreateSP(this, &STryllChatPanel::BuildVoiceSourceMenu))
				.ToolTipText(this, &STryllChatPanel::GetVoiceSourceTooltip)
				.ButtonContent()
				[
					SNew(STextBlock).Text(this, &STryllChatPanel::GetVoiceSourceLabel)
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0,4)
		[
			SNew(SHorizontalBox)
			.Visibility_Lambda([this]
			{
				return VoiceSource == ETryllChatVoiceSource::Chat
					? EVisibility::Visible
					: EVisibility::Collapsed;
			})
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,8,0)
			[
				SNew(STextBlock).Text(LOCTEXT("SttModelLabel", "STT Model:")).MinDesiredWidth(110.f)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&SttModelOptions)
				.OnComboBoxOpening_Lambda([this]{ RebuildSttModelOptions(); })
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
				{
					const FString Name = InItem.IsValid() ? *InItem : FString();
					return SNew(STextBlock).Text(FText::FromString(Name));
				})
				.OnSelectionChanged_Lambda([this](TSharedPtr<FString> InItem, ESelectInfo::Type)
				{
					if (!InItem.IsValid())
					{
						return;
					}
					SttModelName = *InItem;
					SaveChatConfig();
				})
				.IsEnabled_Lambda([this]{ return IsStartable(State); })
				[
					SNew(STextBlock).Text_Lambda([this]{ return FText::FromString(SttModelName); })
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0,4)
		[
			SNew(SHorizontalBox)
			.Visibility_Lambda([this]
			{
				return VoiceSource == ETryllChatVoiceSource::Component
					&& CandidateVoiceComponents.Num() > 1
					? EVisibility::Visible
					: EVisibility::Collapsed;
			})
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,8,0)
			[
				SNew(STextBlock).Text(LOCTEXT("VoiceCompLabel", "Voice Component:")).MinDesiredWidth(110.f)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				BuildVoiceComponentPicker()
			]
		];
}

TSharedRef<SWidget> STryllChatPanel::BuildStartStopRow()
{
	// Match Unity TryllChatWindow toolbar: Start/Stop + status on the left;
	// Compare in Lab + Inspector on the right (spacer takes the middle).
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)
		[
			SNew(SButton)
			.AddMetaData<FTagMetaData>(FTagMetaData(TryllEditorWidgetIds::ChatStart))
			.OnClicked(this, &STryllChatPanel::OnStartStopClicked)
			[
				SNew(STextBlock)
				.Text(this, &STryllChatPanel::GetStartStopLabel)
				.MinDesiredWidth(64.f)
				.Justification(ETextJustify::Center)
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(0,0,8,0)
		[
			SAssignNew(StatusBar, STryllStatusBar)
			.OnAction_Lambda([this] { OnStartStopClicked(); })
		]
		+ SHorizontalBox::Slot().FillWidth(1.f)
		[
			SNullWidget::NullWidget
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,8,0)
		[
			SNew(STryllServerModeBadge)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)
		[
			SNew(SButton)
			.Text(LOCTEXT("CompareInLab", "Compare in Lab…"))
			.ToolTipText(LOCTEXT("CompareInLabTip",
				"Open the Dialog Lab on this agent with the chat so far as seeded history, plus one "
				"blank turn to generate"))
			.OnClicked(this, &STryllChatPanel::OnCompareInLabClicked)
			// Nothing to capture with no agent; an empty history still opens the Lab with a single
			// blank step, which is a legitimate way in.
			.IsEnabled_Lambda([this]{ return SelectedComponent.IsValid(); })
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SButton)
			.Text_Lambda([this]
			{
				return bInspectorVisible
					? LOCTEXT("InspectorHide", "Inspector ▶")
					: LOCTEXT("InspectorShow", "Inspector ◀");
			})
			.OnClicked_Lambda([this]
			{
				bInspectorVisible = !bInspectorVisible;
				SaveChatConfig();
				return FReply::Handled();
			})
		];
}

FReply STryllChatPanel::OnCompareInLabClicked()
{
	TArray<FTryllChatHistoryEntry> Captured;
	int32 SkippedEmptyTurns = 0;
	for (const FTryllChatTurnRecord& Turn : Turns)
	{
		if (!Turn.bComplete)
		{
			continue;
		}
		if (!FTryllChatTurnLogic::ShouldCaptureForCompare(Turn))
		{
			++SkippedEmptyTurns;
			continue;
		}
		Captured.Add({true, Turn.UserText});
		Captured.Add({false, Turn.AssistantText});
	}
	if (SkippedEmptyTurns > 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("STryllChatPanel: Compare in Lab skipped %d turn(s) with no assistant output"),
			SkippedEmptyTurns);
	}

	FModuleManager::GetModuleChecked<FTryllClientEditorModule>("TryllClientEditor")
		.OpenDialogLabFromChat(SelectedComponent.Get(), Captured);
	return FReply::Handled();
}

TSharedRef<SWidget> STryllChatPanel::BuildChatArea()
{
	return SNew(STryllInspectorSplit)
		.FixedSide(STryllInspectorSplit::EFixedSide::Second)
		.PrimaryMin(FTryllEditorStyle::ChatMinWidth)
		.SecondaryMin(FTryllEditorStyle::InspectorMinWidth)
		.StoredWidth_Lambda([this] { return InspectorWidth; })
		.OnStoredWidthChanged_Lambda([this](float NewWidth)
		{
			InspectorWidth = NewWidth;
			SaveChatConfig();
		})
		.IsFixedPaneVisible_Lambda([this] { return bInspectorVisible; })
		.PrimaryContent()
		[
			BuildChatScroll()
		]
		.SecondaryContent()
		[
			SAssignNew(InspectorView, STryllTurnInspectorView)
		];
}

TSharedRef<SWidget> STryllChatPanel::BuildChatScroll()
{
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(4)
		[
			SAssignNew(ChatScroll, SScrollBox)
			.OnUserScrolled_Lambda([this](float)
			{
				ChatTail.NotifyUserScroll();
			})
		];
}

TSharedRef<SWidget> STryllChatPanel::BuildInputRow()
{
	const auto VoiceReady = [this]
	{
		return VoiceSource != ETryllChatVoiceSource::Off
			&& State == EState::Ready
			&& ChatVoice.IsValid()
			&& !bTurnInFlight;
	};
	const auto VoiceVisible = [this]
	{
		return VoiceSource != ETryllChatVoiceSource::Off
			? EVisibility::Visible
			: EVisibility::Collapsed;
	};

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(0,0,4,0)
		[
			SNew(SButton)
			.Visibility_Lambda(VoiceVisible)
			.IsEnabled_Lambda(VoiceReady)
			.OnPressed(FSimpleDelegate::CreateSP(this, &STryllChatPanel::OnMicPressed))
			.OnReleased(FSimpleDelegate::CreateSP(this, &STryllChatPanel::OnMicReleased))
			.ButtonColorAndOpacity_Lambda([this]
			{
				return IsRecording()
					? FTryllEditorStyle::ColErr
					: FTryllEditorStyle::Panel2;
			})
			.ToolTipText_Lambda([this]
			{
				return VoiceMode == EVoiceMode::PushToTalk
					? LOCTEXT("MicTipPtt", "Push-to-talk: hold to record")
					: LOCTEXT("MicTipHf", "Hands-free: click to start/stop");
			})
			[
				SNew(STextBlock)
				.Text_Lambda([this]
				{
					return IsRecording()
						? LOCTEXT("MicRec", "● Rec")
						: LOCTEXT("MicIdle", "● Mic");
				})
				.MinDesiredWidth(48.f)
				.Justification(ETextJustify::Center)
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)
		[
			SNew(SComboButton)
			.Visibility_Lambda(VoiceVisible)
			.IsEnabled_Lambda([this]
			{
				return VoiceSource != ETryllChatVoiceSource::Off
					&& State == EState::Ready
					&& !bTurnInFlight;
			})
			.OnGetMenuContent(FOnGetContent::CreateSP(this, &STryllChatPanel::BuildVoiceModeMenu))
			.ButtonContent()
			[
				SNew(STextBlock)
				.Text(this, &STryllChatPanel::GetVoiceModeLabel)
				.MinDesiredWidth(84.f)
				.Justification(ETextJustify::Center)
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0,0,8,0)
		[
			SAssignNew(InputBox, SEditableTextBox)
			.AddMetaData<FTagMetaData>(FTagMetaData(TryllEditorWidgetIds::ChatInput))
			.HintText(LOCTEXT("InputHint", "Type a message and press Enter…"))
			.OnTextCommitted(this, &STryllChatPanel::OnInputCommitted)
			.IsEnabled_Lambda([this]
			{
				return State == EState::Ready && !IsRecording() && !bTurnInFlight;
			})
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SButton)
			.AddMetaData<FTagMetaData>(FTagMetaData(TryllEditorWidgetIds::ChatSend))
			.Text(LOCTEXT("Send", "Send"))
			.OnClicked(this, &STryllChatPanel::OnSendClicked)
			// While a turn streams (bTurnInFlight) the Send button is replaced
			// by the Stop button below.
			.Visibility_Lambda([this]{ return bTurnInFlight ? EVisibility::Collapsed : EVisibility::Visible; })
			.IsEnabled_Lambda([this]
			{
				return State == EState::Ready && !IsRecording() && !bTurnInFlight;
			})
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SButton)
			.Text(LOCTEXT("Stop", "Stop"))
			.OnClicked(this, &STryllChatPanel::OnStopClicked)
			.Visibility_Lambda([this]{ return bTurnInFlight ? EVisibility::Visible : EVisibility::Collapsed; })
			.IsEnabled_Lambda([this]{ return State == EState::Ready; })
		];
}

TSharedRef<SWidget> STryllChatPanel::BuildVoiceModeMenu()
{
	FMenuBuilder Menu(true, nullptr);
	Menu.AddMenuEntry(
		LOCTEXT("ModePtt", "Push-to-talk"),
		FText::GetEmpty(), FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &STryllChatPanel::SetVoiceMode, EVoiceMode::PushToTalk),
			FCanExecuteAction(),
			FIsActionChecked::CreateLambda([this]{ return VoiceMode == EVoiceMode::PushToTalk; })),
		NAME_None, EUserInterfaceActionType::RadioButton);
	Menu.AddMenuEntry(
		LOCTEXT("ModeHf", "Hands-free"),
		FText::GetEmpty(), FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &STryllChatPanel::SetVoiceMode, EVoiceMode::HandsFree),
			FCanExecuteAction(),
			FIsActionChecked::CreateLambda([this]{ return VoiceMode == EVoiceMode::HandsFree; })),
		NAME_None, EUserInterfaceActionType::RadioButton);
	return Menu.MakeWidget();
}

FText STryllChatPanel::GetVoiceModeLabel() const
{
	return VoiceMode == EVoiceMode::PushToTalk
		? LOCTEXT("LabelPtt", "Push-to-talk")
		: LOCTEXT("LabelHf", "Hands-free");
}

void STryllChatPanel::SetVoiceMode(EVoiceMode Mode)
{
	if (bTurnInFlight)
	{
		return;
	}
	if (bHandsFreeActive && Mode != EVoiceMode::HandsFree)
	{
		ExitHandsFree();
	}
	if (bPttHeld && Mode != EVoiceMode::PushToTalk)
	{
		bPttHeld = false;
		EndUtterance();
	}
	VoiceMode = Mode;
	SaveChatConfig();
}

float STryllChatPanel::GetBubbleMaxWidth() const
{
	float ChatWidth = 0.0f;
	if (ChatScroll.IsValid())
	{
		ChatWidth = ChatScroll->GetCachedGeometry().GetLocalSize().X;
	}
	if (ChatWidth <= 1.0f)
	{
		return FTryllEditorStyle::ChatMinWidth * FTryllEditorStyle::ChatBubbleWidthFraction;
	}
	return ChatWidth * FTryllEditorStyle::ChatBubbleWidthFraction;
}

float STryllChatPanel::GetBubbleWrapWidth() const
{
	// Name/body padding (8) on each side, plus the selected-bubble outline (2).
	constexpr float BubbleChrome = 20.0f;
	return FMath::Max(1.0f, GetBubbleMaxWidth() - BubbleChrome);
}

TSharedRef<SWidget> STryllChatPanel::WrapChatBubble(
	EHorizontalAlignment Align, TSharedRef<SWidget> Bubble) const
{
	// Fill the scroll row so the cap can grow after a splitter drag; AutoWrapText
	// caches last arrange width and would otherwise leave HAlign_Left bubbles stuck.
	return SNew(SBox)
		.HAlign(Align)
		[
			SNew(SBox)
			.MaxDesiredWidth_Lambda([this]
			{
				return GetBubbleMaxWidth();
			})
			[
				Bubble
			]
		];
}

void STryllChatPanel::RebuildChatScroll()
{
	if (!ChatScroll.IsValid())
	{
		return;
	}
	ChatScroll->ClearChildren();

	auto AddUserBubble = [this](const FString& Text)
	{
		ChatScroll->AddSlot().HAlign(HAlign_Fill).Padding(2)
		[
			WrapChatBubble(HAlign_Right,
				SNew(SBorder)
				.BorderImage(FTryllEditorStyle::Rounded(FTryllEditorStyle::RadiusBand))
				.BorderBackgroundColor(FTryllEditorStyle::UserBub)
				.Padding(8)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("YouLabel", "You"))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(Text))
						.WrapTextAt(this, &STryllChatPanel::GetBubbleWrapWidth)
					]
				])
		];
	};

	auto AddAssistantBubble = [this](
		const FTryllChatTurnRecord& Turn, const FString& BodyText, bool bSelectable)
	{
		const bool bSelected = Turn.bComplete && SelectedTurnKey == Turn.TurnKey;
		const FLinearColor BubbleColor = bSelected
			? FTryllEditorStyle::SelBub
			: FTryllEditorStyle::AsstBub;
		const FLinearColor BorderColor = bSelected
			? FTryllEditorStyle::Accent
			: FTryllEditorStyle::Line;
		const FString TurnKey = Turn.TurnKey;

		auto BuildInnerBubble = [&]()
		{
			TSharedRef<SVerticalBox> Content = SNew(SVerticalBox);
			Content->AddSlot().AutoHeight()
			[
				SNew(STextBlock)
					.Text(FText::FromString(GetAssistantDisplayName()))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
			];
			Content->AddSlot().AutoHeight()
			[
				SNew(STextBlock)
					.Text(FText::FromString(BodyText))
					.WrapTextAt(this, &STryllChatPanel::GetBubbleWrapWidth)
			];

			const bool bShowTerminalBadge = Turn.bComplete
				&& !Turn.AssistantText.IsEmpty()
				&& (Turn.Display == ETryllChatTurnDisplay::Cancelled
					|| Turn.Display == ETryllChatTurnDisplay::PauseTimedOut
					|| Turn.Display == ETryllChatTurnDisplay::Error);
			if (bShowTerminalBadge)
			{
				const bool bError = Turn.Display == ETryllChatTurnDisplay::Error;
				Content->AddSlot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBorder)
							.BorderImage(FTryllEditorStyle::Rounded(
								FTryllEditorStyle::RadiusChip,
								bError ? FTryllEditorStyle::ErrBorder : FTryllEditorStyle::WarnBorder,
								1.0f))
							.BorderBackgroundColor(FSlateColor(
								bError ? FTryllEditorStyle::ErrBg : FTryllEditorStyle::WarnBg))
							.Padding(FMargin(5.0f, 0.0f, 5.0f, 1.0f))
							[
								SNew(STextBlock)
									.Text(FText::FromString(
										FTryllChatTurnLogic::StatusRowLabel(Turn.Display)))
									.ColorAndOpacity(FSlateColor(
										bError ? FTryllEditorStyle::ColErr : FTryllEditorStyle::ColWarn))
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
							]
					]
				];
			}

			if (bSelectable)
			{
				return SNew(SBorder)
					.BorderImage(FTryllEditorStyle::Rounded(FTryllEditorStyle::RadiusBand))
					.BorderBackgroundColor(BubbleColor)
					.Padding(8)
					.OnMouseButtonDown(FPointerEventHandler::CreateLambda(
						[this, TurnKey](const FGeometry&, const FPointerEvent& Event)
						{
							if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
							{
								return OnAssistantBubbleClicked(TurnKey);
							}
							return FReply::Unhandled();
						}))
					[
						Content
					];
			}

			return SNew(SBorder)
				.BorderImage(FTryllEditorStyle::Rounded(FTryllEditorStyle::RadiusBand))
				.BorderBackgroundColor(BubbleColor)
				.Padding(8)
				[
					Content
				];
		};

		ChatScroll->AddSlot().HAlign(HAlign_Fill).Padding(2)
		[
			WrapChatBubble(HAlign_Left,
				SNew(SBorder)
				.BorderImage(FTryllEditorStyle::Rounded(
					FTryllEditorStyle::RadiusBand, BorderColor, bSelected ? 2.0f : 0.0f))
				.BorderBackgroundColor(BubbleColor)
				.Padding(bSelected ? 2 : 0)
				[
					BuildInnerBubble()
				])
		];
	};

	auto AddStatusRow = [this, &AddAssistantBubble](const FTryllChatTurnRecord& Turn)
	{
		AddAssistantBubble(Turn, FTryllChatTurnLogic::StatusRowLabel(Turn.Display), true);
	};

	for (int32 Index = 0; Index < Turns.Num(); ++Index)
	{
		const FTryllChatTurnRecord& Turn = Turns[Index];
		AddUserBubble(Turn.UserText);

		if (!Turn.bComplete)
		{
			if (Index == InFlightTurnIndex && !Turn.AssistantText.IsEmpty())
			{
				AddAssistantBubble(Turn, Turn.AssistantText, false);
			}
			continue;
		}

		if (FTryllChatTurnLogic::UsesStatusRow(Turn))
		{
			AddStatusRow(Turn);
		}
		else if (!Turn.AssistantText.IsEmpty())
		{
			AddAssistantBubble(Turn, Turn.AssistantText, true);
		}
	}

	ChatTail.NotifyAppend();
}

float STryllChatPanel::MeasureChatDistance() const
{
	if (!ChatScroll.IsValid())
	{
		return 0.0f;
	}
	return FMath::Max(0.0f, ChatScroll->GetScrollOffsetOfEnd() - ChatScroll->GetScrollOffset());
}

void STryllChatPanel::ScrollChatToEnd()
{
	if (!ChatScroll.IsValid())
	{
		return;
	}
	ChatTail.BeginProgrammaticScroll();
	ChatScroll->ScrollToEnd();
	ChatTail.EndProgrammaticScroll();
}

void STryllChatPanel::RefreshInspector()
{
	if (!InspectorView.IsValid())
	{
		return;
	}

	const FTryllChatTurnRecord* Selected = FindTurnByKey(SelectedTurnKey);
	if (Selected != nullptr)
	{
		InspectorView->Render(FTryllChatTurnLogic::ToInspection(*Selected));
	}
	else
	{
		InspectorView->Render(FTryllTurnInspectionFactory::Empty(TEXT("select an answer")));
	}
}

FReply STryllChatPanel::OnAssistantBubbleClicked(FString TurnKey)
{
	const FTryllChatTurnRecord* Turn = FindTurnByKey(TurnKey);
	if (Turn == nullptr || !Turn->bComplete)
	{
		return FReply::Handled();
	}

	SelectedTurnKey = TurnKey;
	RebuildChatScroll();
	RefreshInspector();
	return FReply::Handled();
}

const FTryllChatTurnRecord* STryllChatPanel::FindTurnByKey(const FString& TurnKey) const
{
	if (TurnKey.IsEmpty())
	{
		return nullptr;
	}

	for (const FTryllChatTurnRecord& Turn : Turns)
	{
		if (Turn.TurnKey == TurnKey)
		{
			return &Turn;
		}
	}
	return nullptr;
}

void STryllChatPanel::LoadChatConfig()
{
	const UTryllChatConfig* Config = GetDefault<UTryllChatConfig>();
	InspectorWidth    = FMath::Max(Config->InspectorWidth, FTryllEditorStyle::InspectorMinWidth);
	bInspectorVisible = Config->bInspectorVisible;
	SttModelName      = Config->SttModelName;
	const bool bMigrateVoice = Config->VoiceSource == FTryllChatVoiceSource::Unset;
	VoiceSource       = bMigrateVoice
		? FTryllChatVoiceSource::FromLegacy(Config->bEnableVoice)
		: FTryllChatVoiceSource::Clamp(Config->VoiceSource);
	VoiceMode         = static_cast<EVoiceMode>(FMath::Clamp(Config->VoiceMode, 0, 1));
	if (bMigrateVoice)
	{
		SaveChatConfig();
	}
}

void STryllChatPanel::SaveChatConfig() const
{
	if (!bPersistLayout)
	{
		return;
	}
	UTryllChatConfig* Config = GetMutableDefault<UTryllChatConfig>();
	Config->InspectorWidth    = InspectorWidth;
	Config->bInspectorVisible = bInspectorVisible;
	Config->bEnableVoice      = VoiceSource != ETryllChatVoiceSource::Off;
	Config->VoiceSource       = FTryllChatVoiceSource::ToInt(VoiceSource);
	Config->SttModelName      = SttModelName;
	Config->VoiceMode         = static_cast<int32>(VoiceMode);
	Config->SaveConfig();
}

TArray<FString> STryllChatPanel::RegisteredSttNames() const
{
	if (const UTryllModelManifest* Manifest = UTryllModelManifest::GetActive())
	{
		return Manifest->ModelsOfType(TEXT("stt"));
	}
	return {};
}

bool STryllChatPanel::CanEnableVoice() const
{
	return RegisteredSttNames().Num() > 0 || !SttModelName.IsEmpty();
}

void STryllChatPanel::RebuildSttModelOptions()
{
	SttModelOptions.Reset();
	for (const FString& Name : RegisteredSttNames())
	{
		SttModelOptions.Add(MakeShared<FString>(Name));
	}
	if (!SttModelName.IsEmpty()
		&& !SttModelOptions.ContainsByPredicate([&](const TSharedPtr<FString>& Option)
		{
			return Option.IsValid() && *Option == SttModelName;
		}))
	{
		SttModelOptions.Add(MakeShared<FString>(SttModelName));
	}
}

void STryllChatPanel::SetVoiceSource(ETryllChatVoiceSource Source)
{
	if (Source == ETryllChatVoiceSource::Chat)
	{
		const TArray<FString> Registered = RegisteredSttNames();
		if (Registered.Num() == 0 && SttModelName.IsEmpty())
		{
			return;
		}
		if (!Registered.Contains(SttModelName) && Registered.Num() > 0)
		{
			SttModelName = Registered[0];
		}
		RebuildSttModelOptions();
	}
	else if (Source == ETryllChatVoiceSource::Component
		&& !FTryllChatVoiceSource::IsComponentAvailable(CandidateVoiceComponents.Num()))
	{
		return;
	}

	VoiceSource = Source;
	SaveChatConfig();
}

TSharedRef<SWidget> STryllChatPanel::BuildVoiceSourceMenu()
{
	RefreshAgentCandidates();
	FMenuBuilder Menu(true, nullptr);

	auto Add = [this, &Menu](
		const FText& Label,
		const FText& Tip,
		ETryllChatVoiceSource Source,
		bool bEnabled)
	{
		Menu.AddMenuEntry(
			Label, Tip, FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &STryllChatPanel::SetVoiceSource, Source),
				FCanExecuteAction::CreateLambda([bEnabled]{ return bEnabled; }),
				FIsActionChecked::CreateLambda([this, Source]
				{
					return VoiceSource == Source;
				})),
			NAME_None, EUserInterfaceActionType::RadioButton);
	};

	Add(LOCTEXT("VoiceOff", "Off"),
		LOCTEXT("VoiceOffTip", "Text only. Start does not load an STT model."),
		ETryllChatVoiceSource::Off, true);

	const bool bChatEnabled = CanEnableVoice() || VoiceSource == ETryllChatVoiceSource::Chat;
	Add(LOCTEXT("VoiceChat", "Chat"),
		bChatEnabled
			? LOCTEXT("VoiceChatTip", "Chat owns speech-to-text and the microphone.")
			: LOCTEXT("VoiceChatNoStt", "Register an STT model in the Model Manager to use Chat voice."),
		ETryllChatVoiceSource::Chat, bChatEnabled);

	const bool bComponent = FTryllChatVoiceSource::IsComponentAvailable(CandidateVoiceComponents.Num());
	Add(LOCTEXT("VoiceActor", "Actor component"),
		bComponent
			? LOCTEXT("VoiceActorTip", "Use the selected actor's voice component settings. Chat still captures the microphone.")
			: LOCTEXT("VoiceActorMissing", "The selected actor has no UTryllVoiceInputComponent."),
		ETryllChatVoiceSource::Component, bComponent);

	return Menu.MakeWidget();
}

FText STryllChatPanel::GetVoiceSourceLabel() const
{
	switch (VoiceSource)
	{
	case ETryllChatVoiceSource::Chat:      return LOCTEXT("VoiceChat", "Chat");
	case ETryllChatVoiceSource::Component: return LOCTEXT("VoiceActor", "Actor component");
	default:                               return LOCTEXT("VoiceOff", "Off");
	}
}

FText STryllChatPanel::GetVoiceSourceTooltip() const
{
	if (VoiceSource == ETryllChatVoiceSource::Component)
	{
		if (!SelectedVoiceComponent.IsValid())
		{
			return LOCTEXT("VoiceActorMissingTip",
				"Selected actor has no UTryllVoiceInputComponent — choose Chat or Off");
		}
		const FString Model = SelectedVoiceComponent->ModelName.ToString();
		return FText::FromString(Model.IsEmpty() ? TEXT("(no STT model)") : Model);
	}
	if (VoiceSource == ETryllChatVoiceSource::Chat)
	{
		return LOCTEXT("VoiceChatTip", "Chat owns speech-to-text and the microphone.");
	}
	return LOCTEXT("VoiceOffTip", "Text only. Start does not load an STT model.");
}

bool STryllChatPanel::BeginChatTurn(const FString& UserText, const bool bFromVoice)
{
	if (!FTryllChatTurnLogic::CanBeginTurn(bTurnInFlight, InFlightTurnIndex))
	{
		return false;
	}
	++TurnSequence;
	InFlightTurnIndex = Turns.Add(FTryllChatTurnLogic::BeginTurn(TurnSequence, UserText, bFromVoice));
	bTurnInFlight = true;
	ChatTail.ResetFollowing();
	return true;
}

bool STryllChatPanel::FinalizeInFlightTurn(
	const ETryllTurnStatus TurnStatus,
	const FString& DebugInfo,
	const int32 TokensGenerated,
	const FString& ErrorText)
{
	return FTryllChatTurnLogic::SettleInFlightTurn(
		Turns,
		InFlightTurnIndex,
		bTurnInFlight,
		SelectedTurnKey,
		TurnStatus,
		DebugInfo,
		TokensGenerated,
		ErrorText);
}

// ── Component picker ──────────────────────────────────────────────────────────

void STryllChatPanel::RefreshAgentCandidates()
{
	CandidateActors.Reset();
	CandidateComponents.Reset();
	CandidateVoiceComponents.Reset();

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		return;
	}

	TSet<AActor*> SeenActors;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor) continue;
		TArray<UTryllAgentComponent*> Comps;
		Actor->GetComponents<UTryllAgentComponent>(Comps);
		if (Comps.Num() == 0) continue;
		if (!SeenActors.Contains(Actor))
		{
			SeenActors.Add(Actor);
			CandidateActors.Add(Actor);
		}
	}

	if (SelectedActor.IsValid())
	{
		TArray<UTryllAgentComponent*> Comps;
		SelectedActor->GetComponents<UTryllAgentComponent>(Comps);
		for (UTryllAgentComponent* C : Comps)
		{
			CandidateComponents.Add(C);
		}
		if (SelectedComponent.IsValid() && SelectedComponent->GetOwner() != SelectedActor.Get())
		{
			SelectedComponent.Reset();
		}
		if (!SelectedComponent.IsValid() && Comps.Num() > 0)
		{
			SelectedComponent = Comps[0];
		}

		TArray<UTryllVoiceInputComponent*> Voices;
		SelectedActor->GetComponents<UTryllVoiceInputComponent>(Voices);
		for (UTryllVoiceInputComponent* Voice : Voices)
		{
			CandidateVoiceComponents.Add(Voice);
		}
		const bool bVoiceStillValid = SelectedVoiceComponent.IsValid()
			&& CandidateVoiceComponents.Contains(SelectedVoiceComponent);
		if (!bVoiceStillValid)
		{
			SelectedVoiceComponent = Voices.Num() > 0 ? Voices[0] : nullptr;
		}
	}
	else
	{
		SelectedComponent.Reset();
		SelectedVoiceComponent.Reset();
	}
}

void STryllChatPanel::SyncSelectionFromEditor()
{
	if (!GEditor) return;

	USelection* Sel = GEditor->GetSelectedActors();
	if (!Sel) return;

	for (FSelectionIterator It(*Sel); It; ++It)
	{
		if (AActor* Actor = Cast<AActor>(*It))
		{
			TArray<UTryllAgentComponent*> Comps;
			Actor->GetComponents<UTryllAgentComponent>(Comps);
			if (Comps.Num() > 0)
			{
				SelectActor(Actor);
				return;
			}
		}
	}
}

TSharedRef<SWidget> STryllChatPanel::BuildActorPicker()
{
	return SNew(SComboButton)
		.IsEnabled_Lambda([this]{ return IsStartable(State); })
		.OnGetMenuContent_Lambda([this]()
		{
			RefreshAgentCandidates();
			FMenuBuilder Menu(true, nullptr);
			Menu.AddMenuEntry(
				LOCTEXT("ClearActor", "(None)"),
				FText::GetEmpty(), FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([this]
				{
					SelectedActor.Reset();
					SelectedComponent.Reset();
					SelectedVoiceComponent.Reset();
					CandidateComponents.Reset();
					CandidateVoiceComponents.Reset();
				})));
			for (const TWeakObjectPtr<AActor>& WeakActor : CandidateActors)
			{
				AActor* Actor = WeakActor.Get();
				if (!Actor) continue;
				const FText Label = FText::FromString(Actor->GetActorLabel());
				Menu.AddMenuEntry(
					Label, FText::GetEmpty(), FSlateIcon(),
					FUIAction(FExecuteAction::CreateSP(this, &STryllChatPanel::SelectActor, Actor)));
			}
			if (CandidateActors.Num() == 0)
			{
				Menu.AddMenuEntry(
					LOCTEXT("NoActors", "No actors with UTryllAgentComponent in the editor world"),
					FText::GetEmpty(), FSlateIcon(), FUIAction());
			}
			return Menu.MakeWidget();
		})
		.ButtonContent()
		[
			SNew(STextBlock).Text(this, &STryllChatPanel::GetSelectedActorLabel)
		];
}

TSharedRef<SWidget> STryllChatPanel::BuildComponentPicker()
{
	return SNew(SComboButton)
		.IsEnabled_Lambda([this]{ return IsStartable(State); })
		.OnGetMenuContent_Lambda([this]()
		{
			RefreshAgentCandidates();
			FMenuBuilder Menu(true, nullptr);
			for (const TWeakObjectPtr<UTryllAgentComponent>& WeakComp : CandidateComponents)
			{
				UTryllAgentComponent* Comp = WeakComp.Get();
				if (!Comp) continue;
				const FText Label = FText::FromString(Comp->GetName());
				Menu.AddMenuEntry(
					Label, FText::GetEmpty(), FSlateIcon(),
					FUIAction(FExecuteAction::CreateSP(this, &STryllChatPanel::SelectComponent, Comp)));
			}
			return Menu.MakeWidget();
		})
		.ButtonContent()
		[
			SNew(STextBlock).Text(this, &STryllChatPanel::GetSelectedComponentLabel)
		];
}

FText STryllChatPanel::GetSelectedActorLabel() const
{
	return SelectedActor.IsValid()
		? FText::FromString(SelectedActor->GetActorLabel())
		: LOCTEXT("PickActor", "Select actor…");
}

FText STryllChatPanel::GetSelectedComponentLabel() const
{
	return SelectedComponent.IsValid()
		? FText::FromString(SelectedComponent->GetName())
		: LOCTEXT("PickComponent", "Select component…");
}

void STryllChatPanel::SelectActor(AActor* Actor)
{
	SelectedActor = Actor;
	SelectedComponent.Reset();
	SelectedVoiceComponent.Reset();
	RefreshAgentCandidates();
	if (State == EState::Idle && SelectedComponent.IsValid())
	{
		SetState(EState::Idle, TEXT("Idle"));
	}
}

void STryllChatPanel::SelectComponent(UTryllAgentComponent* Component)
{
	SelectedComponent = Component;
	if (Component)
	{
		SelectedActor = Component->GetOwner();
	}
	if (State == EState::Idle && SelectedComponent.IsValid())
	{
		SetState(EState::Idle, TEXT("Idle"));
	}
}

void STryllChatPanel::SelectVoiceComponent(UTryllVoiceInputComponent* Component)
{
	SelectedVoiceComponent = Component;
}

TSharedRef<SWidget> STryllChatPanel::BuildVoiceComponentPicker()
{
	return SNew(SComboButton)
		.IsEnabled_Lambda([this]{ return IsStartable(State); })
		.OnGetMenuContent_Lambda([this]()
		{
			RefreshAgentCandidates();
			FMenuBuilder Menu(true, nullptr);
			for (const TWeakObjectPtr<UTryllVoiceInputComponent>& WeakComp : CandidateVoiceComponents)
			{
				UTryllVoiceInputComponent* Comp = WeakComp.Get();
				if (!Comp) continue;
				const FText Label = FText::FromString(Comp->GetName());
				Menu.AddMenuEntry(
					Label, FText::GetEmpty(), FSlateIcon(),
					FUIAction(FExecuteAction::CreateSP(this, &STryllChatPanel::SelectVoiceComponent, Comp)));
			}
			return Menu.MakeWidget();
		})
		.ButtonContent()
		[
			SNew(STextBlock).Text(this, &STryllChatPanel::GetSelectedVoiceComponentLabel)
		];
}

FText STryllChatPanel::GetSelectedVoiceComponentLabel() const
{
	return SelectedVoiceComponent.IsValid()
		? FText::FromString(SelectedVoiceComponent->GetName())
		: LOCTEXT("PickVoiceComponent", "Select voice component…");
}

FString STryllChatPanel::GetAssistantDisplayName() const
{
	if (SelectedComponent.IsValid())
	{
		if (AActor* Owner = SelectedComponent->GetOwner())
		{
			return Owner->GetActorLabel();
		}
		return SelectedComponent->GetName();
	}
	return TEXT("Assistant");
}

// ── Status helpers ────────────────────────────────────────────────────────────

FText STryllChatPanel::GetStartStopLabel() const
{
	return IsStartable(State) ? LOCTEXT("Start", "Start") : LOCTEXT("Stop", "Stop");
}

void STryllChatPanel::RefreshStatusChrome()
{
	if (!StatusBar.IsValid())
	{
		return;
	}

	ETryllPanelOperation LocalOp = ETryllPanelOperation::None;
	switch (State)
	{
	case EState::CreatingAgent:
		LocalOp = ETryllPanelOperation::CreatingAgent;
		break;
	case EState::CreatingVoiceInput:
		LocalOp = ETryllPanelOperation::CreatingVoiceInput;
		break;
	default:
		break;
	}

	FTryllStatusPresentation Pres;
	if (State == EState::CheckingModels)
	{
		Pres.Severity = ETryllStatusSeverity::Progress;
		Pres.Caption = TEXT("Checking required models…");
	}
	else
	{
		ETryllOwnerPhase Owner = EditorSubsystem
			? EditorSubsystem->GetOwnerPhase()
			: ETryllOwnerPhase::Idle;
		FString Detail = EditorSubsystem ? EditorSubsystem->GetOwnerDetail() : FString();
		if (State == EState::Error)
		{
			Owner = ETryllOwnerPhase::Error;
			Detail = Status;
		}
		Pres = FTryllStatusPresentation::Compose(
			Owner, Detail, LocalOp, ETryllEditorPanel::Chat);
	}

	StatusBar->Bind(Pres);
	if (Banner.IsValid())
	{
		if (State == EState::Error && !Status.IsEmpty())
		{
			Banner->Bind(Status, ETryllStatusSeverity::Error);
		}
		else if (State == EState::Idle && !Status.IsEmpty() && Status != TEXT("Idle"))
		{
			Banner->Bind(Status, ETryllStatusSeverity::Warning);
		}
		else
		{
			Banner->Clear();
		}
	}
}

void STryllChatPanel::HandleOwnerPhaseChanged(ETryllOwnerPhase /*Phase*/, const FString& /*Detail*/)
{
	RefreshStatusChrome();
}

void STryllChatPanel::SetState(EState S, const FString& InStatus)
{
	State  = S;
	Status = InStatus;
	RefreshStatusChrome();
}

void STryllChatPanel::Fail(const FString& Reason)
{
	SetState(EState::Error, Reason);
	UE_LOG(LogTemp, Warning, TEXT("STryllChatPanel: %s"), *Reason);
}

void STryllChatPanel::RejectStart(const FString& Reason)
{
	SetState(EState::Idle, Reason);
	UE_LOG(LogTemp, Warning, TEXT("STryllChatPanel: %s"), *Reason);
}

// ── Input ─────────────────────────────────────────────────────────────────────

FReply STryllChatPanel::OnStartStopClicked()
{
	if (IsStartable(State))
	{
		if (State == EState::Error)
		{
			Stop();
		}
		Start();
	}
	else
	{
		Stop();
	}
	return FReply::Handled();
}

FReply STryllChatPanel::OnSendClicked()
{
	if (State != EState::Ready
		|| bTurnInFlight
		|| !InputBox.IsValid()
		|| !ChatAgent.IsValid())
	{
		return FReply::Handled();
	}
	const FString Text = InputBox->GetText().ToString().TrimStartAndEnd();
	if (Text.IsEmpty()) return FReply::Handled();

	if (!BeginChatTurn(Text, false))
	{
		return FReply::Handled();
	}
	InputBox->SetText(FText::GetEmpty());
	ChatAgent->SendMessage(Text);
	RebuildChatScroll();
	return FReply::Handled();
}

void STryllChatPanel::DebugSendForTests(const FString& Text)
{
	if (InputBox.IsValid())
	{
		InputBox->SetText(FText::FromString(Text));
	}
	OnSendClicked();
}

FTryllPanelSnapshot STryllChatPanel::DebugSnapshotForTests() const
{
	static const TCHAR* StateNames[] = { TEXT("Idle"), TEXT("Connecting"), TEXT("Configuring"), TEXT("CheckingModels"),
		TEXT("CreatingAgent"), TEXT("CreatingVoiceInput"), TEXT("Ready"), TEXT("Error") };
	FTryllPanelSnapshot Out;
	Out.OwnerPhase = EditorSubsystem ? EditorSubsystem->GetOwnerPhase() : ETryllOwnerPhase::Idle;
	Out.LocalState = StateNames[static_cast<uint8>(State)];
	if (StatusBar.IsValid())
	{
		Out.StatusCaption = StatusBar->DebugCaption();
		Out.StatusAction = StatusBar->DebugActionText();
	}
	if (Banner.IsValid())
	{
		Out.BannerText = Banner->DebugText();
	}
	for (const FTryllChatTurnRecord& Turn : Turns)
	{
		Out.Rows.Add(Turn.AssistantText);
	}
	Out.bLeaseHeld = bAcquiredConnection;
	Out.ListGeneration = Turns.Num();
	return Out;
}

FReply STryllChatPanel::OnStopClicked()
{
	// Cooperative cancel (stop-and-leave-as-is). The turn ends with a Cancelled
	// TurnComplete, which clears bTurnInFlight and swaps the button back to Send.
	if (bTurnInFlight && ChatAgent.IsValid())
	{
		ChatAgent->Cancel(ETryllCancelMode::StopAndKeep);
	}
	return FReply::Handled();
}

void STryllChatPanel::OnInputCommitted(const FText& /*Text*/, ETextCommit::Type CommitType)
{
	if (CommitType == ETextCommit::OnEnter) OnSendClicked();
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void STryllChatPanel::AutofindSettings()
{
	if (Settings.IsValid()) return;
	Settings = GetMutableDefault<UTryllRuntimeSettings>();
}

void STryllChatPanel::DetachPreviewFromSelected()
{
#if WITH_EDITOR
	if (SelectedComponent.IsValid())
	{
		SelectedComponent->DetachEditorPreviewAgent();
	}
#endif
}

void STryllChatPanel::Start()
{
	if (!Settings.IsValid()) AutofindSettings();
	if (!Settings.IsValid()) { RejectStart(TEXT("No TryllRuntimeSettings available")); return; }
	RefreshAgentCandidates();
	if (!SelectedComponent.IsValid())
	{
		RejectStart(TEXT("Assign a UTryllAgentComponent from the scene"));
		return;
	}
	if (VoiceSource == ETryllChatVoiceSource::Component && !SelectedVoiceComponent.IsValid())
	{
		RejectStart(TEXT("Selected actor has no UTryllVoiceInputComponent — choose Chat or Off"));
		return;
	}
	if (EditorSubsystemOverride)
	{
		EditorSubsystem = EditorSubsystemOverride;
	}
	else
	{
		if (!GEditor) { Fail(TEXT("No editor")); return; }
		EditorSubsystem = GEditor->GetEditorSubsystem<UTryllEditorSubsystem>();
	}
	if (!EditorSubsystem) { Fail(TEXT("Shared editor subsystem unavailable")); return; }

	++RunGeneration;

	Turns.Reset();
	TurnSequence = 0;
	InFlightTurnIndex = INDEX_NONE;
	SelectedTurnKey.Reset();
	ChatVoice.Reset();
	bTurnInFlight           = false;
	bPttHeld                = false;
	bHandsFreeActive        = false;
	bAwaitingUtteranceFinal = false;
	bUtteranceActive        = false;
	VoicePartial.Empty();
	// Data was cleared above; refresh UI now (Unity Start does the same). Stop keeps history.
	RebuildChatScroll();
	RefreshInspector();

	EditorSubsystem->OnConnectionChangedNative.AddSP(this, &STryllChatPanel::HandleConnectionChanged);
	EditorSubsystem->OnCreateSessionCompleteNative.AddSP(this, &STryllChatPanel::HandleCreateSessionComplete);
	EditorSubsystem->OnErrorNative.AddSP(this, &STryllChatPanel::HandleError);
	OwnerPhaseHandle = EditorSubsystem->OnOwnerPhaseChangedNative.AddSP(
		this, &STryllChatPanel::HandleOwnerPhaseChanged);

	SetState(EState::Connecting, FTryllEditorUiContract::StatusConnecting);
	bAcquiredConnection = true;
	EditorSubsystem->AcquireConnection();

	// Join-late: another consumer (PIE, the model manager panel) may have
	// already brought the shared connection up before this panel opened.
	if (EditorSubsystem->IsConnected())
	{
		HandleConnectionChanged(true);
	}
}

void STryllChatPanel::Stop(const bool bRefreshUi)
{
	// Invalidate every in-flight CreateAgent / Voice / model-guard continuation
	// before tearing down local handles.
	++RunGeneration;

	bHandsFreeActive        = false;
	bPttHeld                = false;
	bAwaitingUtteranceFinal = false;
	bUtteranceActive        = false;
	StopCapture();
	ModelGuard.Reset(); // unsubscribes from EditorSubsystem's delegates, which we're about to release

	DetachPreviewFromSelected();

	if (EditorSubsystem)
	{
		if (ChatVoice.IsValid())
		{
			EditorSubsystem->RequestDestroyVoiceInput(ChatVoice);
			ChatVoice.Reset();
		}
		if (ChatAgent.IsValid())
		{
			EditorSubsystem->RequestDestroyAgent(ChatAgent);
			ChatAgent.Reset();
		}

		EditorSubsystem->OnConnectionChangedNative.RemoveAll(this);
		EditorSubsystem->OnCreateSessionCompleteNative.RemoveAll(this);
		EditorSubsystem->OnErrorNative.RemoveAll(this);
		EditorSubsystem->OnOwnerPhaseChangedNative.Remove(OwnerPhaseHandle);
		OwnerPhaseHandle.Reset();

		// Mirrors the Unity fix: release our own acquisition rather than forcing
		// a disconnect that would tear down the connection for other consumers.
		if (bAcquiredConnection)
		{
			EditorSubsystem->ReleaseConnection();
		}
	}
	bAcquiredConnection = false;
	EditorSubsystem = nullptr;
	bTurnInFlight = false;
	SelectedTurnKey.Reset();
	InFlightTurnIndex = INDEX_NONE;

	VoicePartial.Empty();
	// Do not release/kill the shared server: other consumers may still use it.
	SetState(EState::Idle, TEXT("Idle"));
	if (bRefreshUi)
	{
		RebuildChatScroll();
		RefreshInspector();
	}
}

// ── Shared subsystem event handlers ─────────────────────────────────────────────

void STryllChatPanel::HandleConnectionChanged(bool bConnected)
{
	if (!bConnected)
	{
		// Connect() replacement and accept-late ConnectFailed broadcast false
		// while Chat is bringing the socket up. Ignore only while the owner
		// still has retry budget — a launch failure or exhausted budget must
		// leave Connecting and become Error.
		const bool bRetriesRemaining =
			EditorSubsystem && EditorSubsystem->HasConnectRetriesRemaining();
		if (TryllEditorConnectionPolicy::ShouldWaitForOwnerRetry(
				State == EState::Connecting, bRetriesRemaining))
		{
			return;
		}
		Fail(LastErrorMessage.IsEmpty() ? FString(TEXT("Disconnected")) : LastErrorMessage);
		return;
	}
	if (!EditorSubsystem) return;

	SetState(EState::Configuring, TEXT("Configuring session…"));
	// Engines come from the project's model manifest now — the panel does not pick
	// them, and the Voice Input selector no longer has to imply an STT engine here.
	EditorSubsystem->CreateSessionFromSettings();
	// If the session is already configured (join-late), CreateSession() above
	// broadcasts synchronously and HandleCreateSessionComplete() below already
	// subscribed — no separate check needed here.
}

void STryllChatPanel::HandleCreateSessionComplete(bool bSuccess)
{
	if (!bSuccess)
	{
		Fail(FString::Printf(TEXT("CreateSession failed: %s"), *LastErrorMessage));
		return;
	}
	if (!EditorSubsystem || !SelectedComponent.IsValid()) return;
	if (ChatAgent.IsValid()) return;   // already have an agent; ignore redundant broadcast
	if (ModelGuard.IsValid() && ModelGuard->IsActive()) return;   // already checking; ignore redundant broadcast

	// feat-011: ensure every registered (Production/Experimental) model is downloaded
	// before creating the agent — same blocking treatment as PIE.
	SetState(EState::CheckingModels, TEXT("Checking required models…"));
	ModelGuard = MakeShared<FTryllModelDownloadGuard>();
	ModelGuard->Start(EditorSubsystem, FTryllModelDownloadGuard::FOnComplete::CreateSP(this, &STryllChatPanel::OnModelsReady));
}

void STryllChatPanel::OnModelsReady(bool bSuccess, FString Error)
{
	ModelGuard.Reset();
	if (!bSuccess)
	{
		Fail(Error.IsEmpty() ? TEXT("Model download cancelled.") : Error);
		return;
	}
	if (!EditorSubsystem || !SelectedComponent.IsValid()) return;
	if (ChatAgent.IsValid()) return;   // already have an agent; ignore redundant broadcast

	UTryllAgentComponent* Comp = SelectedComponent.Get();
	FTryllGraphDescription ResolvedGraph;
	TArray<FTryllVariableDecl> Declared;
	if (!Comp->WorkflowAsset)
	{
		Fail(TEXT("Assign a Workflow Asset on the selected component."));
		return;
	}
	ResolvedGraph = Comp->WorkflowAsset->Graph;
	ResolvedGraph.TextTable = Comp->WorkflowAsset->TextTable;
	ResolvedGraph.LocalizedAssetName = Comp->WorkflowAsset->GetName();
	Declared      = Comp->WorkflowAsset->Variables;
	const TArray<FTryllVariableDecl> Merged =
		TryllVariableOverrideUtils::MergeVariableOverrides(Declared, Comp->VariableOverrides);

	constexpr bool bEnableDiagnostics = true;
	const bool bMaintainHistory   = Comp->bMaintainDialogueHistory;
	// The scene agent's own voice, so Chat speaks (and lip-syncs) like PIE does.
	const TOptional<FTryllVoiceProfileData> Voice = Comp->GetEffectiveVoiceProfile();

		SetState(EState::CreatingAgent, FTryllEditorUiContract::StatusCreatingAgent);
	TWeakPtr<STryllChatPanel> WeakSelf = SharedThis(this);
	const uint64 Gen = RunGeneration;
	TWeakObjectPtr<UTryllEditorSubsystem> WeakEditor(EditorSubsystem);
	EditorSubsystem->RequestCreateAgent(ResolvedGraph,
		[WeakSelf, WeakEditor, Gen](TSharedPtr<FTryllAgent> Agent, FTryllError CreateError)
		{
			TSharedPtr<STryllChatPanel> Panel = WeakSelf.Pin();
			if (!Panel.IsValid() || Panel->RunGeneration != Gen)
			{
				// Stop() raced CreateAgent — destroy the late agent without
				// reattaching a preview or touching a cleared EditorSubsystem.
				if (Agent.IsValid())
				{
					if (UTryllEditorSubsystem* Ed = WeakEditor.Get())
					{
						Ed->RequestDestroyAgent(Agent);
					}
				}
				return;
			}
			if (!Agent.IsValid())
			{
				Panel->Fail(FString::Printf(TEXT("CreateAgent failed: %s"), *CreateError.Message));
				return;
			}
			Panel->ChatAgent = Agent;
			// Agent Log (and other observers) read DisplayName via SnapshotRegisteredAgents.
			Agent->SetDisplayName(Panel->GetAssistantDisplayName());

#if WITH_EDITOR
			if (Panel->SelectedComponent.IsValid())
			{
				Panel->SelectedComponent->AttachEditorPreviewAgent(Agent);
				TryllToolStateReset::ResetOnOwner(Panel->SelectedComponent.Get());
			}
#endif

			Agent->SetOnAnswerText([WeakSelf, Gen](const FString& /*NodeName*/, const FString& Text, bool bIsDelta, bool /*bIsFinal*/)
			{
				TSharedPtr<STryllChatPanel> P = WeakSelf.Pin();
				if (!P.IsValid() || P->RunGeneration != Gen) return;
				if (P->Turns.IsValidIndex(P->InFlightTurnIndex))
				{
					FTryllChatTurnLogic::AppendAssistantDelta(
						P->Turns[P->InFlightTurnIndex], Text, bIsDelta);
				}
				P->RebuildChatScroll();
			});

			Agent->SetOnTurnComplete([WeakSelf, Gen](ETryllTurnStatus TurnStatus, const FString& DebugInfo, int32 TokensGenerated)
			{
				TSharedPtr<STryllChatPanel> P = WeakSelf.Pin();
				if (!P.IsValid() || P->RunGeneration != Gen) return;
				P->FinalizeInFlightTurn(TurnStatus, DebugInfo, TokensGenerated);
				P->RebuildChatScroll();
				P->RefreshInspector();
				if (P->bHandsFreeActive && P->State == EState::Ready)
				{
					P->BeginUtterance(true);
				}
			});

			Agent->SetOnError([WeakSelf, Gen](const FTryllError& AgentError)
			{
				TSharedPtr<STryllChatPanel> P = WeakSelf.Pin();
				if (!P.IsValid() || P->RunGeneration != Gen) return;
				const FString Reason = FString::Printf(TEXT("Send failed: %s"), *AgentError.Message);
				P->FinalizeInFlightTurn(
					ETryllTurnStatus::Error, FString(), 0, AgentError.Message);
				P->RebuildChatScroll();
				P->RefreshInspector();
				P->Fail(Reason);
			});

			FTryllVoiceInputConfig VoiceConfig;
			bool bCreateVoice = false;
			if (Panel->VoiceSource == ETryllChatVoiceSource::Component)
			{
				UTryllVoiceInputComponent* VoiceComp = Panel->SelectedVoiceComponent.Get();
				if (!VoiceComp)
				{
					Panel->Fail(TEXT("Selected actor has no UTryllVoiceInputComponent — choose Chat or Off"));
					return;
				}
				Panel->CaptureSampleRate = ResolveCaptureRate();
				VoiceConfig = VoiceComp->BuildVoiceInputConfig(Panel->CaptureSampleRate);
				bCreateVoice = true;
			}
			else if (Panel->VoiceSource == ETryllChatVoiceSource::Chat && !Panel->SttModelName.IsEmpty())
			{
				Panel->CaptureSampleRate = ResolveCaptureRate();
				VoiceConfig.ModelName                = Panel->SttModelName;
				VoiceConfig.InputFormat.SampleRate   = Panel->CaptureSampleRate;
				VoiceConfig.InputFormat.Channels     = 1;
				VoiceConfig.InputFormat.BitsPerSample = 16;
				VoiceConfig.VadThreshold             = 0.5f;
				VoiceConfig.VadMinSilenceMs          = 500;
				VoiceConfig.VadSpeechPadMs           = 250;
				bCreateVoice = true;
			}

			if (bCreateVoice)
			{
				Panel->RequestChatVoice(VoiceConfig, Gen);
			}
			else
			{
				Panel->SetState(EState::Ready, FTryllEditorUiContract::StatusReady);
			}
		},
		bEnableDiagnostics,
		bMaintainHistory,
		Merged,
		ETryllKvCacheInitialization::AllocateOnly,
		ETryllWorkload::Interactive,
		Voice);
}

void STryllChatPanel::RequestChatVoice(const FTryllVoiceInputConfig& Config, uint64 Gen)
{
	if (RunGeneration != Gen)
	{
		return;
	}
	if (!EditorSubsystem)
	{
		Fail(TEXT("Shared editor subsystem unavailable"));
		return;
	}
	SetState(EState::CreatingVoiceInput, FTryllEditorUiContract::StatusCreatingVoiceInput);
	TWeakPtr<STryllChatPanel> WeakSelf = SharedThis(this);
	TWeakObjectPtr<UTryllEditorSubsystem> WeakEditor(EditorSubsystem);
	EditorSubsystem->RequestCreateVoiceInput(Config,
		[WeakSelf, WeakEditor, Gen](TSharedPtr<FTryllVoiceInput> Voice, FTryllError VError)
		{
			TSharedPtr<STryllChatPanel> P2 = WeakSelf.Pin();
			if (!P2.IsValid() || P2->RunGeneration != Gen)
			{
				if (Voice.IsValid())
				{
					if (UTryllEditorSubsystem* Ed = WeakEditor.Get())
					{
						Ed->RequestDestroyVoiceInput(Voice);
					}
				}
				return;
			}
			if (!Voice.IsValid())
			{
				P2->Fail(FString::Printf(TEXT("CreateVoiceInput failed: %s"), *VError.Message));
				return;
			}
			P2->ChatVoice = Voice;
			Voice->SetOnTranscriptUpdate([WeakSelf, Gen](const FTryllTranscriptUpdate& Update)
			{
				TSharedPtr<STryllChatPanel> P3 = WeakSelf.Pin();
				if (!P3.IsValid() || P3->RunGeneration != Gen) return;
				P3->HandleTranscriptUpdate(Update.Text, static_cast<uint8>(Update.Kind));
			});
			P2->SetState(EState::Ready, FTryllEditorUiContract::StatusReady);
		});
}

void STryllChatPanel::HandleError(const FString& ErrorMessage)
{
	// Cached for whichever paired bool-only completion fires next — see
	// LastErrorMessage's declaration for why this is always fresh when read.
	LastErrorMessage = ErrorMessage;
}

// ── Tick ──────────────────────────────────────────────────────────────────────

void STryllChatPanel::Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	// Connection events are pumped centrally by the shared UTryllEditorSubsystem, not here.
	if (ModelGuard.IsValid()) ModelGuard->Tick();

	if (!IsStartable(State) && !SelectedComponent.IsValid())
	{
		Stop();
		Fail(TEXT("Selected agent component was destroyed"));
		return;
	}

	const float BubbleMax = GetBubbleMaxWidth();
	if (!FMath::IsNearlyEqual(BubbleMax, LastBubbleMaxWidth, 1.0f))
	{
		LastBubbleMaxWidth = BubbleMax;
		if (ChatScroll.IsValid())
		{
			ChatScroll->Invalidate(EInvalidateWidgetReason::Layout);
		}
	}

	if (bCapturing && MicCapture.IsValid())
	{
		TArray<int16> Chunk;
		{
			FScopeLock Lock(&MicCapture->BufferLock);
			if (MicCapture->PendingPcm.Num() > 0)
			{
				Chunk = MoveTemp(MicCapture->PendingPcm);
				MicCapture->PendingPcm.Reset();
			}
		}
		// Drain unconditionally so the buffer never grows unbounded; only
		// forward audio while an utterance is open on the server.
		if (Chunk.Num() > 0 && bUtteranceActive && ChatVoice.IsValid())
		{
			ChatVoice->SendAudioBuffer(Chunk);
		}
	}
}

// ── Voice ─────────────────────────────────────────────────────────────────────

void STryllChatPanel::OnMicPressed()
{
	if (VoiceSource == ETryllChatVoiceSource::Off
		|| State != EState::Ready
		|| bTurnInFlight
		|| !ChatVoice.IsValid())
	{
		return;
	}

	if (VoiceMode == EVoiceMode::PushToTalk)
	{
		if (bPttHeld || bAwaitingUtteranceFinal)
		{
			return;
		}
		bPttHeld = true;
		BeginUtterance(false);
	}
	else if (bHandsFreeActive)
	{
		ExitHandsFree();
	}
	else
	{
		EnterHandsFree();
	}
}

void STryllChatPanel::OnMicReleased()
{
	if (!bPttHeld)
	{
		return;
	}
	// SButton fires OnReleased when the cursor leaves the button while held;
	// ignore those — only the real mouse-up should end the utterance.
	if (FSlateApplication::Get().GetPressedMouseButtons().Contains(EKeys::LeftMouseButton))
	{
		return;
	}
	bPttHeld = false;
	EndUtterance();
}

void STryllChatPanel::EnterHandsFree()
{
	if (bTurnInFlight)
	{
		return;
	}
	bHandsFreeActive = true;
	BeginUtterance(true);
}

void STryllChatPanel::ExitHandsFree()
{
	if (!bHandsFreeActive)
	{
		return;
	}
	bHandsFreeActive = false;
	CancelUtterance();
}

void STryllChatPanel::BeginUtterance(bool bAutoFinishOnSilence)
{
	if (!ChatVoice.IsValid() || bTurnInFlight)
	{
		return;
	}
	if (bUtteranceActive || bAwaitingUtteranceFinal)
	{
		return;
	}

	VoicePartial.Empty();
	if (InputBox.IsValid() && !bCapturing)
	{
		PreVoiceInput = InputBox->GetText().ToString();
	}

	if (!StartCapture())
	{
		bPttHeld = false;
		UE_LOG(LogTemp, Warning, TEXT("[Tryll] Microphone capture failed to start"));
		return;
	}

	FTryllUtteranceOptions Opts;
	Opts.bAutoFinishOnSilence = bAutoFinishOnSilence;
	Opts.MaxUtteranceMs       = 60000;

	TWeakPtr<STryllChatPanel> WeakSelf = SharedThis(this);
	ChatVoice->BeginUtterance(Opts, [WeakSelf](const FTryllError& Error)
	{
		TSharedPtr<STryllChatPanel> P = WeakSelf.Pin();
		if (!P.IsValid() || Error.IsOk()) return;
		P->bPttHeld                = false;
		P->bUtteranceActive        = false;
		P->bAwaitingUtteranceFinal = false;
		P->bHandsFreeActive        = false;
		P->StopCapture();
		UE_LOG(LogTemp, Warning, TEXT("[Tryll] BeginUtterance failed: %s"), *Error.Message);
	});
	bUtteranceActive = true;
	bCapturing       = true;

	if (InputBox.IsValid())
	{
		InputBox->SetText(LOCTEXT("Listening", "Listening…"));
	}
}

void STryllChatPanel::EndUtterance()
{
	StopCapture();
	if (ChatVoice.IsValid())
	{
		bAwaitingUtteranceFinal = true;
		TWeakPtr<STryllChatPanel> WeakSelf = SharedThis(this);
		ChatVoice->EndUtterance([WeakSelf](const FTryllError& Error)
		{
			TSharedPtr<STryllChatPanel> P = WeakSelf.Pin();
			if (!P.IsValid() || Error.IsOk()) return;
			P->bAwaitingUtteranceFinal = false;
			UE_LOG(LogTemp, Warning, TEXT("[Tryll] Utterance op failed: %s"), *Error.Message);
		});
	}
}

void STryllChatPanel::CancelUtterance()
{
	StopCapture();
	VoicePartial.Empty();
	bAwaitingUtteranceFinal = false;
	bUtteranceActive        = false;
	if (ChatVoice.IsValid())
	{
		TWeakPtr<STryllChatPanel> WeakSelf = SharedThis(this);
		ChatVoice->CancelUtterance([WeakSelf](const FTryllError& Error)
		{
			TSharedPtr<STryllChatPanel> P = WeakSelf.Pin();
			if (!P.IsValid() || Error.IsOk()) return;
			UE_LOG(LogTemp, Warning, TEXT("[Tryll] Utterance op failed: %s"), *Error.Message);
		});
	}
	if (InputBox.IsValid())
	{
		InputBox->SetText(FText::FromString(PreVoiceInput));
	}
}

void STryllChatPanel::HandleTranscriptUpdate(const FString& Text, uint8 Kind)
{
	switch (static_cast<ETryllTranscriptUpdateKind>(Kind))
	{
	case ETryllTranscriptUpdateKind::SpeechStart:
		VoicePartial.Empty();
		break;

	case ETryllTranscriptUpdateKind::Partial:
	case ETryllTranscriptUpdateKind::SegmentFinal:
		VoicePartial = Text;
		break;

	case ETryllTranscriptUpdateKind::UtteranceFinal:
		bAwaitingUtteranceFinal = false;
		bUtteranceActive        = false;
		StopCapture();
		VoicePartial.Empty();
		if (!Text.IsEmpty())
		{
			if (BeginChatTurn(Text, true))
			{
				if (ChatAgent.IsValid())
				{
					ChatAgent->SendMessage(Text);
				}
				RebuildChatScroll();
			}
			else
			{
				UE_LOG(LogTemp, Warning,
					TEXT("STryllChatPanel: ignored final transcript while a turn was active"));
			}
		}
		else if (bHandsFreeActive && State == EState::Ready)
		{
			BeginUtterance(true);
		}
		break;
	}

	if (InputBox.IsValid())
	{
		if (IsRecording())
		{
			InputBox->SetText(FText::FromString(
				VoicePartial.IsEmpty() ? FString(TEXT("Listening…")) : VoicePartial));
		}
		else
		{
			InputBox->SetText(FText::FromString(PreVoiceInput));
		}
	}
}

bool STryllChatPanel::StartCapture()
{
	if (MicCapture.IsValid())
	{
		return true;
	}
	MicCapture = MakeUnique<FTryllChatMicCapture>();

	FTryllChatMicCapture* Cap = MicCapture.Get();
	Audio::FOnAudioCaptureFunction OnCapture =
		[Cap](const void* InAudio, int32 NumFrames, int32 NumChannels, int32 /*SampleRate*/,
		      double /*StreamTime*/, bool /*bOverflow*/)
		{
			if (!InAudio || NumFrames <= 0 || NumChannels <= 0)
			{
				return;
			}
			const float* Samples = static_cast<const float*>(InAudio);

			TArray<int16> Mono;
			Mono.SetNumUninitialized(NumFrames);
			for (int32 Frame = 0; Frame < NumFrames; ++Frame)
			{
				float Sum = 0.f;
				for (int32 Ch = 0; Ch < NumChannels; ++Ch)
				{
					Sum += Samples[Frame * NumChannels + Ch];
				}
				const float Mixed = FMath::Clamp(Sum / NumChannels, -1.f, 1.f);
				Mono[Frame] = static_cast<int16>(FMath::RoundToInt(Mixed * 32767.f));
			}

			FScopeLock Lock(&Cap->BufferLock);
			Cap->PendingPcm.Append(Mono);
		};

	Audio::FAudioCaptureDeviceParams Params;
	if (!MicCapture->Capture.OpenAudioCaptureStream(Params, MoveTemp(OnCapture), 1024))
	{
		MicCapture.Reset();
		return false;
	}
	if (!MicCapture->Capture.StartStream())
	{
		MicCapture->Capture.CloseStream();
		MicCapture.Reset();
		return false;
	}
	return true;
}

void STryllChatPanel::StopCapture()
{
	bCapturing = false;
	if (MicCapture.IsValid())
	{
		MicCapture->Capture.StopStream();
		MicCapture->Capture.CloseStream();
		MicCapture.Reset();
	}
}

#undef LOCTEXT_NAMESPACE
