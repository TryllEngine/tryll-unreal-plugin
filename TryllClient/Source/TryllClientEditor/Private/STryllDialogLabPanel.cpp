// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "STryllDialogLabPanel.h"
#include "TryllEditorPrompts.h"

#include "STryllTurnInspectorView.h"
#include "TryllTurnInspectorModels.h"
#include "TryllDialogLabConfig.h"
#include "TryllDialogLabVariantCodec.h"
#include "TryllDialogLabCompareFromChat.h"

#include "TryllClientEditorModule.h"

#include "TryllDialogLabExport.h"
#include "TryllDialogLabImport.h"
#include "TryllDialogLabRunner.h"
#include "TryllDialogLabDialogues.h"
#include "TryllDialogLabRunValidation.h"
#include "TryllDialogScriptAsset.h"
#include "TryllDialogSuiteAsset.h"
#include "TryllDialogScriptValidation.h"
#include "TryllEditorSubsystem.h"
#include "TryllEditorConnectionPolicy.h"
#include "TryllStatusPresentation.h"
#include "TryllEditorUiContract.h"
#include "UI/STryllBanner.h"
#include "UI/STryllInspectorSplit.h"
#include "UI/STryllStatusBar.h"
#include "UI/TryllEditorStyle.h"
#include "TryllModelDownloadGuard.h"
#include "TryllNodeFieldReflection.h"
#include "TryllModelManifest.h"
#include "TryllAgentComponent.h"
#include "TryllRuntimeSettings.h"
#include "TryllWorkflowAsset.h"

#include "DesktopPlatformModule.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "PropertyCustomizationHelpers.h"
#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"

#define LOCTEXT_NAMESPACE "TryllDialogLab"

namespace
{
	TSharedRef<SWidget> FieldLabel(const FText& Text)
	{
		return SNew(SBox)
			.WidthOverride(118.0f)
			[
				SNew(STextBlock)
					.Text(Text)
					.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
			];
	}
}

void STryllDialogLabPanel::Construct(const FArguments& InArgs)
{
	bPersistLayout = InArgs._PersistLayout;
	if (InArgs._EditorSubsystemOverride != nullptr)
	{
		EditorSubsystem = InArgs._EditorSubsystemOverride;
	}
	else if (GEditor != nullptr)
	{
		EditorSubsystem = GEditor->GetEditorSubsystem<UTryllEditorSubsystem>();
	}
	// Unlike Unity, these are project settings rather than an asset, so there is nothing for
	// the user to assign.
	Settings = GetMutableDefault<UTryllRuntimeSettings>();
	RefreshAgentCandidates();
	// After the candidate sweep, so a persisted agent path resolves against a loaded world.
	LoadLabConfig();
	if (InArgs._AgentComponentOverride != nullptr)
	{
		DefaultAgent = InArgs._AgentComponentOverride;
	}

	ChildSlot
	[
		// Unity sets EditorWindow.minSize; the Slate equivalent is a minimum desired size, which
		// a floating tab honours. A docked tab can still be squeezed by its parent layout.
		SNew(SBox)
			.MinDesiredWidth(FTryllEditorStyle::PanelMinWidth)
			.MinDesiredHeight(FTryllEditorStyle::PanelMinHeight)
			[
		SNew(SBorder)
			.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::Bg))
			.Padding(0.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot().AutoHeight()
				[
					BuildConfigBox()
				]

				+ SVerticalBox::Slot().AutoHeight()
				[
					BuildToolbar()
				]

				+ SVerticalBox::Slot().AutoHeight()
				[
					SAssignNew(ImportBanner, STryllBanner)
				]

				+ SVerticalBox::Slot().AutoHeight()
				[
					BuildProgressRow()
				]

				+ SVerticalBox::Slot().FillHeight(1.0f)
				[
					SNew(STryllInspectorSplit)
						.FixedSide(STryllInspectorSplit::EFixedSide::Second)
						.PrimaryMin(FTryllEditorStyle::StageMinWidth)
						.SecondaryMin(FTryllEditorStyle::InspectorMinWidth)
						.StoredWidth_Lambda([this] { return InspectorWidth; })
						.OnStoredWidthChanged_Lambda([this](float NewWidth)
						{
							InspectorWidth = NewWidth;
							SaveLabConfig();
						})
						.IsFixedPaneVisible_Lambda([this] { return bInspectorVisible; })
						.PrimaryContent()
						[
							SAssignNew(StageHost, SVerticalBox)
						]
						.SecondaryContent()
						[
							SAssignNew(InspectorView, STryllTurnInspectorView)
						]
				]
			]
		]
	];

	if (EditorSubsystem != nullptr)
	{
		OwnerPhaseHandle = EditorSubsystem->OnOwnerPhaseChangedNative.AddSP(
			this, &STryllDialogLabPanel::HandleOwnerPhaseChanged);
	}

	RebuildStage();
	RefreshOwnerChrome();
	HostColorsHandle = FTryllEditorStyle::OnHostColorsRefreshed.AddRaw(
		this, &STryllDialogLabPanel::HandleHostColorsRefreshed);
}

void STryllDialogLabPanel::HandleHostColorsRefreshed()
{
	RebuildStage();
	RefreshInspector();
	RefreshOwnerChrome();
}

STryllDialogLabPanel::~STryllDialogLabPanel()
{
	if (HostColorsHandle.IsValid())
	{
		FTryllEditorStyle::OnHostColorsRefreshed.Remove(HostColorsHandle);
		HostColorsHandle.Reset();
	}
	++RunGeneration;
	if (Runner.IsValid())
	{
		Runner->Stop();
		Runner.Reset();
	}
	if (EditorSubsystem != nullptr && OwnerPhaseHandle.IsValid())
	{
		EditorSubsystem->OnOwnerPhaseChangedNative.Remove(OwnerPhaseHandle);
		OwnerPhaseHandle.Reset();
	}
	TeardownSession();
}

TSharedRef<SWidget> STryllDialogLabPanel::BuildConfigBox()
{
	return SNew(SBorder)
		.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::Panel))
		.Padding(FMargin(10.0f, 7.0f, 10.0f, 5.0f))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()[FieldLabel(LOCTEXT("SettingsLabel", "Tryll Settings"))]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.Text(LOCTEXT("SettingsSource",
							"Project Settings ▶ Plugins ▶ Tryll Client"))
						.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Faint))
				]
			]

			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()[FieldLabel(LOCTEXT("AgentLabel", "Agent (default)"))]
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(SComboBox<TWeakObjectPtr<UTryllAgentComponent>>)
						.OptionsSource(&AgentCandidates)
						.OnComboBoxOpening_Lambda([this] { RefreshAgentCandidates(); })
						.OnGenerateWidget_Lambda([](TWeakObjectPtr<UTryllAgentComponent> Item)
						{
							return SNew(STextBlock).Text(FText::FromString(
								Item.IsValid() ? Item->GetOwner()->GetActorNameOrLabel() : TEXT("None")));
						})
						// Changing the agent rebuilds the whole stage: it changes the graph every
						// non-overriding variant's pickers are built against.
						.OnSelectionChanged_Lambda(
							[this](TWeakObjectPtr<UTryllAgentComponent> Item, ESelectInfo::Type)
							{
								DefaultAgent = Item;
								RebuildStage();
							})
						[
							SNew(STextBlock).Text_Lambda([this]
							{
								return DefaultAgent.IsValid() && DefaultAgent->GetOwner() != nullptr
									? FText::FromString(DefaultAgent->GetOwner()->GetActorNameOrLabel())
									: LOCTEXT("NoAgent", "None");
							})
						]
				]
			]

			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()[FieldLabel(LOCTEXT("ScriptLabel", "Dialog Script"))]
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(SObjectPropertyEntryBox)
						.AllowedClass(UPrimaryDataAsset::StaticClass())
						.OnShouldFilterAsset_Lambda([](const FAssetData& Asset)
						{
							const UClass* Class = Asset.GetClass();
							return Class == nullptr
								|| !(Class->IsChildOf(UTryllDialogScriptAsset::StaticClass())
									|| Class->IsChildOf(UTryllDialogSuiteAsset::StaticClass()));
						})
						.ToolTipText(LOCTEXT("ScriptTip",
							"A Dialog Script, or a Dialog Suite to run several dialogues in every column."))
						.ObjectPath_Lambda([this]
						{
							return DialogSource.IsValid() ? DialogSource->GetPathName() : FString();
						})
						.OnObjectChanged_Lambda([this](const FAssetData& Asset)
						{
							UObject* Picked = Asset.GetAsset();
							DialogSource.Reset(FTryllDialogLabDialogues::IsAcceptedSource(Picked) ? Picked : nullptr);
							RebuildStage();
						})
				]
			]

			// Seeded lines that break a global check: not counted (global checks skip seeded
			// turns), so shown before Run. The runner repeats them in the run's issues.
			+ SVerticalBox::Slot().AutoHeight().Padding(122.0f, 0.0f, 0.0f, 2.0f)
			[
				SNew(STextBlock)
					.Text_Lambda([this]
					{
						const TArray<const UTryllDialogScriptAsset*> Scripts = SourceScripts();
						TArray<FString> Lines;
						for (int32 D = 0; D < Scripts.Num(); ++D)
						{
							const FString Prefix = FTryllDialogLabDialogues::Prefix(D, Scripts[D]->GetName(), Scripts.Num());
							for (const FString& Warn : FTryllDialogScriptValidation::CollectSeededGlobalCheckWarnings(
								FTryllLabScriptSnapshot::FromAsset(Scripts[D])))
							{
								Lines.Add(TEXT("! ") + Prefix + Warn);
							}
						}
						return FText::FromString(FString::Join(Lines, TEXT("\n")));
					})
					.Visibility_Lambda([this]
					{
						for (const UTryllDialogScriptAsset* Script : SourceScripts())
						{
							if (FTryllDialogScriptValidation::CollectSeededGlobalCheckWarnings(
								FTryllLabScriptSnapshot::FromAsset(Script)).Num() > 0)
							{
								return EVisibility::Visible;
							}
						}
						return EVisibility::Collapsed;
					})
					.ColorAndOpacity(FSlateColor(FTryllEditorStyle::ColWarn))
					.AutoWrapText(true)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			]

			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()[FieldLabel(LOCTEXT("RepeatsLabel", "Repeats / base seed"))]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(60.0f)
					[
						SNew(SNumericEntryBox<int32>)
							.Value_Lambda([this] { return TOptional<int32>(Experiment.Repeats); })
							.ToolTipText(LOCTEXT("RepeatsTip",
								"How many seeds per variant. Values below 1 are raised to 1."))
							.OnValueCommitted_Lambda([this](int32 Value, ETextCommit::Type)
							{
								Experiment.Repeats = FMath::Max(1, Value);
								RebuildStage();
							})
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(60.0f)
					[
						SNew(SNumericEntryBox<int32>)
							.Value_Lambda([this] { return TOptional<int32>(Experiment.BaseSeed); })
							// Never 0: SamplingParams treats seed 0 as "random", which would
							// silently defeat every reproducibility guarantee the Lab offers.
							.ToolTipText(LOCTEXT("BaseSeedTip",
								"Must be at least 1. Seed 0 means random on the wire; the Lab raises it to 1."))
							.OnValueCommitted_Lambda([this](int32 Value, ETextCommit::Type)
							{
								Experiment.BaseSeed = FMath::Max(1, Value);
								RebuildStage();
							})
					]
				]
			]

			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
					.Text(LOCTEXT("VariantsHint", "Variants — edit in the Variants tab."))
					.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Faint))
			]
		];
}

TSharedRef<SWidget> STryllDialogLabPanel::BuildSegmented(
	const TArray<FText>& Labels,
	TFunction<int32()> GetActive,
	TFunction<void(int32)> SetActive,
	TFunction<bool()> IsEnabled)
{
	return FTryllEditorStyle::Segmented(
		Labels, MoveTemp(GetActive), MoveTemp(SetActive), MoveTemp(IsEnabled));
}

TSharedRef<SWidget> STryllDialogLabPanel::BuildToolbar()
{
	return SNew(SBorder)
		.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::Panel2))
		.Padding(FMargin(10.0f, 5.0f))
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SButton)
					.Text_Lambda([this]
					{
						return FText::Format(LOCTEXT("RunFmt", "▶ Run {0} cells"),
							FText::AsNumber(PlannedCells()));
					})
					.IsEnabled_Lambda([this] { return !bRunning; })
					.OnClicked(this, &STryllDialogLabPanel::OnRunClicked)
			]

			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 10.0f, 0.0f)
			[
				SNew(SButton)
					.Text(LOCTEXT("Reset", "↺ Reset"))
					.OnClicked(this, &STryllDialogLabPanel::OnResetClicked)
			]

			+ SHorizontalBox::Slot().AutoWidth()
			[
				BuildSegmented(
					{LOCTEXT("StageDialog", "Dialog"),
				 FText::Format(LOCTEXT("StageVariantsFmt", "Variants ({0})"),
					FText::AsNumber(Experiment.Variants.Num()))},
					[this] { return static_cast<int32>(Stage); },
					[this](int32 Index)
					{
						Stage = static_cast<EStage>(Index);
						// Each stage scrolls independently; carrying the offset across a switch
						// lands you mid-list on arrival (6.16).
						VerticalOffset = 0.0f;
						RebuildStage();
					},
					[] { return true; })
			]

			+ SHorizontalBox::Slot().AutoWidth()
			[
				BuildSegmented(
					{LOCTEXT("ModeStatistics", "Statistics"), LOCTEXT("ModeGrid", "Grid")},
					[this] { return static_cast<int32>(ViewMode); },
					[this](int32 Index)
					{
						ViewMode = static_cast<EViewMode>(Index);
						RebuildStage();
					},
					[this] { return Stage == EStage::Dialog; })
			]

			+ SHorizontalBox::Slot().AutoWidth()
			[
				BuildSegmented(
					{LOCTEXT("Compact", "Compact"), LOCTEXT("Comfortable", "Comfortable"),
					 LOCTEXT("Full", "Full")},
					[this] { return Density; },
					[this](int32 Index) { Density = Index; RebuildStage(); },
					[this] { return Stage == EStage::Dialog; })
			]

			+ SHorizontalBox::Slot().AutoWidth()
			[
				BuildSegmented(
					{LOCTEXT("WidthS", "S"), LOCTEXT("WidthM", "M"), LOCTEXT("WidthL", "L")},
					[this]
					{
						return CellWidth <= FTryllEditorStyle::CellWidthSmall ? 0
							 : CellWidth <= FTryllEditorStyle::CellWidthMedium ? 1 : 2;
					},
					[this](int32 Index)
					{
						CellWidth = Index == 0 ? FTryllEditorStyle::CellWidthSmall
							: Index == 1 ? FTryllEditorStyle::CellWidthMedium
							: FTryllEditorStyle::CellWidthLarge;
						RebuildStage();
					},
					[this] { return Stage == EStage::Dialog; })
			]

			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(8, 0)
			[
				SAssignNew(StatusBar, STryllStatusBar)
			]

			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(STextBlock)
					.Text(this, &STryllDialogLabPanel::RunStatusText)
					.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
			]

			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SButton)
					.Text(LOCTEXT("Import", "Import…"))
					.ToolTipText(LOCTEXT("ImportTip",
						"Open a Dialog Lab JSONL v3 export as read-only results "
						"(does not recreate agents or config)"))
					.IsEnabled_Lambda([this] { return !bRunning; })
					.OnClicked(this, &STryllDialogLabPanel::OnImportClicked)
			]

			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(SButton)
					.Text(LOCTEXT("Export", "Export…"))
					.ToolTipText(LOCTEXT("ExportTip",
						"Write the completed (or imported) run as JSONL v3 — header plus one line per cell"))
					.IsEnabled_Lambda([this] { return !bRunning && CompletedRun.IsSet(); })
					.OnClicked(this, &STryllDialogLabPanel::OnExportClicked)
			]

			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton)
					// The label tracks visibility rather than assuming it: after a reload the
					// pane may already be collapsed (6.19).
					.Text_Lambda([this]
					{
						return bInspectorVisible
							? LOCTEXT("InspectorHide", "Inspector ▶")
							: LOCTEXT("InspectorShow", "Inspector ◀");
					})
					.OnClicked_Lambda([this]
					{
						bInspectorVisible = !bInspectorVisible;
						SaveLabConfig();
						return FReply::Handled();
					})
			]
		];
}

FReply STryllDialogLabPanel::OnExportClicked()
{
	if (!CompletedRun.IsSet())
	{
		return FReply::Handled();
	}
	TArray<FString> Chosen;
	if (!FTryllEditorPrompts::SaveFileDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			TEXT("Export Dialog Lab run"), FPaths::ProjectSavedDir(),
			TEXT("dialog-lab-run.jsonl"), TEXT("JSONL|*.jsonl"), EFileDialogFlags::None, Chosen)
		|| Chosen.Num() == 0)
	{
		return FReply::Handled();
	}

	const FString Jsonl = FTryllDialogLabExport::ExportToJsonl(CompletedRun.GetValue());
	if (!FFileHelper::SaveStringToFile(Jsonl, *Chosen[0]))
	{
		FTryllEditorPrompts::Open(EAppMsgType::Ok,
			FText::Format(LOCTEXT("ExportFailed", "Could not write {0}"),
				FText::FromString(Chosen[0])));
		return FReply::Handled();
	}
	UE_LOG(LogTryllEditor, Display, TEXT("[Tryll] Dialog Lab: exported %d samples to %s"),
		CompletedRun->Samples.Num(), *Chosen[0]);
	return FReply::Handled();
}

FReply STryllDialogLabPanel::OnImportClicked()
{
	if (bRunning)
	{
		return FReply::Handled();
	}
	TArray<FString> Chosen;
	if (!FTryllEditorPrompts::OpenFileDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			TEXT("Import Dialog Lab run"), FPaths::ProjectSavedDir(), FString(),
			TEXT("JSONL|*.jsonl"), EFileDialogFlags::None, Chosen)
		|| Chosen.Num() == 0)
	{
		return FReply::Handled();
	}

	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Chosen[0]))
	{
		if (ImportBanner.IsValid())
		{
			ImportBanner->Bind(
				FString::Printf(TEXT("Import failed — could not read %s"), *Chosen[0]),
				ETryllStatusSeverity::Error);
		}
		return FReply::Handled();
	}

	FTryllDialogLabRunResult Imported;
	FString Error;
	if (!FTryllDialogLabImport::TryLoad(Text, Imported, Error))
	{
		if (ImportBanner.IsValid())
		{
			ImportBanner->Bind(
				FString::Printf(TEXT("Import failed — %s"),
					Error.IsEmpty() ? TEXT("Unknown error") : *Error),
				ETryllStatusSeverity::Error);
		}
		return FReply::Handled();
	}

	if (ImportBanner.IsValid())
	{
		ImportBanner->Clear();
	}
	InstallRunResult(Imported, /*bImported*/ true);
	RebuildStage();
	return FReply::Handled();
}

TSharedRef<SWidget> STryllDialogLabPanel::BuildProgressRow()
{
	return SAssignNew(ProgressRow, SBorder)
		.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::Panel))
		.Padding(FMargin(10.0f, 5.0f))
		.Visibility_Lambda([this] { return bRunning ? EVisibility::Visible : EVisibility::Collapsed; })
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
					.Text_Lambda([this]
					{
						if (ProgressTotal <= 0)
						{
							return FText::FromString(SessionStatus);
						}
						const int32 Percent = FMath::RoundToInt(100.0f * ProgressDone / ProgressTotal);
						return FText::Format(LOCTEXT("ProgressFmt", "{0}/{1} turns · {2}%"),
							FText::AsNumber(ProgressDone), FText::AsNumber(ProgressTotal),
							FText::AsNumber(Percent));
					})
					.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
			[
				SNew(SProgressBar)
					.Percent_Lambda([this]
					{
						return ProgressTotal > 0
							? TOptional<float>(static_cast<float>(ProgressDone) / ProgressTotal)
							: TOptional<float>();
					})
			]
		];
}

int32 STryllDialogLabPanel::PlannedCells() const
{
	return Experiment.Variants.Num() * FMath::Max(1, Experiment.Repeats);
}

int32 STryllDialogLabPanel::PlannedTurns() const
{
	int32 Generated = 0;
	for (const UTryllDialogScriptAsset* Each : SourceScripts())
	{
		Generated += FMath::Max(0, Each->Steps.Num() - Each->SeedTurnCount);
	}
	return PlannedCells() * Generated;
}

TArray<const UTryllDialogScriptAsset*> STryllDialogLabPanel::SourceScripts() const
{
	TArray<const UTryllDialogScriptAsset*> Scripts;
	FString Ignored;
	if (!FTryllDialogLabDialogues::TryResolve(DialogSource.Get(), Scripts, Ignored))
	{
		Scripts.Reset();
	}
	return Scripts;
}

FText STryllDialogLabPanel::PreRunMessage() const
{
	TArray<FString> Missing;
	if (!DefaultAgent.IsValid())       { Missing.Add(TEXT("an Agent Component")); }
	if (!DialogSource.IsValid())       { Missing.Add(TEXT("a Dialog Script or Suite")); }
	if (Missing.Num() > 0)
	{
		return FText::FromString(
			FString::Printf(TEXT("Assign %s to run."), *FString::Join(Missing, TEXT(", "))));
	}
	const int32 Dialogues = SourceScripts().Num();
	const FString Across = Dialogues > 1
		? FString::Printf(TEXT(" across %d dialogues"), Dialogues)
		: FString();
	return FText::FromString(FString::Printf(
		TEXT("Press Run to execute %d cells (%d generated turns%s)."),
		PlannedCells(), PlannedTurns(), *Across));
}

void STryllDialogLabPanel::WarnIfInconsistent(const TCHAR* Where)
{
	if (!bRan || HasResults() || bWarnedInconsistentState)
	{
		return;
	}
	bWarnedInconsistentState = true;
	UE_LOG(LogTryllEditor, Warning,
		TEXT("[Tryll] Dialog Lab: results marked present but missing in %s (dataset=%s). ")
		TEXT("Showing the pre-run state. Please report the steps that led here."),
		Where, Dataset.IsSet() ? TEXT("ok") : TEXT("null"));
}

FText STryllDialogLabPanel::RunStatusText() const
{
	const_cast<STryllDialogLabPanel*>(this)->WarnIfInconsistent(TEXT("RunStatusText"));

	if (!HasResults())
	{
		return bRunning ? LOCTEXT("Running", "running…") : LOCTEXT("NotRun", "not run");
	}

	int32 Ok = 0;
	int32 CheckFail = 0;
	int32 Error = 0;
	int32 Skipped = 0;
	// Generated turns only, as on the Statistics tab: a scripted line has no checks to fail, so
	// counting it would always score it clean.
	int32 Total = 0;
	for (const TPair<FString, FTryllLabCellView>& Pair : Dataset->Cells)
	{
		if (Pair.Value.bSeeded)
		{
			continue;
		}
		++Total;
		switch (FTryllDialogLabDataset::StatusOf(Pair.Value))
		{
			case ETryllLabCellStatus::Ok:        ++Ok; break;
			case ETryllLabCellStatus::CheckFail: ++CheckFail; break;
			case ETryllLabCellStatus::Error:     ++Error; break;
			default:                             ++Skipped; break;
		}
	}
	TArray<FString> Parts;
	if (bResultsImported) { Parts.Add(TEXT("imported")); }
	Parts.Add(FString::Printf(TEXT("%d/%d cells"), Dataset->Samples.Num(), Dataset->Samples.Num()));
	Parts.Add(FString::Printf(TEXT("%d/%d turns clean"), Ok, Total));
	if (CheckFail > 0) { Parts.Add(FString::Printf(TEXT("%d check fail"), CheckFail)); }
	if (Error > 0)     { Parts.Add(FString::Printf(TEXT("%d error"), Error)); }
	if (Skipped > 0)   { Parts.Add(FString::Printf(TEXT("%d not run"), Skipped)); }
	return FText::FromString(FString::Join(Parts, TEXT(" · ")));
}

void STryllDialogLabPanel::RefreshAgentCandidates()
{
	AgentCandidates.Reset();
	if (GEditor == nullptr || GEditor->GetEditorWorldContext().World() == nullptr)
	{
		return;
	}
	for (TActorIterator<AActor> It(GEditor->GetEditorWorldContext().World()); It; ++It)
	{
		if (UTryllAgentComponent* Component = It->FindComponentByClass<UTryllAgentComponent>())
		{
			AgentCandidates.Add(Component);
		}
	}
}

UTryllAgentComponent* STryllDialogLabPanel::EffectiveAgent(const FTryllDialogLabVariant& Variant) const
{
	return Variant.AgentOverride.IsValid() ? Variant.AgentOverride.Get() : DefaultAgent.Get();
}

const FTryllGraphDescription* STryllDialogLabPanel::GraphOf(const UTryllAgentComponent* Agent) const
{
	if (Agent == nullptr)
	{
		return nullptr;
	}
	return Agent->WorkflowAsset != nullptr
		? &Agent->WorkflowAsset->Graph
		: nullptr;
}

void STryllDialogLabPanel::RebuildStage()
{
	// Every path through the content build — placeholders and the early-returning Variants stage
	// included — must end with the offsets restored, so the restore lives outside it.
	RebuildStageContent();
	ScheduleScrollRestore();
	// Every state change that matters already rebuilds, so persisting here means no mutation site
	// can forget to. The splitter drag is the one exception — it saves on its own.
	SaveLabConfig();
}

void STryllDialogLabPanel::RebuildStageContent()
{
	if (!StageHost.IsValid())
	{
		return;
	}
	StageHost->ClearChildren();
	ModelChoiceOptions.Reset();
	VerticalScroller.Reset();

	// The panel hands the inspector everything it draws from; nulls when there are no results,
	// because the panel owns its own empty state.
	RefreshInspector();

	auto Placeholder = [this](const FText& Text)
	{
		StageHost->AddSlot().FillHeight(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
				.Text(Text)
				.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Faint))
				.AutoWrapText(true)
		];
	};

	// On every stage and over old results alike: the user may press Run from the Variants tab, or
	// with a previous run's grid still on screen.
	if (!RunFailure.IsEmpty())
	{
		StageHost->AddSlot().AutoHeight().Padding(10.0f, 8.0f, 10.0f, 0.0f)
		[
			SNew(SBorder)
				.BorderImage(FTryllEditorStyle::Rounded(
					FTryllEditorStyle::RadiusCard, FTryllEditorStyle::WarnBorder, 1.0f))
				.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::WarnBg))
				.Padding(FMargin(8.0f, 6.0f))
				[
					SNew(STextBlock)
						.Text(FText::FromString(RunFailure))
						.ColorAndOpacity(FSlateColor(FTryllEditorStyle::ColWarn))
						.AutoWrapText(true)
				]
		];
	}

	// Variant authoring is an editing surface: usable before a first run and during one, so it
	// is checked ahead of the run-state gates.
	if (Stage == EStage::Variants)
	{
		BuildVariantsStage();
		return;
	}

	// A multi-dialogue run shows each dialogue as it lands; until the first one does, a placeholder.
	if (bRunning && !HasResults())
	{
		Placeholder(LOCTEXT("RunningPlaceholder", "Running…"));
		return;
	}
	if (!HasResults())
	{
		Placeholder(PreRunMessage());
		return;
	}

	if (bResultsImported)
	{
		StageHost->AddSlot().AutoHeight().Padding(10.0f, 8.0f, 10.0f, 0.0f)
		[
			SNew(SBorder)
				.BorderImage(FTryllEditorStyle::Rounded(
					FTryllEditorStyle::RadiusCard, FTryllEditorStyle::WarnBorder, 1.0f))
				.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::WarnBg))
				.Padding(FMargin(8.0f, 6.0f))
				[
					SNew(STextBlock)
						.Text(LOCTEXT("ImportedBanner",
							"Imported run (read-only results). Authoring fields above are for the next local run."))
						.ColorAndOpacity(FSlateColor(FTryllEditorStyle::ColWarn))
						.AutoWrapText(true)
				]
		];
	}
	if (!RunNotes.IsEmpty())
	{
		StageHost->AddSlot().AutoHeight().Padding(10.0f, 8.0f, 10.0f, 0.0f)
		[
			SNew(SBorder)
				.BorderImage(FTryllEditorStyle::Rounded(
					FTryllEditorStyle::RadiusCard, FTryllEditorStyle::WarnBorder, 1.0f))
				.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::WarnBg))
				.Padding(FMargin(8.0f, 6.0f))
				[
					SNew(STextBlock)
						.Text(FText::FromString(RunNotes))
						.ColorAndOpacity(FSlateColor(FTryllEditorStyle::ColWarn))
						.AutoWrapText(true)
				]
		];
	}

	HorizontalScrollers.Reset();

	if (ViewMode == EViewMode::Grid)
	{
		StageHost->AddSlot().AutoHeight()
		[
			BuildColumnHeader()
		];
		BuildGridStage();
	}
	else
	{
		BuildStatisticsStage();
	}

	if (bRunning && Dataset->Dialogues.Num() < Dataset->TotalDialogues)
	{
		StageHost->AddSlot().AutoHeight().Padding(10.0f, 6.0f)
		[
			SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("Running dialogue %d of %d…"),
					Dataset->Dialogues.Num() + 1, Dataset->TotalDialogues)))
				.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Faint))
		];
	}
}

void STryllDialogLabPanel::ScheduleScrollRestore()
{
	// Offsets can only be applied once layout has run.
	const float RestoreTo         = HorizontalOffset;
	const float RestoreToVertical = VerticalOffset;
	TWeakPtr<STryllDialogLabPanel> SelfWeak = SharedThis(this);
	GEditor->GetTimerManager()->SetTimerForNextTick([SelfWeak, RestoreTo, RestoreToVertical]
	{
		TSharedPtr<STryllDialogLabPanel> Self = SelfWeak.Pin();
		if (!Self.IsValid())
		{
			return;
		}
		Self->bSyncingHorizontal = true;
		for (const TSharedPtr<SScrollBox>& Scroller : Self->HorizontalScrollers)
		{
			if (Scroller.IsValid())
			{
				Scroller->SetScrollOffset(RestoreTo);
			}
		}
		Self->bSyncingHorizontal = false;

		// A jump has already asked the scroller for its target; restoring would undo it.
		if (Self->bJumpScrollPending)
		{
			Self->bJumpScrollPending = false;
		}
		else if (Self->VerticalScroller.IsValid())
		{
			Self->VerticalScroller->SetScrollOffset(RestoreToVertical);
		}
	});
}

void STryllDialogLabPanel::OpenFromChat(
	UTryllAgentComponent* SourceAgent, const TArray<FTryllChatHistoryEntry>& History)
{
	if (SourceAgent != nullptr)
	{
		DefaultAgent = SourceAgent;
	}

	// A transient instance, deliberately not an asset: nothing is written to disk unless the user
	// saves it explicitly (13.2).
	UTryllDialogScriptAsset* Captured = NewObject<UTryllDialogScriptAsset>(
		GetTransientPackage(), UTryllDialogScriptAsset::StaticClass(),
		MakeUniqueObjectName(GetTransientPackage(), UTryllDialogScriptAsset::StaticClass(),
			FName(FTryllDialogLabCompareFromChat::ScriptName)),
		RF_Transient);
	FTryllDialogLabCompareFromChat::ApplyToAsset(Captured, History);
	DialogSource.Reset(Captured);

	// Previous results describe a different script, so they go (13.3).
	Dataset.Reset();
	CompletedRun.Reset();
	PartialRun.Reset();
	bRan             = false;
	bResultsImported = false;
	RunFailure.Reset();
	RunNotes.Reset();
	SelectedKey.Reset();
	PinA.Reset();
	PinB.Reset();

	RefreshAgentCandidates();
	RebuildStage();
}

void STryllDialogLabPanel::DebugBindForTests(UObject* InSource,
	const TArray<FTryllDialogLabVariant>& Variants, int32 Repeats)
{
	DialogSource.Reset(InSource);
	Experiment.Variants = Variants;
	Experiment.Repeats = FMath::Max(1, Repeats);
	RebuildStage();
}

FTryllPanelSnapshot STryllDialogLabPanel::DebugSnapshotForTests() const
{
	auto StatusName = [](ETryllLabCellStatus Status) -> const TCHAR*
	{
		switch (Status)
		{
		case ETryllLabCellStatus::Ok:        return TEXT("Ok");
		case ETryllLabCellStatus::CheckFail: return TEXT("CheckFail");
		case ETryllLabCellStatus::Error:     return TEXT("Error");
		default:                             return TEXT("Skipped");
		}
	};
	FTryllPanelSnapshot Out;
	Out.OwnerPhase = EditorSubsystem ? EditorSubsystem->GetOwnerPhase() : ETryllOwnerPhase::Idle;
	Out.LocalState = bRunning ? TEXT("Running")
		: CompletedRun.IsSet() ? TEXT("Completed")
		: !RunFailure.IsEmpty() ? TEXT("Failed")
		: TEXT("Idle");
	if (StatusBar.IsValid())
	{
		Out.StatusCaption = StatusBar->DebugCaption();
		Out.StatusAction = StatusBar->DebugActionText();
	}
	Out.BannerText = RunFailure;
	if (CompletedRun.IsSet())
	{
		for (const FTryllLabSampleResult& Sample : CompletedRun->Samples)
		{
			for (const FTryllLabTurnRecord& Turn : Sample.Turns)
			{
				Out.Rows.Add(FString::Printf(TEXT("%s#%d:%d:%d:%s:%s"), *Sample.VariantLabel, Sample.Repeat,
					Turn.DialogueIndex, Turn.TurnIndex, StatusName(Turn.Status), *Turn.AnswerText));
			}
		}
	}
	Out.bLeaseHeld = bAcquiredConnection;
	Out.ListGeneration = static_cast<int32>(RunGeneration);
	return Out;
}

void STryllDialogLabPanel::LoadLabConfig()
{
	if (!bPersistLayout)
	{
		return;
	}
	const UTryllDialogLabConfig* Config = GetDefault<UTryllDialogLabConfig>();

	Stage             = static_cast<EStage>(FMath::Clamp(Config->Stage, 0, 1));
	ViewMode          = static_cast<EViewMode>(FMath::Clamp(Config->ViewMode, 0, 1));
	bStatsDelta       = Config->bStatsDelta;
	bStatsIntervals   = Config->bStatsIntervals;
	Density           = FMath::Clamp(Config->Density, 0, 2);
	CellWidth         = Config->CellWidth > 0.0f ? Config->CellWidth : FTryllEditorStyle::CellWidthMedium;
	InspectorWidth    = FMath::Max(Config->InspectorWidth, FTryllEditorStyle::InspectorMinWidth);
	bInspectorVisible = Config->bInspectorVisible;

	// Clamped here as well as in the UI: an ini is hand-editable, and base seed 0 means "random"
	// to the engine (4.2).
	Experiment.Repeats  = FMath::Max(1, Config->Repeats);
	Experiment.BaseSeed = FMath::Max(1, Config->BaseSeed);

	if (!Config->ScriptPath.IsEmpty())
	{
		UObject* Loaded = LoadObject<UObject>(nullptr, *Config->ScriptPath);
		DialogSource.Reset(FTryllDialogLabDialogues::IsAcceptedSource(Loaded) ? Loaded : nullptr);
	}
	if (!Config->DefaultAgentPath.IsEmpty())
	{
		DefaultAgent = FindObject<UTryllAgentComponent>(nullptr, *Config->DefaultAgentPath);
	}

	if (Config->Variants.Num() > 0)
	{
		TArray<FTryllDialogLabVariant> Restored;
		for (const FString& Line : Config->Variants)
		{
			FTryllDialogLabVariant Variant;
			if (FTryllDialogLabVariantCodec::Decode(Line, Variant))
			{
				Restored.Add(Variant);
			}
		}
		// A config that decodes to nothing leaves the constructor's Baseline in place rather than
		// presenting a variant list with no variants in it.
		if (Restored.Num() > 0)
		{
			Experiment.Variants = MoveTemp(Restored);
		}
	}
}

void STryllDialogLabPanel::SaveLabConfig() const
{
	if (!bPersistLayout)
	{
		return;
	}
	UTryllDialogLabConfig* Config = GetMutableDefault<UTryllDialogLabConfig>();

	Config->Stage             = static_cast<int32>(Stage);
	Config->ViewMode          = static_cast<int32>(ViewMode);
	Config->bStatsDelta       = bStatsDelta;
	Config->bStatsIntervals   = bStatsIntervals;
	Config->Density           = Density;
	Config->CellWidth         = CellWidth;
	Config->InspectorWidth    = InspectorWidth;
	Config->bInspectorVisible = bInspectorVisible;
	Config->Repeats           = Experiment.Repeats;
	Config->BaseSeed          = Experiment.BaseSeed;

	Config->ScriptPath = DialogSource.IsValid() ? DialogSource->GetPathName() : FString();
	Config->DefaultAgentPath =
		DefaultAgent.IsValid() ? DefaultAgent->GetPathName() : FString();

	Config->Variants.Reset();
	for (const FTryllDialogLabVariant& Variant : Experiment.Variants)
	{
		Config->Variants.Add(FTryllDialogLabVariantCodec::Encode(Variant));
	}

	Config->SaveConfig();
}

const TCHAR* STryllDialogLabPanel::ModelDefaultChoice = TEXT("(workflow default)");

TArray<FString> STryllDialogLabPanel::BuildModelChoices(
	const TArray<FString>& LanguageModels, const FString& CurrentOverride)
{
	// Two invariants: the current override is always present (a value the manifest no longer
	// lists stays selectable instead of silently resetting to default), and nothing repeats.
	TArray<FString> Choices;
	Choices.Add(ModelDefaultChoice);
	for (const FString& Name : LanguageModels)
	{
		if (!Name.IsEmpty() && !Choices.Contains(Name))
		{
			Choices.Add(Name);
		}
	}
	if (!CurrentOverride.IsEmpty() && !Choices.Contains(CurrentOverride))
	{
		Choices.Add(CurrentOverride);
	}
	return Choices;
}

void STryllDialogLabPanel::BuildVariantsStage()
{
	TSharedRef<SScrollBox> Vertical = MakeVerticalScroller();

	Vertical->AddSlot().Padding(FMargin(10.0f, 8.0f, 10.0f, 8.0f))
	[
		SNew(STextBlock)
			.Text(LOCTEXT("VariantsNote",
				"A variant is one configuration to compare, run on its own fresh agent. Give it an "
				"Agent to compare structural differences (a different model, context size, or "
				"workflow); leave that empty to use the panel's default agent and compare parameter "
				"overrides instead. Structural fields are absent from the override editor by design "
				"— ChangeParams rejects them (3006), so a different agent is how you change them."))
			.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Faint))
			.AutoWrapText(true)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
	];

	for (int32 Index = 0; Index < Experiment.Variants.Num(); ++Index)
	{
		Vertical->AddSlot().Padding(FMargin(10.0f, 0.0f, 10.0f, 8.0f))[BuildVariantCard(Index)];
	}

	Vertical->AddSlot().Padding(FMargin(10.0f, 0.0f, 10.0f, 24.0f))
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SButton)
				.Text(LOCTEXT("AddVariant", "+ Add variant"))
				.OnClicked_Lambda([this]
				{
					FTryllDialogLabVariant Added;
					Added.Label = FString::Printf(TEXT("Variant %d"), Experiment.Variants.Num() + 1);
					Added.Swatch = FTryllEditorStyle::VariantSwatch(Experiment.Variants.Num());
					Experiment.Variants.Add(Added);
					RebuildStage();
					return FReply::Handled();
				})
		]
	];

	StageHost->AddSlot().FillHeight(1.0f)[Vertical];
}

TSharedRef<SWidget> STryllDialogLabPanel::BuildVariantCard(int32 VariantIndex)
{
	FTryllDialogLabVariant& Variant = Experiment.Variants[VariantIndex];

	TSharedRef<SHorizontalBox> Head = SNew(SHorizontalBox);
	Head->AddSlot().AutoWidth().VAlign(VAlign_Center)
	[
		SNew(SColorBlock).Color(FTryllEditorStyle::VariantSwatch(VariantIndex)).Size(FVector2D(8.0f, 8.0f))
	];
	Head->AddSlot().FillWidth(1.0f).Padding(6.0f, 0.0f)
	[
		SNew(SEditableTextBox)
			.Text(FText::FromString(Variant.Label))
			.OnTextCommitted_Lambda([this, VariantIndex](const FText& Text, ETextCommit::Type)
			{
				if (Experiment.Variants.IsValidIndex(VariantIndex))
				{
					Experiment.Variants[VariantIndex].Label = Text.ToString();
				}
			})
	];
	// Removal is offered only when more than one variant exists.
	if (Experiment.Variants.Num() > 1)
	{
		Head->AddSlot().AutoWidth()
		[
			SNew(SButton)
				.Text(FText::FromString(TEXT("✕")))
				.OnClicked_Lambda([this, VariantIndex]
				{
					if (Experiment.Variants.IsValidIndex(VariantIndex))
					{
						Experiment.Variants.RemoveAt(VariantIndex);
					}
					RebuildStage();
					return FReply::Handled();
				})
		];
	}

	TSharedRef<SVerticalBox> Card = SNew(SVerticalBox);
	Card->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[Head];

	// Per-variant agent; when empty the hint names the effective default so it is never a guess.
	TSharedRef<SHorizontalBox> AgentRow = SNew(SHorizontalBox);
	AgentRow->AddSlot().AutoWidth().VAlign(VAlign_Center)
	[
		SNew(SBox).WidthOverride(46.0f)
		[
			SNew(STextBlock)
				.Text(LOCTEXT("VariantAgent", "Agent"))
				.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
		]
	];
	AgentRow->AddSlot().FillWidth(1.0f)
	[
		SNew(SComboBox<TWeakObjectPtr<UTryllAgentComponent>>)
			.OptionsSource(&AgentCandidates)
			.OnComboBoxOpening_Lambda([this] { RefreshAgentCandidates(); })
			.OnGenerateWidget_Lambda([](TWeakObjectPtr<UTryllAgentComponent> Item)
			{
				return SNew(STextBlock).Text(FText::FromString(
					(Item.IsValid() && Item->GetOwner() != nullptr)
						? Item->GetOwner()->GetActorNameOrLabel() : TEXT("None")));
			})
			// Rebuild rather than patch: the override pickers below are built against this
			// variant's graph, which just changed.
			.OnSelectionChanged_Lambda(
				[this, VariantIndex](TWeakObjectPtr<UTryllAgentComponent> Item, ESelectInfo::Type)
				{
					if (Experiment.Variants.IsValidIndex(VariantIndex))
					{
						Experiment.Variants[VariantIndex].AgentOverride = Item;
						Experiment.Variants[VariantIndex].AgentOverrideId =
							Item.IsValid() ? Item->GetPathName() : FString();
					}
					RebuildStage();
				})
			[
				SNew(STextBlock).Text_Lambda([this, VariantIndex]
				{
					if (!Experiment.Variants.IsValidIndex(VariantIndex))
					{
						return LOCTEXT("NoneAgent", "None");
					}
					const TWeakObjectPtr<UTryllAgentComponent>& Override =
						Experiment.Variants[VariantIndex].AgentOverride;
					return (Override.IsValid() && Override->GetOwner() != nullptr)
						? FText::FromString(Override->GetOwner()->GetActorNameOrLabel())
						: LOCTEXT("NoneAgent", "None");
				})
			]
	];
	if (!Variant.AgentOverride.IsValid())
	{
		const bool bHasDefault = DefaultAgent.IsValid() && DefaultAgent->GetOwner() != nullptr;
		AgentRow->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(6.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock)
				.Text(FText::FromString(bHasDefault
					? FString::Printf(TEXT("→ default: %s"),
						*DefaultAgent->GetOwner()->GetActorNameOrLabel())
					: FString(TEXT("→ no default assigned"))))
				.ColorAndOpacity(FSlateColor(bHasDefault
					? FTryllEditorStyle::Faint : FTryllEditorStyle::ColWarn))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
		];
	}
	Card->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[AgentRow];

	Card->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
	[
		SNew(SCheckBox)
			.IsChecked_Lambda([this, VariantIndex]
			{
				return Experiment.Variants.IsValidIndex(VariantIndex)
					&& Experiment.Variants[VariantIndex].bUseLiveTools
					? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
			.OnCheckStateChanged_Lambda([this, VariantIndex](ECheckBoxState State)
			{
				if (Experiment.Variants.IsValidIndex(VariantIndex))
				{
					Experiment.Variants[VariantIndex].bUseLiveTools =
						State == ECheckBoxState::Checked;
				}
			})
			.ToolTipText(LOCTEXT("UseLiveToolsTip",
				"Off (default): a tool call with no canned result CheckFails the cell — the "
				"model must not call that tool here — and later steps still run. On: an "
				"uncanned call may be answered by the scene agent's registered handler."))
			[
				SNew(STextBlock)
					.Text(LOCTEXT("UseLiveTools", "Use live tools"))
					.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			]
	];

	const UTryllAgentComponent* Agent = EffectiveAgent(Variant);
	const FTryllGraphDescription* Graph = GraphOf(Agent);
	if (Graph == nullptr || Graph->Nodes.Num() == 0)
	{
		Card->AddSlot().AutoHeight()
		[
			SNew(STextBlock)
				.Text(LOCTEXT("VariantNoGraph",
					"Assign an agent with a workflow — on this variant or as the panel default — "
					"to edit overrides."))
				.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Faint))
				.AutoWrapText(true)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
		];
	}
	else
	{
		// Overrides authored against another agent's workflow can name nodes this one lacks.
		// Flagging here means it is visible before a run is spent.
		TSet<FString> Known;
		for (const FTryllNodeDescription& Node : Graph->Nodes)
		{
			Known.Add(Node.Name);
		}
		TArray<FString> Stale;
		for (const FTryllLabParamOverride& Override : Variant.Overrides)
		{
			if (!Known.Contains(Override.NodeName) && !Stale.Contains(Override.NodeName))
			{
				Stale.Add(Override.NodeName);
			}
		}
		if (Stale.Num() > 0)
		{
			Card->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 4.0f)
			[
				SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(
						TEXT("! %s — not in this agent's workflow; these overrides will not be applied."),
						*FString::Join(Stale, TEXT(", ")))))
					.ColorAndOpacity(FSlateColor(FTryllEditorStyle::ColWarn))
					.AutoWrapText(true)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			];
		}

		Card->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[BuildModelRow(VariantIndex)];
		const TArray<const UTryllDialogScriptAsset*> Scripts = SourceScripts();
		TArray<FString> IntentWarnings;
		for (int32 D = 0; D < Scripts.Num(); ++D)
		{
			const FString Prefix = FTryllDialogLabDialogues::Prefix(D, Scripts[D]->GetName(), Scripts.Num());
			for (const FString& Warn : FTryllDialogScriptValidation::CollectIntentCheckWarnings(
				FTryllLabScriptSnapshot::FromAsset(Scripts[D]), Graph))
			{
				IntentWarnings.Add(Prefix + Warn);
			}
		}
		{
			for (const FString& Warn : IntentWarnings)
			{
				Card->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 4.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 4.0f, 0.0f)
					[
						SNew(SImage)
							.Image(FAppStyle::GetBrush("Icons.Warning"))
							.ColorAndOpacity(FSlateColor(FTryllEditorStyle::ColWarn))
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
							.Text(FText::FromString(Warn))
							.ColorAndOpacity(FSlateColor(FTryllEditorStyle::ColWarn))
							.AutoWrapText(true)
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					]
				];
			}
		}
		Card->AddSlot().AutoHeight()[BuildOverrideEditor(VariantIndex)];
	}

	return SNew(SBorder)
		.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::Panel))
		.Padding(FMargin(8.0f, 6.0f))
		[
			Card
		];
}

TSharedRef<SWidget> STryllDialogLabPanel::BuildModelRow(int32 VariantIndex)
{
	FTryllDialogLabVariant& Variant = Experiment.Variants[VariantIndex];
	const UTryllAgentComponent* Agent = EffectiveAgent(Variant);
	const FTryllGraphDescription* Graph = GraphOf(Agent);
	const FString AssetDefault = Graph != nullptr ? Graph->DefaultModelName.Name : FString();

	TArray<FString> LanguageModels;
	if (const UTryllModelManifest* Manifest = UTryllModelManifest::GetActive())
	{
		for (const FTryllModelManifestEntry& Entry : Manifest->Entries)
		{
			if (Entry.ModelType.Equals(TEXT("language"), ESearchCase::IgnoreCase))
			{
				LanguageModels.Add(Entry.Name);
			}
		}
	}

	TSharedPtr<TArray<TSharedPtr<FString>>> Options = MakeShared<TArray<TSharedPtr<FString>>>();
	for (const FString& Choice : BuildModelChoices(LanguageModels, Variant.ModelOverride))
	{
		Options->Add(MakeShared<FString>(Choice));
	}
	ModelChoiceOptions.Add(Options);

	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	Box->AddSlot().AutoHeight()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(118.0f)
			[
				SNew(STextBlock)
					.Text(LOCTEXT("ModelLabel", "Model"))
					.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f)
		[
			SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(Options.Get())
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
				{
					return SNew(STextBlock).Text(FText::FromString(Item.IsValid() ? *Item : FString()));
				})
				.OnSelectionChanged_Lambda(
					[this, VariantIndex](TSharedPtr<FString> Item, ESelectInfo::Type)
					{
						if (!Item.IsValid() || !Experiment.Variants.IsValidIndex(VariantIndex))
						{
							return;
						}
						Experiment.Variants[VariantIndex].ModelOverride =
							(*Item == ModelDefaultChoice) ? FString() : *Item;
						// Rebuild so the hint line and the pinned-node warning re-evaluate.
						RebuildStage();
					})
				[
					SNew(STextBlock).Text_Lambda([this, VariantIndex]
					{
						if (!Experiment.Variants.IsValidIndex(VariantIndex))
						{
							return FText::FromString(ModelDefaultChoice);
						}
						const FString& Override = Experiment.Variants[VariantIndex].ModelOverride;
						return FText::FromString(Override.IsEmpty() ? ModelDefaultChoice : Override);
					})
				]
		]
	];

	if (Variant.ModelOverride.IsEmpty())
	{
		Box->AddSlot().AutoHeight().Padding(122.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock)
				.Text(FText::FromString(AssetDefault.IsEmpty()
					? FString(TEXT("→ the workflow sets no default; every model-using node must pin its own"))
					: FString::Printf(TEXT("→ %s (from the workflow)"), *AssetDefault)))
				.ColorAndOpacity(FSlateColor(AssetDefault.IsEmpty()
					? FTryllEditorStyle::ColWarn : FTryllEditorStyle::Faint))
				.AutoWrapText(true)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
		];
	}
	else
	{
		// The override only reaches nodes that inherit the agent default. Saying so beats a run
		// where the user changes the model and nothing moves.
		TArray<FString> Pinned;
		if (Graph != nullptr)
		{
			for (const FTryllNodeDescription& Node : Graph->Nodes)
			{
				if (Node.Params == nullptr) { continue; }
				const FProperty* Property = Node.Params->GetClass()->FindPropertyByName(TEXT("ModelName"));
				const FStructProperty* AsStruct = CastField<FStructProperty>(Property);
				if (AsStruct == nullptr || AsStruct->Struct != FTryllModelName::StaticStruct())
				{
					continue;
				}
				const FTryllModelName* Value =
					AsStruct->ContainerPtrToValuePtr<FTryllModelName>(Node.Params);
				if (Value != nullptr && !Value->Name.IsEmpty())
				{
					Pinned.Add(FString::Printf(TEXT("%s (%s)"), *Node.Name, *Value->Name));
				}
			}
		}
		if (Pinned.Num() > 0)
		{
			Box->AddSlot().AutoHeight().Padding(122.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(
						TEXT("! %s pin their own model and will ignore this override."),
						*FString::Join(Pinned, TEXT(", ")))))
					.ColorAndOpacity(FSlateColor(FTryllEditorStyle::ColWarn))
					.AutoWrapText(true)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			];
		}
	}

	return Box;
}

TSharedRef<SWidget> STryllDialogLabPanel::BuildOverrideEditor(int32 VariantIndex)
{
	FTryllDialogLabVariant& Variant = Experiment.Variants[VariantIndex];
	const UTryllAgentComponent* Agent = EffectiveAgent(Variant);
	const FTryllGraphDescription* Graph = GraphOf(Agent);

	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

	Box->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
	[
		SNew(STextBlock)
			.Text(LOCTEXT("OverrideNote",
				"Model / context-size / graph-wiring fields are not offered here — they require a "
				"fresh agent (ChangeParams rejects them: 3006). To compare models or context sizes, "
				"assign a different Agent on this variant."))
			.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Faint))
			.AutoWrapText(true)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
	];

	// Existing overrides, each annotated when stale, not overridable, or now invalid.
	for (int32 OverrideIndex = 0; OverrideIndex < Variant.Overrides.Num(); ++OverrideIndex)
	{
		const FTryllLabParamOverride& Override = Variant.Overrides[OverrideIndex];

		FString Problem;
		const FTryllNodeDescription* Node = Graph != nullptr
			? Graph->Nodes.FindByPredicate([&Override](const FTryllNodeDescription& N)
				{ return N.Name == Override.NodeName; })
			: nullptr;
		if (Node == nullptr)
		{
			Problem = TEXT("not in this agent's workflow");
		}
		else if (Node->Params == nullptr)
		{
			Problem = TEXT("field not overridable on this node type");
		}
		else
		{
			FTryllOverridableField Field;
			if (!FTryllNodeFieldReflection::TryGetField(
					Node->Params->GetClass(), Override.FieldPath, Field))
			{
				Problem = TEXT("field not overridable on this node type");
			}
			else
			{
				Problem = FTryllNodeFieldReflection::ValidateValue(Field, Override.Value);
			}
		}

		const FString Text = Problem.IsEmpty()
			? FString::Printf(TEXT("%s · %s = %s"),
				*Override.NodeName, *Override.FieldPath, *Override.Value)
			: FString::Printf(TEXT("%s · %s = %s  ! %s"),
				*Override.NodeName, *Override.FieldPath, *Override.Value, *Problem);

		Box->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
					.Text(FText::FromString(Text))
					.ColorAndOpacity(FSlateColor(Problem.IsEmpty()
						? FTryllEditorStyle::Txt : FTryllEditorStyle::ColWarn))
					.AutoWrapText(true)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton)
					.Text(FText::FromString(TEXT("✕")))
					.OnClicked_Lambda([this, VariantIndex, OverrideIndex]
					{
						if (Experiment.Variants.IsValidIndex(VariantIndex)
							&& Experiment.Variants[VariantIndex].Overrides.IsValidIndex(OverrideIndex))
						{
							Experiment.Variants[VariantIndex].Overrides.RemoveAt(OverrideIndex);
						}
						RebuildStage();
						return FReply::Handled();
					})
			]
		];
	}

	if (Graph == nullptr)
	{
		return Box;
	}

	// Add row: node, field and the Add button on one line, the value editor on the line below
	// so a long field (a system prompt) gets the full width. Kept text-based so every kind
	// round-trips through the same string form the runner applies.
	TSharedPtr<TArray<TSharedPtr<FString>>> NodeOptions = MakeShared<TArray<TSharedPtr<FString>>>();
	for (const FTryllNodeDescription& Node : Graph->Nodes)
	{
		if (Node.Params != nullptr)
		{
			NodeOptions->Add(MakeShared<FString>(Node.Name));
		}
	}
	ModelChoiceOptions.Add(NodeOptions);

	const FString SelectedNode = PendingOverrideNode.IsEmpty() && NodeOptions->Num() > 0
		? *(*NodeOptions)[0]
		: PendingOverrideNode;

	// Fields are offered by display name (as Unity does) while identity stays the dotted path —
	// display names must never be used as identity (15.4).
	const FTryllNodeDescription* SelectedNodeDesc = Graph->Nodes.FindByPredicate(
		[&SelectedNode](const FTryllNodeDescription& N) { return N.Name == SelectedNode; });
	UTryllNodeParamsBase* SelectedParams =
		SelectedNodeDesc != nullptr ? SelectedNodeDesc->Params : nullptr;

	TArray<FTryllOverridableField> Fields;
	if (SelectedParams != nullptr)
	{
		Fields = FTryllNodeFieldReflection::EnumerateOverridableFields(SelectedParams->GetClass());
	}

	TSharedPtr<TArray<TSharedPtr<FString>>> FieldOptions = MakeShared<TArray<TSharedPtr<FString>>>();
	for (const FTryllOverridableField& Field : Fields)
	{
		FieldOptions->Add(MakeShared<FString>(Field.DisplayName));
	}
	ModelChoiceOptions.Add(FieldOptions);

	const FTryllOverridableField* SelectedField = nullptr;
	if (Fields.Num() > 0)
	{
		SelectedField = PendingOverrideField.IsEmpty()
			? &Fields[0]
			: Fields.FindByPredicate([this](const FTryllOverridableField& F)
				{ return F.FieldPath == PendingOverrideField; });
		if (SelectedField == nullptr)
		{
			SelectedField = &Fields[0];
		}
	}

	// An override starts from the node's current authored value rather than from zero (15.14).
	// Re-prefilled whenever the selected field changes, which is what PendingValuePath tracks.
	if (SelectedField != nullptr && SelectedParams != nullptr
		&& PendingValuePath != SelectedNode + TEXT("/") + SelectedField->FieldPath)
	{
		PendingValuePath      = SelectedNode + TEXT("/") + SelectedField->FieldPath;
		PendingOverrideField  = SelectedField->FieldPath;
		PendingOverrideValue  = FTryllNodeFieldReflection::GetValue(SelectedParams, *SelectedField);
		OverrideAddError.Reset();
	}

	Box->AddSlot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.0f)
		[
			SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(NodeOptions.Get())
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
				{
					return SNew(STextBlock).Text(FText::FromString(Item.IsValid() ? *Item : FString()));
				})
				.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type)
				{
					if (Item.IsValid())
					{
						PendingOverrideNode  = *Item;
						PendingOverrideField.Reset();
						RebuildStage();
					}
				})
				[
					SNew(STextBlock).Text_Lambda([this, SelectedNode]
					{
						return FText::FromString(SelectedNode);
					})
				]
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f, 0.0f)
		[
			SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(FieldOptions.Get())
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
				{
					return SNew(STextBlock).Text(FText::FromString(Item.IsValid() ? *Item : FString()));
				})
				.OnSelectionChanged_Lambda([this, Fields](TSharedPtr<FString> Item, ESelectInfo::Type)
				{
					if (!Item.IsValid())
					{
						return;
					}
					// The list carries display names; identity is the dotted path behind it.
					const FTryllOverridableField* Picked = Fields.FindByPredicate(
						[&Item](const FTryllOverridableField& F) { return F.DisplayName == *Item; });
					if (Picked != nullptr)
					{
						PendingOverrideField = Picked->FieldPath;
						// Changing the field resets the value editor, which re-prefills on rebuild.
						PendingValuePath.Reset();
						RebuildStage();
					}
				})
				[
					SNew(STextBlock).Text_Lambda(
						[SelectedDisplay = SelectedField != nullptr
							? SelectedField->DisplayName : FString(TEXT("(field)"))]
						{
							return FText::FromString(SelectedDisplay);
						})
				]
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(4.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("AddOverride", "Add override"))
				.OnClicked_Lambda([this, VariantIndex, SelectedNode, Graph]
				{
					if (!Experiment.Variants.IsValidIndex(VariantIndex)
						|| PendingOverrideField.IsEmpty())
					{
						return FReply::Handled();
					}

					const FTryllNodeDescription* Node = Graph->Nodes.FindByPredicate(
						[&SelectedNode](const FTryllNodeDescription& N)
						{ return N.Name == SelectedNode; });
					if (Node == nullptr || Node->Params == nullptr)
					{
						return FReply::Handled();
					}

					FTryllOverridableField Field;
					if (!FTryllNodeFieldReflection::TryGetField(
							Node->Params->GetClass(), PendingOverrideField, Field))
					{
						return FReply::Handled();
					}

					// An optional field accepts empty as "clear the override" (15.6); a scalar
					// does not, and ValidateValue is what says so.

					OverrideAddError = FTryllNodeFieldReflection::ValidateValue(
						Field, PendingOverrideValue);
					if (!OverrideAddError.IsEmpty())
					{
						RebuildStage();
						return FReply::Handled();
					}

					// Replace rather than append when this node+path already has an override.
					TArray<FTryllLabParamOverride>& Overrides =
						Experiment.Variants[VariantIndex].Overrides;
					FTryllLabParamOverride* Existing = Overrides.FindByPredicate(
						[&SelectedNode, this](const FTryllLabParamOverride& O)
						{
							return O.NodeName == SelectedNode && O.FieldPath == PendingOverrideField;
						});
					if (Existing != nullptr)
					{
						Existing->Value = PendingOverrideValue;
					}
					else
					{
						FTryllLabParamOverride Added;
						Added.NodeName  = SelectedNode;
						Added.FieldPath = PendingOverrideField;
						Added.Value     = PendingOverrideValue;
						Overrides.Add(Added);
					}
					RebuildStage();
					return FReply::Handled();
				})
		]
	];

	Box->AddSlot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
	[
		BuildOverrideValueEditor(SelectedField)
	];

	if (!OverrideAddError.IsEmpty())
	{
		Box->AddSlot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock)
				.Text(FText::FromString(OverrideAddError))
				.ColorAndOpacity(FSlateColor(FTryllEditorStyle::ColWarn))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
		];
	}

	return Box;
}

TSharedRef<SWidget> STryllDialogLabPanel::BuildOverrideValueEditor(
	const FTryllOverridableField* Field)
{
	if (Field == nullptr)
	{
		return SNew(SEditableTextBox).IsEnabled(false);
	}

	switch (Field->Kind)
	{
		case ETryllFieldKind::Bool:
			return SNew(SCheckBox)
				.IsChecked_Lambda([this]
				{
					return PendingOverrideValue == TEXT("true")
						? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
				.OnCheckStateChanged_Lambda([this](ECheckBoxState State)
				{
					PendingOverrideValue = State == ECheckBoxState::Checked
						? TEXT("true") : TEXT("false");
				});

		case ETryllFieldKind::Enum:
		{
			// Values are matched by name, so the list is the enum's names verbatim (15.11).
			TSharedPtr<TArray<TSharedPtr<FString>>> Names =
				MakeShared<TArray<TSharedPtr<FString>>>();
			if (Field->EnumType != nullptr)
			{
				const int32 Count = Field->EnumType->NumEnums();
				for (int32 Index = 0; Index < Count; ++Index)
				{
					// The trailing _MAX sentinel is not a value a user may pick.
					if (Field->EnumType->HasMetaData(TEXT("Hidden"), Index))
					{
						continue;
					}
					const FString Name = Field->EnumType->GetNameStringByIndex(Index);
					if (!Name.IsEmpty() && !Name.EndsWith(TEXT("_MAX")))
					{
						Names->Add(MakeShared<FString>(Name));
					}
				}
			}
			ModelChoiceOptions.Add(Names);

			if (Names->Num() > 0
				&& !Names->ContainsByPredicate([this](const TSharedPtr<FString>& N)
					{ return N.IsValid() && *N == PendingOverrideValue; }))
			{
				PendingOverrideValue = *(*Names)[0];
			}

			return SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(Names.Get())
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
				{
					return SNew(STextBlock)
						.Text(FText::FromString(Item.IsValid() ? *Item : FString()));
				})
				.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type)
				{
					if (Item.IsValid())
					{
						PendingOverrideValue = *Item;
					}
				})
				[
					SNew(STextBlock).Text_Lambda([this]
					{
						return FText::FromString(PendingOverrideValue);
					})
				];
		}

		case ETryllFieldKind::Int:
			return SNew(SNumericEntryBox<int32>)
				.AllowSpin(false)
				.Value_Lambda([this]
				{
					int32 Parsed = 0;
					return LexTryParseString(Parsed, *PendingOverrideValue)
						? TOptional<int32>(Parsed) : TOptional<int32>();
				})
				.OnValueChanged_Lambda([this](int32 NewValue)
				{
					// Invariant-culture by construction: LexToString never localises (15.7).
					PendingOverrideValue = LexToString(NewValue);
				});

		case ETryllFieldKind::Float:
			return SNew(SNumericEntryBox<float>)
				.AllowSpin(false)
				.Value_Lambda([this]
				{
					float Parsed = 0.0f;
					return LexTryParseString(Parsed, *PendingOverrideValue)
						? TOptional<float>(Parsed) : TOptional<float>();
				})
				.OnValueChanged_Lambda([this](float NewValue)
				{
					PendingOverrideValue = LexToString(NewValue);
				});

		case ETryllFieldKind::MultilineString:
			// Wraps to the row width and scrolls past ~10 lines, as the Unity editor does.
			return SNew(SBox).MinDesiredHeight(40.0f).MaxDesiredHeight(160.0f)
				[
					SNew(SMultiLineEditableTextBox)
						.Text_Lambda([this] { return FText::FromString(PendingOverrideValue); })
						.AutoWrapText(true)
						.OnTextChanged_Lambda([this](const FText& Text)
						{
							PendingOverrideValue = Text.ToString();
						})
				];

		default:
			return SNew(SEditableTextBox)
				.Text_Lambda([this] { return FText::FromString(PendingOverrideValue); })
				.OnTextChanged_Lambda([this](const FText& Text)
				{
					PendingOverrideValue = Text.ToString();
				});
	}
}

TSharedRef<SScrollBox> STryllDialogLabPanel::MakeHorizontalScroller()
{
	const int32 Index = HorizontalScrollers.Num();
	TSharedRef<SScrollBox> Box = SNew(SScrollBox)
		.Orientation(Orient_Horizontal)
		.ScrollBarVisibility(EVisibility::Collapsed)
		.OnUserScrolled_Lambda([this, Index](float Offset)
		{
			if (HorizontalScrollers.IsValidIndex(Index))
			{
				SyncHorizontal(HorizontalScrollers[Index], Offset);
			}
		});
	HorizontalScrollers.Add(Box);
	return Box;
}

void STryllDialogLabPanel::SyncHorizontal(const TSharedPtr<SScrollBox>& From, float Offset)
{
	if (bSyncingHorizontal)
	{
		return;
	}
	bSyncingHorizontal = true;
	HorizontalOffset = Offset;
	for (const TSharedPtr<SScrollBox>& Scroller : HorizontalScrollers)
	{
		if (Scroller.IsValid() && Scroller != From)
		{
			Scroller->SetScrollOffset(Offset);
		}
	}
	bSyncingHorizontal = false;
}

TSharedRef<SWidget> STryllDialogLabPanel::BuildChip(
	const FText& Text, const FLinearColor& Foreground,
	const FLinearColor& Background, const FLinearColor& Border)
{
	// A forbid pattern can be far wider than the column. Without a bound the chip draws straight
	// over its neighbours — several long labels in one cell overlap into unreadable mush — so it
	// is capped at the cell's inner width and elides. The full text stays on the tooltip.
	return SNew(SBorder)
		.BorderImage(FTryllEditorStyle::Rounded(FTryllEditorStyle::RadiusChip, Border, 1.0f))
		.BorderBackgroundColor(FSlateColor(Background))
		.ToolTipText(Text)
		.Padding(FMargin(5.0f, 0.0f, 5.0f, 1.0f))
		[
			SNew(SBox)
				.MaxDesiredWidth(CellWidth - 28.0f)
				.Clipping(EWidgetClipping::ClipToBounds)
				[
					SNew(STextBlock)
						.Text(Text)
						.ColorAndOpacity(FSlateColor(Foreground))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
				]
		];
}

TSharedRef<SWidget> STryllDialogLabPanel::BuildTimingBar(const FTryllLabCellView& Cell, float Height)
{
	TSharedRef<SHorizontalBox> Bar = SNew(SHorizontalBox);

	int32 Total = 0;
	for (const FTryllLabRouteNode& Node : Cell.Route)
	{
		Total += FMath::Max(0, Node.Ms);
	}

	if (Total <= 0)
	{
		Bar->AddSlot().FillWidth(1.0f)
		[
			SNew(SColorBlock).Color(FTryllEditorStyle::Line2).Size(FVector2D(1.0f, Height))
		];
		return Bar;
	}

	for (const FTryllLabRouteNode& Node : Cell.Route)
	{
		if (Node.Ms <= 0)
		{
			continue;
		}
		Bar->AddSlot().FillWidth(static_cast<float>(Node.Ms))
		[
			SNew(SColorBlock)
				.Color(FTryllEditorStyle::NodeColor(Node.Type))
				.Size(FVector2D(1.0f, Height))
		];
	}
	return Bar;
}

TSharedRef<SWidget> STryllDialogLabPanel::BuildColumnHeader()
{
	TSharedRef<SHorizontalBox> VariantBand = SNew(SHorizontalBox);
	for (const FTryllLabVariantView& Variant : Dataset->Variants)
	{
		int32 SampleCount = 0;
		for (const FTryllLabSampleView& Sample : Dataset->Samples)
		{
			if (Sample.VariantId == Variant.Id)
			{
				++SampleCount;
			}
		}
		// The group spans exactly its repeat columns, gaps included.
		const float GroupWidth = SampleCount * CellWidth
			+ FMath::Max(0, SampleCount - 1) * FTryllEditorStyle::Gap;

		const FString Diff = Variant.AgentName.IsEmpty()
			? FString::Printf(TEXT("· %s"), *Variant.DiffLabel)
			: FString::Printf(TEXT("· %s · %s"), *Variant.AgentName, *Variant.DiffLabel);

		VariantBand->AddSlot().AutoWidth().Padding(0.0f, 0.0f, FTryllEditorStyle::Gap, 0.0f)
		[
			SNew(SBox).WidthOverride(GroupWidth)
			[
				SNew(SBorder)
					.BorderImage(FTryllEditorStyle::Rounded(
						FTryllEditorStyle::RadiusHeader, FTryllEditorStyle::Line2, 1.0f))
					.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::Panel2))
					.Padding(FMargin(8.0f, 2.0f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(SColorBlock).Color(Variant.Color).Size(FVector2D(8.0f, 8.0f))
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							.Padding(5.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
								.Text(FText::FromString(Variant.Label))
								.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Txt))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
							.Padding(6.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
								.Text(FText::FromString(Diff))
								.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Faint))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						]
					]
			]
		];
	}

	TSharedRef<SHorizontalBox> SampleBand = SNew(SHorizontalBox);
	for (const FTryllLabSampleView& Sample : Dataset->Samples)
	{
		const FString SampleId = Sample.Id;
		SampleBand->AddSlot().AutoWidth().Padding(0.0f, 0.0f, FTryllEditorStyle::Gap, 0.0f)
		[
			SNew(SBox).WidthOverride(CellWidth)
			[
				SNew(SBorder)
					.BorderImage(FTryllEditorStyle::Rounded(
						FTryllEditorStyle::RadiusHeader, FTryllEditorStyle::Line2, 1.0f))
					.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::Panel2))
					.Padding(FMargin(8.0f, 2.0f, 6.0f, 2.0f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock)
								.Text(FText::FromString(Sample.Id))
								.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Txt))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							.Padding(6.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
								.Text(FText::Format(LOCTEXT("SeedFmt", "seed {0}"),
									FText::AsNumber(Sample.Seed)))
								.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Faint))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f)[SNullWidget::NullWidget]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(SButton)
								.ToolTipText(LOCTEXT("PinTip", "Pin this column for the A/B prompt diff"))
								.ContentPadding(FMargin(4.0f, 0.0f))
								.ForegroundColor_Lambda([this, SampleId]
								{
									return (SampleId == PinA || SampleId == PinB)
										? FSlateColor(FTryllEditorStyle::Accent)
										: FSlateColor(FTryllEditorStyle::Faint);
								})
								.OnClicked_Lambda([this, SampleId]
								{
									PinColumn(SampleId);
									return FReply::Handled();
								})
								[
									SNew(STextBlock).Text(FText::FromString(TEXT("⊙")))
								]
						]
					]
			]
		];
	}

	// The header is the one scroller that shows its bar.
	TSharedRef<SScrollBox> Scroller = MakeHorizontalScroller();
	Scroller->SetScrollBarVisibility(EVisibility::Visible);
	Scroller->AddSlot()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[VariantBand]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)[SampleBand]
	];

	return SNew(SBox).Padding(FMargin(10.0f, 4.0f, 10.0f, 0.0f))[Scroller];
}

TSharedRef<SWidget> STryllDialogLabPanel::BuildBand(int32 TurnIndex)
{
	const FTryllLabTurnView& Turn = Dataset->Turns[TurnIndex];

	TSharedRef<SWrapBox> Chips = SNew(SWrapBox).UseAllottedSize(true);
	for (const FTryllLabCheck& Check : Turn.Checks)
	{
		Chips->AddSlot().Padding(0.0f, 2.0f, 4.0f, 0.0f)
		[
			BuildChip(FText::FromString(Check.Label), FTryllEditorStyle::Dim,
				FTryllEditorStyle::ChipBg, FTryllEditorStyle::Line2)
		];
	}

	return SNew(SBorder)
		.BorderImage(FTryllEditorStyle::Rounded(FTryllEditorStyle::RadiusBand))
		.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::UserBub))
		.Padding(FMargin(11.0f, 5.0f, 11.0f, 6.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("YOU · TURN %d%s"),
						Turn.StepIndex + 1, Turn.bSeeded ? TEXT(" · SEEDED") : TEXT(""))))
					.ColorAndOpacity(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.7f)))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
					.Text(FText::FromString(Turn.User))
					.ColorAndOpacity(FSlateColor(FTryllEditorStyle::BandText))
					.AutoWrapText(true)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
			[
				Chips
			]
		];
}

TSharedRef<SWidget> STryllDialogLabPanel::BuildCell(const FTryllLabCellView& Cell)
{
	const ETryllLabCellStatus Status = FTryllDialogLabDataset::StatusOf(Cell);
	const FString CellKey = Cell.Key;

	// Selection and pin state are bound rather than baked: Slate re-evaluates these each frame,
	// so clicking a cell changes no widget structure and the grid does not flicker.
	// Fill carries status; selection wins. Outline is pin identity only (7.10).
	auto BackgroundAttr = [this, Status, CellKey]
	{
		if (SelectedKey == CellKey)
		{
			return FSlateColor(FTryllEditorStyle::SelBub);
		}
		if (Status == ETryllLabCellStatus::Error)
		{
			return FSlateColor(FTryllEditorStyle::ErrCell);
		}
		if (Status == ETryllLabCellStatus::Skipped)
		{
			return FSlateColor(FTryllEditorStyle::SkipCell);
		}
		if (Status == ETryllLabCellStatus::CheckFail)
		{
			return FSlateColor(FTryllEditorStyle::WarnBg);
		}
		return FSlateColor(FTryllEditorStyle::AsstBub);
	};

	const FString CellSample = Cell.SampleId;
	auto OutlineAttr = [this, CellSample]
	{
		if (CellSample == PinA)
		{
			return FSlateColor(FTryllEditorStyle::Accent);
		}
		if (CellSample == PinB)
		{
			return FSlateColor(FTryllEditorStyle::ColWarn);
		}
		return FSlateColor(FTryllEditorStyle::Line2);
	};

	const bool bMuted = Status == ETryllLabCellStatus::Error
		|| Status == ETryllLabCellStatus::Skipped;

	const int32 MaxLines = FTryllEditorStyle::DensityLines[FMath::Clamp(Density, 0, 2)];

	TSharedRef<SWidget> Body = SNew(STextBlock)
		.Text(FText::FromString(Cell.Text))
		.ColorAndOpacity(FSlateColor(bMuted ? FTryllEditorStyle::Dim : FTryllEditorStyle::CellBody))
		.Font(bMuted
			? FCoreStyle::GetDefaultFontStyle("Italic", 9)
			: FCoreStyle::GetDefaultFontStyle("Regular", 9))
		.AutoWrapText(true);

	if (MaxLines > 0)
	{
		Body = SNew(SBox)
			.MaxDesiredHeight(MaxLines * FTryllEditorStyle::LineHeight)
			.Clipping(EWidgetClipping::ClipToBounds)
			[
				Body
			];
	}

	TSharedRef<SWrapBox> Footer = SNew(SWrapBox).UseAllottedSize(true);
	if (Status == ETryllLabCellStatus::Error)
	{
		Footer->AddSlot().Padding(0.0f, 0.0f, 4.0f, 0.0f)
		[
			BuildChip(LOCTEXT("CellError", "✕ ERROR"), FTryllEditorStyle::ColErr,
				FTryllEditorStyle::ErrBg, FTryllEditorStyle::ErrBorder)
		];
	}
	else if (Status == ETryllLabCellStatus::Skipped)
	{
		Footer->AddSlot().Padding(0.0f, 0.0f, 4.0f, 0.0f)
		[
			BuildChip(LOCTEXT("CellSkipped", "○ not run"), FTryllEditorStyle::Faint,
				FTryllEditorStyle::ChipBg, FTryllEditorStyle::Line2)
		];
	}
	else
	{
		Footer->AddSlot().Padding(0.0f, 0.0f, 4.0f, 0.0f)
		[
			BuildChip(FText::FromString(FString::Printf(TEXT("%.2fs"), Cell.Ms / 1000.0f)),
				FTryllEditorStyle::Dim, FTryllEditorStyle::ChipBg, FTryllEditorStyle::Line2)
		];
		Footer->AddSlot().Padding(0.0f, 0.0f, 4.0f, 0.0f)
		[
			BuildChip(FText::FromString(FString::Printf(TEXT("%dw"), Cell.Words)),
				FTryllEditorStyle::Dim, FTryllEditorStyle::ChipBg, FTryllEditorStyle::Line2)
		];

		bool bCanned = false;
		for (const FTryllLabRouteNode& Node : Cell.Route)
		{
			if (Node.Type == TEXT("CannedResponseNode"))
			{
				bCanned = true;
				break;
			}
		}
		if (bCanned)
		{
			Footer->AddSlot().Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				BuildChip(LOCTEXT("CellCanned", "canned"), FTryllEditorStyle::ColWarn,
					FTryllEditorStyle::WarnBg, FTryllEditorStyle::WarnBorder)
			];
		}

		if (Cell.Failed.Num() > 0)
		{
			for (const FTryllLabCheckOutcome& Failure : Cell.Failed)
			{
				Footer->AddSlot().Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					BuildChip(FText::FromString(TEXT("✗ ")
							+ (Failure.Label.IsEmpty() ? Failure.Kind : Failure.Label)),
						FTryllEditorStyle::ColWarn, FTryllEditorStyle::WarnBg, FTryllEditorStyle::WarnBorder)
				];
			}
		}
		else
		{
			Footer->AddSlot().Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				BuildChip(LOCTEXT("CellChecksOk", "✓ checks"), FTryllEditorStyle::ColOk,
					FTryllEditorStyle::OkBg, FTryllEditorStyle::OkBorder)
			];
		}

		if (MaxLines > 0 && FTryllEditorStyle::EstimateLines(Cell.Text, CellWidth) > MaxLines)
		{
			Footer->AddSlot().Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				BuildChip(LOCTEXT("CellMore", "… more"), FTryllEditorStyle::Faint,
					FTryllEditorStyle::ChipBg, FTryllEditorStyle::Line2)
			];
		}
	}

	TSharedRef<SVerticalBox> Inner = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[Body]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)[Footer];

	if (Status != ETryllLabCellStatus::Skipped)
	{
		Inner->AddSlot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
		[
			BuildTimingBar(Cell, 3.0f)
		];
	}

	const FString Key = Cell.Key;
	const FString SampleId = Cell.SampleId;

	return SNew(SBox).WidthOverride(CellWidth)
	[
		SNew(SBorder)
			.BorderImage_Lambda([OutlineAttr]
			{
				return FTryllEditorStyle::Rounded(
					FTryllEditorStyle::RadiusBand,
					OutlineAttr().GetSpecifiedColor(),
					1.0f);
			})
			.BorderBackgroundColor_Lambda(BackgroundAttr)
			.Padding(FMargin(9.0f, 6.0f, 9.0f, 5.0f))
			.OnMouseButtonDown(FPointerEventHandler::CreateSP(
				this, &STryllDialogLabPanel::OnCellMouseDown, Key, SampleId))
			[
				Inner
			]
	];
}

TSharedRef<SWidget> STryllDialogLabPanel::BuildCellRow(int32 TurnIndex)
{
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	for (const FTryllLabSampleView& Sample : Dataset->Samples)
	{
		const FString Key = FTryllDialogLabDataset::CellKey(Sample.Id, TurnIndex);
		if (const FTryllLabCellView* Cell = Dataset->Cells.Find(Key))
		{
			Row->AddSlot().AutoWidth().Padding(0.0f, 0.0f, FTryllEditorStyle::Gap, 0.0f)
			[
				BuildCell(*Cell)
			];
		}
	}

	TSharedRef<SScrollBox> Scroller = MakeHorizontalScroller();
	// A row would otherwise spend the wheel on its own columns until they hit the end, so
	// wheeling over the cells (most of the Grid) scrolled sideways first. The wheel goes to the
	// vertical list; columns scroll with the header's bar.
	Scroller->SetConsumeMouseWheel(EConsumeMouseWheel::Never);
	Scroller->AddSlot()[Row];
	return SNew(SBox).Padding(FMargin(10.0f, 0.0f))[Scroller];
}

TSharedRef<SScrollBox> STryllDialogLabPanel::MakeVerticalScroller()
{
	// Selecting a cell or opening a turn rebuilds the stage; without carrying this offset every
	// click snaps back to the top of the list (10.3).
	TSharedRef<SScrollBox> Box = SNew(SScrollBox)
		.Orientation(Orient_Vertical)
		.OnUserScrolled_Lambda([this](float Offset) { VerticalOffset = Offset; });
	VerticalScroller = Box;
	return Box;
}

void STryllDialogLabPanel::BuildGridStage()
{
	TSharedRef<SScrollBox> Vertical = MakeVerticalScroller();
	const bool bGrouped = Dataset->TotalDialogues > 1;
	for (int32 TurnIndex = 0; TurnIndex < Dataset->Turns.Num(); ++TurnIndex)
	{
		const FTryllLabTurnView& Turn = Dataset->Turns[TurnIndex];
		if (bGrouped && Turn.StepIndex == 0)
		{
			Vertical->AddSlot().Padding(FMargin(10.0f, 14.0f, 10.0f, 0.0f))
			[
				BuildDialogueHeader(Turn.DialogueIndex)
			];
		}
		if (bGrouped && CollapsedDialogues.Contains(Turn.DialogueIndex))
		{
			continue;
		}
		TSharedRef<SWidget> Band = BuildBand(TurnIndex);
		Vertical->AddSlot().Padding(FMargin(10.0f, 10.0f, 10.0f, 6.0f))[Band];
		Vertical->AddSlot()[BuildCellRow(TurnIndex)];
		if (TurnIndex == JumpTurn)
		{
			// Resolved by the scroller once it has geometry, so no wait on layout here.
			Vertical->ScrollDescendantIntoView(Band, /*InAnimateScroll*/ false, EDescendantScrollDestination::TopOrLeft, 10.0f);
			bJumpScrollPending = true;
		}
	}
	JumpTurn = -1;
	Vertical->AddSlot()[SNew(SBox).HeightOverride(24.0f)];

	StageHost->AddSlot().FillHeight(1.0f)[Vertical];
}

TSharedRef<SWidget> STryllDialogLabPanel::BuildDialogueHeader(const int32 DialogueIndex)
{
	const FTryllLabDialogueView& Dialogue = Dataset->Dialogues[DialogueIndex];
	const bool bCollapsed = CollapsedDialogues.Contains(DialogueIndex);

	// How many of its generated turns passed across all columns.
	int32 Generated = 0;
	int32 Passed    = 0;
	for (int32 TurnIndex = Dialogue.FirstTurn; TurnIndex < Dialogue.FirstTurn + Dialogue.TurnCount; ++TurnIndex)
	{
		if (Dataset->Turns[TurnIndex].bSeeded)
		{
			continue;
		}
		for (const FTryllLabSampleView& Sample : Dataset->Samples)
		{
			++Generated;
			const FTryllLabCellView* Cell =
				Dataset->Cells.Find(FTryllDialogLabDataset::CellKey(Sample.Id, TurnIndex));
			if (Cell != nullptr && FTryllDialogLabDataset::StatusOf(*Cell) == ETryllLabCellStatus::Ok)
			{
				++Passed;
			}
		}
	}
	const bool bAllOk = Passed == Generated;

	const FString Size = Dialogue.SeededTurns > 0
		? FString::Printf(TEXT("%d turns (%d seeded)"), Dialogue.TurnCount, Dialogue.SeededTurns)
		: FString::Printf(TEXT("%d turns"), Dialogue.TurnCount);

	return SNew(SBorder)
		.BorderImage(FTryllEditorStyle::Rounded(FTryllEditorStyle::RadiusCard, FTryllEditorStyle::Line, 1.0f))
		.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::Panel))
		.Padding(FMargin(6.0f, 5.0f, 10.0f, 5.0f))
		.Cursor(EMouseCursor::Hand)
		.ToolTipText(bCollapsed
			? LOCTEXT("ExpandDialogue", "Expand this dialogue")
			: LOCTEXT("CollapseDialogue", "Collapse this dialogue"))
		.OnMouseButtonDown_Lambda([this, DialogueIndex](const FGeometry&, const FPointerEvent&)
		{
			if (CollapsedDialogues.Remove(DialogueIndex) == 0)
			{
				CollapsedDialogues.Add(DialogueIndex);
			}
			RebuildStage();
			return FReply::Handled();
		})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(14.0f).HAlign(HAlign_Center)
				[
					SNew(STextBlock)
						.Text(FText::FromString(bCollapsed ? TEXT("▶") : TEXT("▼")))
						.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Faint))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(4.0f, 0.0f)
			[
				SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("DIALOGUE %d · %s"),
						DialogueIndex + 1, *Dialogue.Name)))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
					.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(STextBlock)
					.Text(FText::FromString(Size))
					.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				BuildChip(FText::FromString(FString::Printf(TEXT("%d/%d turns passed"), Passed, Generated)),
					bAllOk ? FTryllEditorStyle::ColOk : FTryllEditorStyle::ColWarn,
					bAllOk ? FTryllEditorStyle::OkBg : FTryllEditorStyle::WarnBg,
					bAllOk ? FTryllEditorStyle::OkBorder : FTryllEditorStyle::WarnBorder)
			]
		];
}

int32 STryllDialogLabPanel::NextVisibleTurn(const int32 From, const int32 Step) const
{
	for (int32 TurnIndex = From + Step; TurnIndex >= 0 && TurnIndex < Dataset->Turns.Num(); TurnIndex += Step)
	{
		if (ViewMode == EViewMode::Grid && Dataset->TotalDialogues > 1
			&& CollapsedDialogues.Contains(Dataset->Turns[TurnIndex].DialogueIndex))
		{
			continue;
		}
		return TurnIndex;
	}
	return From;
}

FReply STryllDialogLabPanel::OnCellMouseDown(
	const FGeometry& /*Geometry*/, const FPointerEvent& Event, FString Key, FString SampleId)
{
	if (Event.IsShiftDown())
	{
		PinColumn(SampleId);
	}
	SelectCell(Key);
	return FReply::Handled();
}

FReply STryllDialogLabPanel::OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (!HasResults() || SelectedKey.IsEmpty())
	{
		return SCompoundWidget::OnKeyDown(Geometry, Event);
	}
	const FTryllLabCellView* Cell = Dataset->Cells.Find(SelectedKey);
	if (Cell == nullptr)
	{
		return SCompoundWidget::OnKeyDown(Geometry, Event);
	}

	int32 SampleIndex = Dataset->Samples.IndexOfByPredicate(
		[Cell](const FTryllLabSampleView& S) { return S.Id == Cell->SampleId; });
	int32 TurnIndex = Cell->TurnIndex;
	if (SampleIndex == INDEX_NONE)
	{
		return SCompoundWidget::OnKeyDown(Geometry, Event);
	}

	const FKey Key = Event.GetKey();
	if      (Key == EKeys::Right) { SampleIndex = FMath::Min(Dataset->Samples.Num() - 1, SampleIndex + 1); }
	else if (Key == EKeys::Left)  { SampleIndex = FMath::Max(0, SampleIndex - 1); }
	else if (Key == EKeys::Down)  { TurnIndex   = NextVisibleTurn(TurnIndex, +1); }
	else if (Key == EKeys::Up)    { TurnIndex   = NextVisibleTurn(TurnIndex, -1); }
	else
	{
		return SCompoundWidget::OnKeyDown(Geometry, Event);
	}

	SelectCell(FTryllDialogLabDataset::CellKey(Dataset->Samples[SampleIndex].Id, TurnIndex));
	return FReply::Handled();
}

void STryllDialogLabPanel::SelectCell(const FString& Key)
{
	SelectedKey = Key;
	// Cell chrome is bound, so nothing structural changes — only the inspector needs new data.
	RefreshInspector();
}

void STryllDialogLabPanel::RefreshInspector()
{
	if (!InspectorView.IsValid())
	{
		return;
	}

	InspectorView->Render(FTryllTurnInspectionFactory::FromLab(
		HasResults() ? &Dataset.GetValue() : nullptr,
		SelectedKey,
		PinA,
		PinB,
		[this](const FString& Key)
		{
			if (Key == TEXT("A")) { PinA.Reset(); }
			else                  { PinB.Reset(); }
			RefreshInspector();
		}));
}

void STryllDialogLabPanel::PinColumn(const FString& SampleId)
{
	if (PinA == SampleId)      { PinA.Reset(); }
	else if (PinB == SampleId) { PinB.Reset(); }
	else if (PinA.IsEmpty())   { PinA = SampleId; }
	else if (PinB.IsEmpty())   { PinB = SampleId; }
	else                       { PinA = PinB; PinB = SampleId; }
	RefreshInspector();
}

FString STryllDialogLabPanel::ValidateBeforeRun(bool bScriptOnly) const
{
	if (!Settings.IsValid())
	{
		return TEXT("Assign Tryll Settings.");
	}

	TArray<const UTryllDialogScriptAsset*> Scripts;
	FString Error;
	if (!FTryllDialogLabDialogues::TryResolve(DialogSource.Get(), Scripts, Error))
	{
		return Error;
	}
	if (!FTryllDialogLabRunValidation::TryValidateScripts(Scripts, Error))
	{
		return Error;
	}

	if (bScriptOnly)
	{
		return FString();
	}

	for (const FTryllDialogLabVariant& Variant : Experiment.Variants)
	{
		if (!FTryllDialogLabRunValidation::TryValidateVariant(
				Variant, FTryllLabRunAgent::FromComponent(EffectiveAgent(Variant)), Scripts, Error))
		{
			return Error;
		}
	}

	return FString();
}

FReply STryllDialogLabPanel::OnRunClicked()
{
	if (bRunning)
	{
		return FReply::Handled();
	}

	RunFailure.Reset();
	RunNotes.Reset();

	// Unity checks settings and the script *before* adding the default variant: a Run clicked with
	// no settings must report that and leave the experiment untouched, not silently gain a variant.
	RunFailure = ValidateBeforeRun(/*bScriptOnly*/ true);
	if (RunFailure.IsEmpty())
	{
		if (Experiment.Variants.Num() == 0)
		{
			Experiment.Variants.Add(FTryllDialogLabVariant());
		}
		RunFailure = ValidateBeforeRun(/*bScriptOnly*/ false);
	}
	if (!RunFailure.IsEmpty())
	{
		UE_LOG(LogTryllEditor, Warning, TEXT("[Tryll] Dialog Lab: run not started. %s"), *RunFailure);
		RebuildStage();
		return FReply::Handled();
	}

	// Capture the plan before any connection wait, so live edits during a run affect neither
	// the run nor its export.
	PendingSnapshot = FTryllDialogLabRunSnapshot::Capture(
		Settings.Get(), DefaultAgent.Get(), SourceScripts(), Experiment,
		[this](const FTryllDialogLabVariant& Variant) { return EffectiveAgent(Variant); });

	PendingLiveAgents.Reset(Experiment.Variants.Num());
	for (const FTryllDialogLabVariant& Variant : Experiment.Variants)
	{
		PendingLiveAgents.Add(EffectiveAgent(Variant));
	}

	++RunGeneration;
	bResultsImported = false;
	CompletedRun.Reset();
	PartialRun.Reset();
	CollapsedDialogues.Reset();
	Dataset.Reset();
	bRan          = false;
	bRunning      = true;
	ProgressDone  = 0;
	ProgressTotal = 0;
	SessionStatus = FTryllEditorUiContract::StatusConnecting;
	RebuildStage();

	BeginSession();
	return FReply::Handled();
}

void STryllDialogLabPanel::BeginSession()
{
	if (EditorSubsystem == nullptr && GEditor != nullptr)
	{
		EditorSubsystem = GEditor->GetEditorSubsystem<UTryllEditorSubsystem>();
	}
	if (EditorSubsystem == nullptr)
	{
		RunFailure = TEXT("Shared editor subsystem unavailable");
		bRunning = false;
		RebuildStage();
		return;
	}

	if (!TryllEditorConnectionPolicy::ShouldBindAndAcquire(bAcquiredConnection))
	{
		if (EditorSubsystem->IsConnected())
		{
			HandleConnectionChanged(true);
		}
		return;
	}

	EditorSubsystem->OnConnectionChangedNative.AddSP(
		this, &STryllDialogLabPanel::HandleConnectionChanged);
	EditorSubsystem->OnCreateSessionCompleteNative.AddSP(
		this, &STryllDialogLabPanel::HandleCreateSessionComplete);

	bAcquiredConnection = true;
	EditorSubsystem->AcquireConnection();

	// Join-late: another consumer may already have brought the shared connection up.
	if (EditorSubsystem->IsConnected())
	{
		HandleConnectionChanged(true);
	}
}

void STryllDialogLabPanel::HandleConnectionChanged(bool bConnected)
{
	if (!bRunning)
	{
		return;
	}
	if (!bConnected)
	{
		const bool bRetriesRemaining =
			EditorSubsystem && EditorSubsystem->HasConnectRetriesRemaining();
		if (TryllEditorConnectionPolicy::ShouldWaitForOwnerRetry(bRunning, bRetriesRemaining))
		{
			SessionStatus = TEXT("Waiting for server…");
			RebuildStage();
			return;
		}
		RunFailure = TEXT("Disconnected");
		bRunning = false;
		TeardownSession();
		RebuildStage();
		return;
	}

	SessionStatus = TEXT("Configuring session…");
	if (EditorSubsystem->IsSessionReady())
	{
		HandleCreateSessionComplete(true);
		return;
	}
	EditorSubsystem->CreateSessionFromSettings();
}

void STryllDialogLabPanel::HandleCreateSessionComplete(bool bSuccess)
{
	if (!bRunning)
	{
		return;
	}
	if (!bSuccess)
	{
		RunFailure = TEXT("CreateSession failed");
		bRunning = false;
		TeardownSession();
		RebuildStage();
		return;
	}

	// Every registered model must be present before any agent is created — the same gate the
	// chat panel and Play use.
	SessionStatus = TEXT("Checking required models…");
	ModelGuard = MakeShared<FTryllModelDownloadGuard>();
	ModelGuard->Start(EditorSubsystem, FTryllModelDownloadGuard::FOnComplete::CreateSP(
		this, &STryllDialogLabPanel::HandleModelsReady));
}

void STryllDialogLabPanel::HandleModelsReady(bool bReady, FString Error)
{
	if (!bRunning)
	{
		return;
	}
	if (!bReady)
	{
		RunFailure = Error.IsEmpty() ? TEXT("Model download cancelled.") : Error;
		bRunning = false;
		TeardownSession();
		RebuildStage();
		return;
	}
	StartRun();
}

void STryllDialogLabPanel::StartRun()
{
	// Unity guards a null snapshot here; the value-struct analogue is one that carries no script,
	// which would otherwise run zero turns and report success.
	if (PendingSnapshot.TotalSteps() == 0)
	{
		RunFailure = TEXT("Run snapshot missing.");
		bRunning   = false;
		TeardownSession();
		RebuildStage();
		return;
	}

	SessionStatus = TEXT("Running…");
	Runner = MakeShared<FTryllDialogLabRunner>();
	Runner->Start(
		EditorSubsystem, PendingSnapshot,
		FTryllDialogLabRunner::FOnProgress::CreateSP(this, &STryllDialogLabPanel::HandleRunProgress),
		FTryllDialogLabRunner::FOnFinished::CreateSP(this, &STryllDialogLabPanel::HandleRunFinished),
		PendingLiveAgents,
		FTryllDialogLabRunner::FOnDialogueCompleted::CreateSP(
			this, &STryllDialogLabPanel::HandleDialogueCompleted));
}

void STryllDialogLabPanel::HandleDialogueCompleted(
	const FTryllDialogLabRunResult& Result, int32 CompletedDialogues)
{
	// Selection survives, so the reader is not thrown back to the top each time a dialogue lands.
	PartialRun = Result;
	Dataset    = FTryllDialogLabDataset::FromRunResult(Result, /*bInImported*/ false, CompletedDialogues);
	bRan       = true;
	RebuildStage();
}

void STryllDialogLabPanel::HandleRunProgress(int32 Done, int32 Total)
{
	ProgressDone  = Done;
	ProgressTotal = Total;
}

void STryllDialogLabPanel::HandleRunFinished(const FTryllDialogLabRunResult& Result)
{
	bRunning = false;
	TeardownSession();

	// Reset/Stop discards: partial rectangular results are never published to the UI.
	if (Result.Status == ETryllDialogLabRunStatus::Stopped)
	{
		PartialRun.Reset();
		Dataset.Reset();
		bRan = false;
		RebuildStage();
		return;
	}

	InstallRunResult(Result, /*bImported*/ false);
	RebuildStage();
}

void STryllDialogLabPanel::InstallRunResult(
	const FTryllDialogLabRunResult& Result, bool bImported)
{
	PartialRun.Reset();
	CompletedRun     = Result;
	bResultsImported = bImported;
	Dataset          = FTryllDialogLabDataset::FromRunResult(Result, bImported);
	bRan             = true;
	bRunning         = false;
	RunFailure.Reset();
	RunNotes = Result.Issues.Num() > 0 ? FString::Join(Result.Issues, TEXT("; ")) : FString();

	SelectedKey.Reset();
	PinA.Reset();
	PinB.Reset();
	CollapsedDialogues.Reset();
	StatsDialogue = -1;
}

void STryllDialogLabPanel::TeardownSession()
{
	ModelGuard.Reset();
	if (EditorSubsystem != nullptr)
	{
		EditorSubsystem->OnConnectionChangedNative.RemoveAll(this);
		EditorSubsystem->OnCreateSessionCompleteNative.RemoveAll(this);
		if (bAcquiredConnection)
		{
			// Release, never kill: the managed server self-exits after its idle grace, and other
			// editor windows may still be using it.
			EditorSubsystem->ReleaseConnection();
			bAcquiredConnection = false;
		}
	}
	RefreshOwnerChrome();
}

void STryllDialogLabPanel::HandleOwnerPhaseChanged(ETryllOwnerPhase /*Phase*/, const FString& /*Detail*/)
{
	RefreshOwnerChrome();
}

void STryllDialogLabPanel::RefreshOwnerChrome()
{
	if (!StatusBar.IsValid())
	{
		return;
	}

	const ETryllOwnerPhase Owner = EditorSubsystem
		? EditorSubsystem->GetOwnerPhase()
		: ETryllOwnerPhase::Idle;
	const FString Detail = EditorSubsystem ? EditorSubsystem->GetOwnerDetail() : FString();
	StatusBar->Bind(FTryllStatusPresentation::Compose(
		Owner, Detail, ETryllPanelOperation::None, ETryllEditorPanel::DialogLab));
}

FReply STryllDialogLabPanel::OnResetClicked()
{
	++RunGeneration;
	if (Runner.IsValid())
	{
		Runner->Stop();
		Runner.Reset();
	}
	TeardownSession();

	bRunning         = false;
	bRan             = false;
	bResultsImported = false;
	CompletedRun.Reset();
	PartialRun.Reset();
	Dataset.Reset();
	RunFailure.Reset();
	RunNotes.Reset();
	SelectedKey.Reset();
	PinA.Reset();
	PinB.Reset();
	ProgressDone  = 0;
	ProgressTotal = 0;

	RebuildStage();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
