// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "STryllModelManagerPanel.h"
#include "TryllEditorPrompts.h"
#include "TryllModelManagerPresentation.h"
#include "TryllStatusPresentation.h"
#include "UI/STryllServerModeBadge.h"
#include "UI/STryllStatusBar.h"
#include "UI/STryllBanner.h"

#include "TryllRuntimeSettings.h"
#include "TryllLanguages.h"
#include "TryllGraphDescription.h"
#include "TryllSubsystem.h"
#include "TryllEditorSubsystem.h"
#include "TryllEditorConnectionPolicy.h"
#include "TryllDocs.h"
#include "ModelManager/ModelManagerConfig.h"
#include "TryllEditorUiContract.h"
#include "TryllModelCatalogFile.h"
#include "TryllInspectorSplitMath.h"
#include "UI/STryllInspectorSplit.h"
#include "UI/TryllEditorStyle.h"
#include "Editor.h"
#include "TryllModelDownloadGuard.h"
#include "TryllPIEModelGuard.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMenuAnchor.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Types/ISlateMetaData.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Misc/Paths.h"
#include "Misc/MessageDialog.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"

#define LOCTEXT_NAMESPACE "STryllModelManagerPanel"

namespace
{
	const FLinearColor& ColLoaded      = FTryllEditorStyle::Loaded;
	const FLinearColor& ColDownloaded  = FTryllEditorStyle::Accent;
	const FLinearColor& ColDownloading = FTryllEditorStyle::ColWarn;
	const FLinearColor& ColMissing     = FTryllEditorStyle::ColErr;
	const FLinearColor& ColProd        = FTryllEditorStyle::ColOk;
	const FLinearColor& ColExp         = FTryllEditorStyle::ColWarn;
	const FLinearColor& Dim            = FTryllEditorStyle::Dim;
	const FLinearColor& SelBg          = FTryllEditorStyle::SelBub;

	FString RegPillText(ETryllModelRegistration R)
	{
		switch (R)
		{
		case ETryllModelRegistration::Production:   return TEXT("PROD");
		case ETryllModelRegistration::Experimental: return TEXT("EXP");
		default:                                    return TEXT("—");
		}
	}

	FLinearColor RegColor(ETryllModelRegistration R)
	{
		switch (R)
		{
		case ETryllModelRegistration::Production:   return ColProd;
		case ETryllModelRegistration::Experimental: return ColExp;
		default:                                    return Dim;
		}
	}

	FText RegLabel(ETryllModelRegistration R)
	{
		switch (R)
		{
		case ETryllModelRegistration::Production:   return LOCTEXT("RegProd", "Production");
		case ETryllModelRegistration::Experimental: return LOCTEXT("RegExp", "Experimental");
		default:                                    return LOCTEXT("RegNone", "Not registered");
		}
	}

	// Rounded chip/pill brushes (must outlive the widgets → function-local statics).
	const FSlateBrush* BadgeBrush()
	{
		static const FSlateRoundedBoxBrush Brush(FLinearColor::Transparent, 7.f, FTryllEditorStyle::Wash(FTryllEditorStyle::Dim, 0.7f), 1.f);
		return &Brush;
	}

	const FSlateBrush* RegPillBrush(ETryllModelRegistration R)
	{
		static const FSlateRoundedBoxBrush Prod(FLinearColor(ColProd.R, ColProd.G, ColProd.B, 0.12f), 7.f, FLinearColor(ColProd.R, ColProd.G, ColProd.B, 0.5f), 1.f);
		static const FSlateRoundedBoxBrush Exp (FLinearColor(ColExp.R,  ColExp.G,  ColExp.B,  0.12f), 7.f, FLinearColor(ColExp.R,  ColExp.G,  ColExp.B,  0.5f), 1.f);
		static const FSlateRoundedBoxBrush None(FLinearColor::Transparent, 7.f, FLinearColor(Dim.R, Dim.G, Dim.B, 0.5f), 1.f);
		switch (R)
		{
		case ETryllModelRegistration::Production:   return &Prod;
		case ETryllModelRegistration::Experimental: return &Exp;
		default:                                    return &None;
		}
	}

	TSharedRef<SWidget> MakeBadge(const FString& Text)
	{
		return SNew(SBorder).BorderImage(BadgeBrush()).Padding(FMargin(6, 1)).VAlign(VAlign_Center)
			[ SNew(STextBlock).Text(FText::FromString(Text)).ColorAndOpacity(FSlateColor(Dim)).Font(FCoreStyle::GetDefaultFontStyle("Regular", 8)) ];
	}

	/// Compact row marker for a non-Auto placement choice. Auto is omitted so
	/// the registration pills stay in one column; the name slot absorbs the tag.
	TSharedRef<SWidget> MakePlacementPill(ETryllMemoryPlacement Placement)
	{
		if (Placement == ETryllMemoryPlacement::Auto)
		{
			return SNullWidget::NullWidget;
		}
		return SNew(STextBlock)
			.Text(Placement == ETryllMemoryPlacement::Vram
				? LOCTEXT("PillVram", "VRAM") : LOCTEXT("PillRam", "RAM"))
			.ColorAndOpacity(FSlateColor(RegColor(ETryllModelRegistration::Production)))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8));
	}

	TSharedRef<SWidget> MakeRegPill(ETryllModelRegistration R)
	{
		return SNew(SBorder).BorderImage(RegPillBrush(R)).Padding(FMargin(7, 1)).HAlign(HAlign_Center).VAlign(VAlign_Center)
			[ SNew(STextBlock).Text(FText::FromString(RegPillText(R))).ColorAndOpacity(FSlateColor(RegColor(R))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 8)) ];
	}

	FText StatusLabel(ETryllModelStatus S)
	{
		switch (S)
		{
		case ETryllModelStatus::Loaded:      return LOCTEXT("StLoaded", "Loaded");
		case ETryllModelStatus::Downloaded:  return LOCTEXT("StDownloaded", "Downloaded");
		case ETryllModelStatus::Local:       return LOCTEXT("StLocal", "Local");
		case ETryllModelStatus::Downloading: return LOCTEXT("StDownloading", "Downloading");
		case ETryllModelStatus::Incomplete:  return LOCTEXT("StIncomplete", "Incomplete");
		default:                             return LOCTEXT("StAbsent", "Not downloaded");
		}
	}

	FLinearColor StatusColor(ETryllModelStatus S)
	{
		switch (S)
		{
		case ETryllModelStatus::Loaded:      return ColLoaded;
		case ETryllModelStatus::Downloaded:  return ColDownloaded;
		case ETryllModelStatus::Local:       return ColDownloaded;
		case ETryllModelStatus::Downloading: return ColDownloading;
		case ETryllModelStatus::Incomplete:  return ColDownloading;
		default:                             return ColMissing;
		}
	}

	FString TypeDisplay(const FString& T)
	{
		if (T == TEXT("embedding")) return TEXT("Embedding");
		if (T == TEXT("stt"))       return TEXT("Speech-to-text");
		if (T == TEXT("tts"))       return TEXT("Text-to-speech");
		if (T == TEXT("vad"))       return TEXT("Voice activity");
		return TEXT("Language");
	}

	FString AudienceLabel(const FString& A)
	{
		return FTryllModelManagerPresentation::AudienceOrUnset(A);
	}

	FString FmtSampling(const FTryllSamplingParams& S)
	{
		return FTryllModelManagerPresentation::FormatSampling(S);
	}
}

// ── Construction ──────────────────────────────────────────────────────────────

STryllModelManagerPanel::STryllModelManagerPanel() = default;

void STryllModelManagerPanel::Construct(const FArguments& InArgs)
{
	EditorSubsystemOverride = InArgs._EditorSubsystemOverride;
	bPersistLayout = InArgs._PersistLayout;
	AutofindSettings();
	StatusFilter.Add(ETryllModelStatus::Loaded);
	StatusFilter.Add(ETryllModelStatus::Downloaded);
	StatusFilter.Add(ETryllModelStatus::Local);
	StatusFilter.Add(ETryllModelStatus::Incomplete);
	StatusFilter.Add(ETryllModelStatus::Absent);
	for (const TCHAR* T : { TEXT("language"), TEXT("embedding"), TEXT("stt"), TEXT("tts"), TEXT("vad") }) TypeFilter.Add(T);
	for (const TCHAR* E : { TEXT("llama-cpp"), TEXT("sherpa-onnx"), TEXT("onnx-runtime") }) EngineFilter.Add(E);
	for (const TCHAR* A : { TEXT("public"), TEXT("experimental"), TEXT("internal"), TEXT("") }) AudienceFilter.Add(A);
	for (ETryllModelRegistration R : { ETryllModelRegistration::Production, ETryllModelRegistration::Experimental, ETryllModelRegistration::None }) RegFilter.Add(R);

	SortOptions = {
		MakeShared<FString>(TEXT("Name")), MakeShared<FString>(TEXT("Type")), MakeShared<FString>(TEXT("Size")),
		MakeShared<FString>(TEXT("Status")), MakeShared<FString>(TEXT("Registration")) };

	LoadLayoutConfig();

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 6)[ BuildToolbar() ]
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 0)
		[
			SAssignNew(Banner, STryllBanner)
		]
		+ SVerticalBox::Slot().AutoHeight()[ SNew(SSeparator) ]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SAssignNew(OuterSplitter, STryllInspectorSplit)
			.FixedSide(STryllInspectorSplit::EFixedSide::First)
			.PrimaryMin(FTryllEditorUiContract::ModelFilterMinWidth)
			.SecondaryMin(
				FTryllEditorUiContract::ModelListMinWidth
				+ FTryllInspectorSplitMath::DefaultHandleWidth
				+ FTryllEditorUiContract::ModelPropsMinWidth)
			.StoredWidth_Lambda([this] { return FilterWidth; })
			.OnStoredWidthChanged_Lambda([this](float NewWidth)
			{
				FilterWidth = NewWidth;
				SaveLayoutConfig();
			})
			.IsFixedPaneVisible(true)
			.PrimaryContent()
			[
				BuildFiltersPanel()
			]
			.SecondaryContent()
			[
				SAssignNew(InnerSplitter, STryllInspectorSplit)
				.FixedSide(STryllInspectorSplit::EFixedSide::Second)
				.PrimaryMin(FTryllEditorUiContract::ModelListMinWidth)
				.SecondaryMin(FTryllEditorUiContract::ModelPropsMinWidth)
				.StoredWidth_Lambda([this] { return PropsWidth; })
				.OnStoredWidthChanged_Lambda([this](float NewWidth)
				{
					PropsWidth = NewWidth;
					SaveLayoutConfig();
				})
				.IsFixedPaneVisible(true)
				.PrimaryContent()
				[
					BuildListPanel()
				]
				.SecondaryContent()
				[
					BuildPropertiesPanel()
				]
			]
		]
	];

	// Vova's flow: opening the window immediately starts the server and lists models.
	Start();
	HostColorsHandle = FTryllEditorStyle::OnHostColorsRefreshed.AddRaw(
		this, &STryllModelManagerPanel::HandleHostColorsRefreshed);
}

void STryllModelManagerPanel::HandleHostColorsRefreshed()
{
	RebuildAll();
	RefreshStatusChrome();
}

STryllModelManagerPanel::~STryllModelManagerPanel()
{
	if (HostColorsHandle.IsValid())
	{
		FTryllEditorStyle::OnHostColorsRefreshed.Remove(HostColorsHandle);
		HostColorsHandle.Reset();
	}
	SaveLayoutConfig();
	if (State != EState::Idle) Stop();
}

void STryllModelManagerPanel::LoadLayoutConfig()
{
	if (!bPersistLayout)
	{
		FilterWidth = FTryllEditorUiContract::ModelFilterDefaultWidth;
		PropsWidth = FTryllEditorUiContract::ModelPropsDefaultWidth;
		return;
	}
	const FTryllModelManagerConfig Config = FTryllModelManagerConfig::Load();
	FilterWidth = Config.FilterWidth;
	PropsWidth  = Config.PropsWidth;
}

void STryllModelManagerPanel::SaveLayoutConfig() const
{
	if (!bPersistLayout)
	{
		return;
	}
	FTryllModelManagerConfig Config = FTryllModelManagerConfig::Load();
	if (FTryllInspectorSplitMath::ShouldPersistWidth(
			FilterWidth, FTryllEditorUiContract::ModelFilterMinWidth))
	{
		Config.FilterWidth = FilterWidth;
	}
	if (FTryllInspectorSplitMath::ShouldPersistWidth(
			PropsWidth, FTryllEditorUiContract::ModelPropsMinWidth))
	{
		Config.PropsWidth = PropsWidth;
	}
	Config.Save();
}

// ── Layout: toolbar ─────────────────────────────────────────────────────────

TSharedRef<SWidget> STryllModelManagerPanel::BuildToolbar()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Title", "Tryll Model Manager"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10, 0, 0, 0)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("LiveNote", "live · server-backed"))
			.ColorAndOpacity(FSlateColor(Dim))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).HAlign(HAlign_Right).Padding(10, 0)
		[
			SAssignNew(StatusBar, STryllStatusBar)
			.OnAction_Lambda([this]
			{
				if (State == EState::Error || State == EState::Idle)
				{
					RetryOwner();
				}
			})
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 6, 0)
		[
			SNew(STryllServerModeBadge)
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0, 0, 0)
		[
			SAssignNew(DownloadsAnchor, SMenuAnchor)
			.Placement(MenuPlacement_BelowRightAnchor)
			.OnGetMenuContent_Lambda([this] { return BuildDownloadsPopover(); })
			.OnMenuOpenChanged_Lambda([this](bool bIsOpen) { if (!bIsOpen) DownloadsList.Reset(); })
			[
				SNew(SButton)
				.ToolTipText(LOCTEXT("DownloadsTip", "Show active and recent downloads"))
				.OnClicked_Lambda([this] { if (DownloadsAnchor.IsValid()) DownloadsAnchor->SetIsOpen(!DownloadsAnchor->IsOpen()); return FReply::Handled(); })
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox)
						.WidthOverride(16.0f)
						.HeightOverride(16.0f)
						[
							SNew(SImage)
							.Image(FAppStyle::GetBrush("Icons.Download"))
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(5, 0, 0, 0)
					[
						SNew(STextBlock)
						.Visibility_Lambda([this] { return ActiveDownloadCount() > 0 ? EVisibility::Visible : EVisibility::Collapsed; })
						.Text_Lambda([this] { return FText::AsNumber(ActiveDownloadCount()); })
						.ColorAndOpacity(FSlateColor(ColDownloading))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
					]
				]
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0, 0, 0)
		[
			SNew(SButton)
			.Text(LOCTEXT("DownloadAll", "Download all"))
			.ToolTipText(LOCTEXT("DownloadAllTip", "Download every not-downloaded model in the current list"))
			.OnClicked_Lambda([this] { OnDownloadAll(); return FReply::Handled(); })
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0, 0, 0)
		[
			SNew(SButton)
			.AddMetaData<FTagMetaData>(FTagMetaData(TryllEditorWidgetIds::Refresh))
			.Text(LOCTEXT("Refresh", "Refresh"))
			.OnClicked_Lambda([this]
			{
				OnRefreshClicked();
				return FReply::Handled();
			})
		];
}


// Filters panel: collapsible sections, per-value counts, project totals, reset.
TSharedRef<SWidget> STryllModelManagerPanel::BuildFiltersPanel()
{
	auto Row = [this](TFunction<bool()> IsOn, TFunction<void(bool)> Set, const FText& Label,
	                  TFunction<int32()> Count, FSlateColor LabelColor) -> TSharedRef<SWidget>
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([IsOn] { return IsOn() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this, Set](ECheckBoxState S) { Set(S == ECheckBoxState::Checked); RebuildList(); })
				[ SNew(STextBlock).Text(Label).ColorAndOpacity(LabelColor) ]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0, 0, 0)
			[ SNew(STextBlock).Text_Lambda([Count] { return FText::AsNumber(Count()); }).ColorAndOpacity(FSlateColor(Dim)).Font(FCoreStyle::GetDefaultFontStyle("Regular", 8)) ];
	};

	auto Section = [](const FText& Title, const TSharedRef<SWidget>& Body) -> TSharedRef<SWidget>
	{
		return SNew(SExpandableArea)
			.InitiallyCollapsed(false)
			.Padding(FMargin(14, 2, 6, 6))
			.HeaderContent()
			[ SNew(STextBlock).Text(Title).Font(FCoreStyle::GetDefaultFontStyle("Bold", 9)).ColorAndOpacity(FSlateColor(Dim)) ]
			.BodyContent()[ Body ];
	};

	auto TotalRow = [](const FText& Label, FSlateColor Col, TSharedPtr<STextBlock>& Out) -> TSharedRef<SWidget>
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)[ SNew(STextBlock).Text(Label).ColorAndOpacity(Col) ]
			+ SHorizontalBox::Slot().AutoWidth()[ SAssignNew(Out, STextBlock).Text(FText::FromString(TEXT("0"))).ColorAndOpacity(Col) ];
	};

	TSharedRef<SVerticalBox> TypeBody = SNew(SVerticalBox);
	const TArray<TPair<FString, FText>> TypeVals = {
		{ TEXT("language"),  LOCTEXT("TLang", "Language") },
		{ TEXT("embedding"), LOCTEXT("TEmb", "Embedding") },
		{ TEXT("stt"),       LOCTEXT("TStt", "Speech-to-text") },
		{ TEXT("tts"),       LOCTEXT("TTts", "Text-to-speech") },
		{ TEXT("vad"),       LOCTEXT("TVad", "Voice activity") } };
	for (const TPair<FString, FText>& P : TypeVals)
	{
		const FString Val = P.Key;
		TypeBody->AddSlot().AutoHeight().Padding(0, 1)[ Row(
			[this, Val] { return TypeFilter.Contains(Val); },
			[this, Val](bool b) { if (b) TypeFilter.Add(Val); else TypeFilter.Remove(Val); },
			P.Value,
			[this, Val] { int32 N = 0; for (const FTryllModelInfo& M : Models) if (M.ModelType == Val) ++N; return N; },
			FSlateColor::UseForeground()) ];
	}

	TSharedRef<SVerticalBox> EngBody = SNew(SVerticalBox);
	for (const TCHAR* E : { TEXT("llama-cpp"), TEXT("sherpa-onnx"), TEXT("onnx-runtime") })
	{
		const FString Val = E;
		EngBody->AddSlot().AutoHeight().Padding(0, 1)[ Row(
			[this, Val] { return EngineFilter.Contains(Val); },
			[this, Val](bool b) { if (b) EngineFilter.Add(Val); else EngineFilter.Remove(Val); },
			FText::FromString(Val),
			[this, Val] { int32 N = 0; for (const FTryllModelInfo& M : Models) if (M.Engine == Val) ++N; return N; },
			FSlateColor::UseForeground()) ];
	}

	TSharedRef<SVerticalBox> AudBody = SNew(SVerticalBox);
	const TArray<TPair<FString, FText>> AudVals = {
		{ TEXT("public"),       LOCTEXT("APublic", "Public") },
		{ TEXT("experimental"), LOCTEXT("AExp2", "Experimental") },
		{ TEXT("internal"),     LOCTEXT("AInt", "Internal") } };
	for (const TPair<FString, FText>& P : AudVals)
	{
		const FString Val = P.Key;
		AudBody->AddSlot().AutoHeight().Padding(0, 1)[ Row(
			[this, Val] { return AudienceFilter.Contains(Val); },
			[this, Val](bool b) { if (b) AudienceFilter.Add(Val); else AudienceFilter.Remove(Val); },
			P.Value,
			[this, Val] { int32 N = 0; for (const FTryllModelInfo& M : Models) if (M.Audience == Val) ++N; return N; },
			FSlateColor::UseForeground()) ];
	}

	TSharedRef<SVerticalBox> RegBody = SNew(SVerticalBox);
	for (ETryllModelRegistration R : { ETryllModelRegistration::Production, ETryllModelRegistration::Experimental, ETryllModelRegistration::None })
	{
		RegBody->AddSlot().AutoHeight().Padding(0, 1)[ Row(
			[this, R] { return RegFilter.Contains(R); },
			[this, R](bool b) { if (b) RegFilter.Add(R); else RegFilter.Remove(R); },
			RegLabel(R),
			[this, R] { int32 N = 0; for (const FTryllModelInfo& M : Models) if (RegOf(M.Name) == R) ++N; return N; },
			FSlateColor(RegColor(R))) ];
	}

	TSharedRef<SVerticalBox> StatBody = SNew(SVerticalBox);
	const TArray<TPair<ETryllModelStatus, FText>> StatVals = {
		{ ETryllModelStatus::Loaded,     LOCTEXT("FLoaded", "Loaded") },
		{ ETryllModelStatus::Downloaded, LOCTEXT("FDownloaded", "Downloaded") },
		{ ETryllModelStatus::Local,      LOCTEXT("FLocal", "Local") },
		{ ETryllModelStatus::Incomplete, LOCTEXT("FIncomplete", "Incomplete") },
		{ ETryllModelStatus::Absent,     LOCTEXT("FAbsent", "Not downloaded") } };
	for (const TPair<ETryllModelStatus, FText>& P : StatVals)
	{
		const ETryllModelStatus S = P.Key;
		StatBody->AddSlot().AutoHeight().Padding(0, 1)[ Row(
			[this, S] { return StatusFilter.Contains(S); },
			[this, S](bool b) { if (b) StatusFilter.Add(S); else StatusFilter.Remove(S); },
			P.Value,
			[this, S] { int32 N = 0; for (const FTryllModelInfo& M : Models) if (M.Status == S) ++N; return N; },
			FSlateColor::UseForeground()) ];
	}

	TSharedRef<SVerticalBox> TotBody = SNew(SVerticalBox);
	TotBody->AddSlot().AutoHeight().Padding(0, 1)[ TotalRow(LOCTEXT("ShipTot", "Ships in build"), FSlateColor(ColProd), TotalShip) ];
	TotBody->AddSlot().AutoHeight().Padding(0, 1)[ TotalRow(LOCTEXT("ProdTot", "Production"),     FSlateColor(ColProd), TotalProd) ];
	TotBody->AddSlot().AutoHeight().Padding(0, 1)[ TotalRow(LOCTEXT("ExpTot", "Experimental"),    FSlateColor(ColExp),  TotalExp) ];
	TotBody->AddSlot().AutoHeight().Padding(0, 1)[ TotalRow(LOCTEXT("OnDisk", "Downloaded"),       FSlateColor(Dim),     TotalDownloaded) ];
	TotBody->AddSlot().AutoHeight().Padding(0, 1)[ TotalRow(LOCTEXT("DiskUsage", "Disk usage"),    FSlateColor(Dim),     TotalDisk) ];

	return SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(0)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(10, 8)
			[ SNew(STextBlock).Text(LOCTEXT("FiltersHdr", "FILTERS")).Font(FCoreStyle::GetDefaultFontStyle("Bold", 9)).ColorAndOpacity(FSlateColor(Dim)) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(8, 2)
			[
				SAssignNew(SearchBox, SEditableTextBox)
				.HintText(LOCTEXT("SearchHint", "Filter models..."))
				.OnTextChanged_Lambda([this](const FText& T) { SearchText = T.ToString().ToLower(); RebuildList(); })
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()[ Section(LOCTEXT("MType", "Model type"), TypeBody) ]
				+ SScrollBox::Slot()[ Section(LOCTEXT("EngHdr", "Engine"), EngBody) ]
				+ SScrollBox::Slot()[ Section(LOCTEXT("AudHdr", "Audience"), AudBody) ]
				+ SScrollBox::Slot()[ Section(LOCTEXT("RegHdr", "Build registration"), RegBody) ]
				+ SScrollBox::Slot()[ Section(LOCTEXT("DlStatus", "Download status"), StatBody) ]
				+ SScrollBox::Slot()[ Section(LOCTEXT("Totals", "Project totals"), TotBody) ]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(8, 6)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(LOCTEXT("ResetFilters", "Reset filters"))
				.OnClicked_Lambda([this] { ResetFilters(); return FReply::Handled(); })
			]
		];
}

// Model list
TSharedRef<SWidget> STryllModelManagerPanel::BuildListPanel()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 4)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[ SAssignNew(CountText, STextBlock).Text(LOCTEXT("NoModels", "0 models")).ColorAndOpacity(FSlateColor(Dim)) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 5, 0)
			[ SNew(STextBlock).Text(LOCTEXT("SortLbl", "Sort")).ColorAndOpacity(FSlateColor(Dim)).Font(FCoreStyle::GetDefaultFontStyle("Regular", 8)) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&SortOptions)
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> In) { return SNew(STextBlock).Text(FText::FromString(In.IsValid() ? *In : FString())); })
				.OnSelectionChanged_Lambda([this](TSharedPtr<FString> In, ESelectInfo::Type)
				{
					const int32 Idx = SortOptions.IndexOfByKey(In);
					if (Idx != INDEX_NONE) { SortKey = static_cast<ESortKey>(Idx); RebuildList(); }
				})
				.InitiallySelectedItem(SortOptions.Num() > 0 ? SortOptions[0] : nullptr)
				[ SNew(STextBlock).Text_Lambda([this] { return FText::FromString(SortOptions.IsValidIndex((int32)SortKey) ? *SortOptions[(int32)SortKey] : FString(TEXT("Name"))); }) ]
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(4, 0, 4, 4)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			.Padding(2)
			[ SAssignNew(ListScroll, SScrollBox) ]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 2, 8, 6)
		[ BuildLegend() ];
}

TSharedRef<SWidget> STryllModelManagerPanel::BuildLegend()
{
	auto Item = [](const FText& Label, FLinearColor C) -> TSharedRef<SWidget>
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
			[ SNew(STextBlock).Text(FText::FromString(TEXT("●"))).ColorAndOpacity(FSlateColor(C)) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 12, 0)
			[ SNew(STextBlock).Text(Label).ColorAndOpacity(FSlateColor(Dim)).Font(FCoreStyle::GetDefaultFontStyle("Regular", 8)) ];
	};
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()[ Item(LOCTEXT("LegLoaded", "Loaded"), ColLoaded) ]
		+ SHorizontalBox::Slot().AutoWidth()[ Item(LOCTEXT("LegDownloaded", "Downloaded"), ColDownloaded) ]
		+ SHorizontalBox::Slot().AutoWidth()[ Item(LOCTEXT("LegLocal", "Local"), ColDownloaded) ]
		+ SHorizontalBox::Slot().AutoWidth()[ Item(LOCTEXT("LegAbsent", "Not downloaded"), ColMissing) ];
}

TSharedRef<SWidget> STryllModelManagerPanel::MakeListRow(int32 Index)
{
	const FTryllModelInfo& M = Models[Index];
	const bool bSel = (Index == SelectedIndex);
	const bool bShowPlacement = SupportsPlacement(M.Name)
		&& PlacementOf(M.Name) != ETryllMemoryPlacement::Auto;

	return SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(bSel ? FSlateColor(SelBg) : FSlateColor(FLinearColor::Transparent))
		.Padding(0)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ContentPadding(FMargin(9, 6))
			.OnClicked_Lambda([this, Index] { SelectRow(Index); return FReply::Handled(); })
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 9, 0)
				[ SNew(STextBlock).Text(FText::FromString(TEXT("●"))).ColorAndOpacity(FSlateColor(StatusColor(M.Status))) ]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[ SNew(STextBlock).Text(FText::FromString(M.Name)) ]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("%s · %s"), *TypeDisplay(M.ModelType), *M.Engine)))
						.ColorAndOpacity(FSlateColor(Dim))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0, 0, 0)
				[ SNew(STextBlock).Text(FText::FromString(FTryllModelManagerPresentation::FmtDecimalBytes(M.SizeBytes))).ColorAndOpacity(FSlateColor(Dim)) ]
				// Placement sits to the left of the registration pill so
				// Production / Experimental / — stay in one column. Only shown
				// when it is a deliberate choice: an Auto marker on every row
				// would be noise. A VAD row never shows one — nothing reads a
				// VAD placement. Slot padding is zero when the tag is omitted
				// so an empty slot cannot shift the registration column.
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					.Padding(bShowPlacement ? FMargin(8.f, 0.f, 4.f, 0.f) : FMargin(0.f))
				[ bShowPlacement
					? MakePlacementPill(PlacementOf(M.Name))
					: SNullWidget::NullWidget ]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0, 0, 0)
				[ MakeRegPill(RegOf(M.Name)) ]
			]
		];
}

// ── Layout: properties ──────────────────────────────────────────────────────

TSharedRef<SWidget> STryllModelManagerPanel::BuildPropertiesPanel()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(0)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(12, 8)
			[ SNew(STextBlock).Text(LOCTEXT("PropsHdr", "MODEL PROPERTIES")).Font(FCoreStyle::GetDefaultFontStyle("Bold", 9)).ColorAndOpacity(FSlateColor(Dim)) ]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()[ SAssignNew(PropertiesContainer, SBox) ]
			]
		];
}

TSharedRef<SWidget> STryllModelManagerPanel::MakePropertiesContent()
{
	if (!Models.IsValidIndex(SelectedIndex))
	{
		return SNew(SBox).Padding(16)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FTryllEditorUiContract::ModelManagerSelect))
			.ColorAndOpacity(FSlateColor(Dim)).AutoWrapText(true)
		];
	}

	const FTryllModelInfo& M = Models[SelectedIndex];
	const bool bDownloading = M.Status == ETryllModelStatus::Downloading;
	const bool bOnDisk      = M.Status == ETryllModelStatus::Downloaded || M.Status == ETryllModelStatus::Local || M.Status == ETryllModelStatus::Loaded;
	const bool bLoaded      = M.Status == ETryllModelStatus::Loaded;
	const bool bLocal       = M.Status == ETryllModelStatus::Local || M.HuggingFaceRepo.IsEmpty();
	const FString Name      = M.Name;

	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

	auto KV = [](const FText& K, const FString& V) -> TSharedRef<SWidget>
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[ SNew(SBox).WidthOverride(92.f)[ SNew(STextBlock).Text(K).ColorAndOpacity(FSlateColor(Dim)) ] ]
			+ SHorizontalBox::Slot().FillWidth(1.f)[ SNew(STextBlock).Text(FText::FromString(V)).AutoWrapText(true) ];
	};

	auto AddSection = [&Box](const FText& Title, const TSharedRef<SWidget>& Content)
	{
		Box->AddSlot().AutoHeight().Padding(16, 12, 16, 6)
		[ SNew(STextBlock).Text(Title).Font(FCoreStyle::GetDefaultFontStyle("Bold", 8)).ColorAndOpacity(FSlateColor(Dim)) ];
		Box->AddSlot().AutoHeight().Padding(16, 0, 16, 10)[ Content ];
		Box->AddSlot().AutoHeight()[ SNew(SSeparator).Thickness(0.5f) ];
	};

	// Header: name + type/engine/audience badges
	Box->AddSlot().AutoHeight().Padding(16, 12, 16, 6)
	[ SNew(STextBlock).Text(FText::FromString(M.Name)).Font(FCoreStyle::GetDefaultFontStyle("Bold", 13)).AutoWrapText(true) ];
	Box->AddSlot().AutoHeight().Padding(16, 0, 16, 8)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 5, 0)[ MakeBadge(TypeDisplay(M.ModelType)) ]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 5, 0)[ MakeBadge(M.Engine) ]
		+ SHorizontalBox::Slot().AutoWidth()[ MakeBadge(FString::Printf(TEXT("audience: %s"), *AudienceLabel(M.Audience))) ]
	];
	Box->AddSlot().AutoHeight()[ SNew(SSeparator).Thickness(0.5f) ];

	// Languages — shown for every model type. A language model declares nothing
	// today, and saying so explicitly is more useful than an absent section that
	// reads as "no languages".
	{
		TSharedRef<SVerticalBox> S = SNew(SVerticalBox);
		if (M.Languages.Num() == 0)
		{
			S->AddSlot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("LangNone",
					"Not declared in the model catalog. Tryll will not pass this model a "
					"language hint, and it is never treated as blocking a language."))
				.ColorAndOpacity(FSlateColor(Dim))
				.AutoWrapText(true)
			];
		}
		else
		{
			TSharedRef<SWrapBox> Chips = SNew(SWrapBox).UseAllottedSize(true);
			for (const FString& Code : M.Languages)
			{
				Chips->AddSlot().Padding(0, 0, 5, 5)
				[ MakeBadge(FString::Printf(TEXT("%s (%s)"),
					*UTryllLanguages::LanguageDisplayName(Code), *Code)) ];
			}
			S->AddSlot().AutoHeight()[ Chips ];

			// The actionable part: does this model cover what the project selected?
			const FString Selected = GetDefault<UTryllRuntimeSettings>()->Locale;
			if (!Selected.IsEmpty())
			{
				const FString Lang    = UTryllLanguages::LanguageOf(Selected);
				const bool bCovered   = UTryllLanguages::ModelSupportsLanguage(M, Selected);
				const FString Display = UTryllLanguages::LanguageDisplayName(Lang);
				const FText CoverageText = bCovered
					? FText::Format(LOCTEXT("LangCovered", "Covers the project language: {0} ({1})."),
						FText::FromString(Display), FText::FromString(Lang))
					: (M.ModelType == TEXT("stt")
						? FText::Format(LOCTEXT("SttLangNotCovered",
							"Does NOT cover the project language {0} ({1}). Voice input "
							"creation with this model is rejected."),
							FText::FromString(Display), FText::FromString(Lang))
						: FText::Format(LOCTEXT("LangNotCovered",
							"Does NOT cover the project language {0} ({1}). Agents relying "
							"on the session locale with this model are rejected at creation."),
							FText::FromString(Display), FText::FromString(Lang)));
				S->AddSlot().AutoHeight().Padding(0, 6, 0, 0)
				[
					SNew(STextBlock)
					.Text(CoverageText)
					.ColorAndOpacity(FSlateColor(bCovered ? ColProd : ColMissing))
					.AutoWrapText(true)
				];
			}
		}
		AddSection(LOCTEXT("LangHdr", "LANGUAGES"), S);
	}

	// Status
	{
		TSharedRef<SVerticalBox> S = SNew(SVerticalBox);
		S->AddSlot().AutoHeight().Padding(0, 1)[ KV(LOCTEXT("StateK", "State"), StatusLabel(M.Status).ToString()) ];
		S->AddSlot().AutoHeight().Padding(0, 1)[ KV(LOCTEXT("DiskK", "On disk"), bOnDisk ? FTryllModelManagerPresentation::FmtDecimalBytes(M.SizeBytes) : FString(TEXT("—"))) ];
		S->AddSlot().AutoHeight().Padding(0, 1)[ KV(LOCTEXT("LocK", "Location"),
			M.LocalPath.IsEmpty() ? FString(TEXT("—")) : M.LocalPath) ];
		if (bDownloading)
		{
			S->AddSlot().AutoHeight().Padding(0, 6, 0, 0)
			[ SNew(SProgressBar).Percent_Lambda([this, Name]() -> TOptional<float> { return (Progress.Contains(Name) ? Progress[Name] : 0.f) / 100.f; }) ];
		}
		AddSection(LOCTEXT("StatusHdr", "STATUS"), S);
	}

	// Build registration
	{
		const ETryllModelRegistration CurReg = RegOf(Name);
		auto RegButton = [this, Name, CurReg](ETryllModelRegistration R, const FText& Label) -> TSharedRef<SWidget>
		{
			const bool bOn = (CurReg == R);
			return SNew(SButton).HAlign(HAlign_Center)
				.OnClicked_Lambda([this, Name, R] { SetRegistration(Name, R); return FReply::Handled(); })
				[ SNew(STextBlock).Text(Label).ColorAndOpacity(FSlateColor(bOn ? RegColor(R) : Dim)).Font(FCoreStyle::GetDefaultFontStyle(bOn ? "Bold" : "Regular", 9)) ];
		};
		const FText Hint =
			CurReg == ETryllModelRegistration::Production     ? LOCTEXT("HintProd", "Shipped: files are copied into the packaged build.")
			: CurReg == ETryllModelRegistration::Experimental ? LOCTEXT("HintExp", "Dev-only: usable in the editor, stripped from the packaged build.")
			:                                                   LOCTEXT("HintNone", "Not part of this project; ignored by the build.");
		TSharedRef<SVerticalBox> S = SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 4, 0)[ RegButton(ETryllModelRegistration::None,         LOCTEXT("RegNoneBtn", "Not registered")) ]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 4, 0)[ RegButton(ETryllModelRegistration::Experimental, LOCTEXT("RegExpBtn", "Experimental")) ]
				+ SHorizontalBox::Slot().FillWidth(1.f)[ RegButton(ETryllModelRegistration::Production, LOCTEXT("RegProdBtn", "Production")) ]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
			[ SNew(STextBlock).Text(Hint).ColorAndOpacity(FSlateColor(Dim)).Font(FCoreStyle::GetDefaultFontStyle("Regular", 8)).AutoWrapText(true) ];
		AddSection(LOCTEXT("BuildHdr", "BUILD REGISTRATION  (THIS PROJECT)"), S);
	}

	// Memory placement. Only meaningful for a registered model this control can
	// actually affect — an unregistered model is never loaded, and a VAD model is not
	// placed at all (see SupportsPlacement).
	if (RegOf(Name) != ETryllModelRegistration::None && SupportsPlacement(Name))
	{
		const ETryllMemoryPlacement CurPlace = PlacementOf(Name);
		auto PlaceButton = [this, Name, CurPlace](
			ETryllMemoryPlacement P, const FText& Label) -> TSharedRef<SWidget>
		{
			const bool bOn = (CurPlace == P);
			// Auto is the default, so a selected Auto reads as neutral rather than
			// as a choice the user made; RAM/VRAM are deliberate.
			const FLinearColor OnColor =
				(P == ETryllMemoryPlacement::Auto) ? Dim : RegColor(ETryllModelRegistration::Production);
			return SNew(SButton).HAlign(HAlign_Center)
				.OnClicked_Lambda([this, Name, P] { SetPlacement(Name, P); return FReply::Handled(); })
				[
					SNew(STextBlock).Text(Label)
						.ColorAndOpacity(FSlateColor(bOn ? OnColor : Dim))
						.Font(FCoreStyle::GetDefaultFontStyle(bOn ? "Bold" : "Regular", 9))
				];
		};

		// Placement is authored here and shipped to players' machines, so the text
		// promises nothing about hardware we cannot see.
		const FTryllModelInfo* Row = Models.FindByPredicate(
			[&](const FTryllModelInfo& X){ return X.Name == Name; });
		const bool bIsTts = (Row && Row->ModelType == TEXT("tts"));
		const FText PlaceHint = bIsTts
			? LOCTEXT("PlaceHintTts",
				"Auto keeps TTS on the CPU — measured faster for small voice graphs, "
				"where host/device copies cost more than they save. Override only if "
				"you have measured otherwise on your target hardware.")
			: LOCTEXT("PlaceHintDefault",
				"Auto uses device memory when the player's machine has a usable GPU, "
				"system memory otherwise. RAM and VRAM are hints, not guarantees: a "
				"machine that cannot honour one loads the model the other way and "
				"reports what it did. Whether VRAM means CUDA or Vulkan is the host's "
				"decision, not yours.");

		TSharedRef<SVerticalBox> P = SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 4, 0)
					[ PlaceButton(ETryllMemoryPlacement::Auto, LOCTEXT("PlaceAutoBtn", "Auto")) ]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 4, 0)
					[ PlaceButton(ETryllMemoryPlacement::Ram,  LOCTEXT("PlaceRamBtn", "RAM")) ]
				+ SHorizontalBox::Slot().FillWidth(1.f)
					[ PlaceButton(ETryllMemoryPlacement::Vram, LOCTEXT("PlaceVramBtn", "VRAM")) ]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
			[
				SNew(STextBlock).Text(PlaceHint).ColorAndOpacity(FSlateColor(Dim))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8)).AutoWrapText(true)
			];
		AddSection(LOCTEXT("PlaceHdr", "MEMORY PLACEMENT  (THIS PROJECT)"), P);
	}

	// Hotword biasing. STT only — nothing else has a decoder to bias — and only
	// once registered, for the same reason placement is.
	if (RegOf(Name) != ETryllModelRegistration::None && SupportsHotwordBias(Name))
	{
		const ETryllHotwordBias CurBias = HotwordBiasOf(Name);
		auto BiasButton = [this, Name, CurBias](
			ETryllHotwordBias B, const FText& Label) -> TSharedRef<SWidget>
		{
			const bool bOn = (CurBias == B);
			// Default reads as neutral — it is the absence of an opinion rather
			// than a decision — while Off and On are deliberate.
			const FLinearColor OnColor = (B == ETryllHotwordBias::Default)
				? Dim : RegColor(ETryllModelRegistration::Production);
			return SNew(SButton).HAlign(HAlign_Center)
				.OnClicked_Lambda([this, Name, B] { SetHotwordBias(Name, B); return FReply::Handled(); })
				[
					SNew(STextBlock).Text(Label)
						.ColorAndOpacity(FSlateColor(bOn ? OnColor : Dim))
						.Font(FCoreStyle::GetDefaultFontStyle(bOn ? "Bold" : "Regular", 9))
				];
		};

		const FText BiasHint = LOCTEXT("HotwordHint",
			"Hotwords bias the recognizer toward names and terms you supply per "
			"voice input. Default follows the server's own setting for this model. "
			"On and Off decide instead of hinting: biasing needs beam-search "
			"decoding, which is chosen when the model loads, so On costs a few "
			"percent of decode time and changes this model's transcripts even for "
			"voice inputs that supply no hotwords. Off makes supplying them an "
			"error rather than a silent no-op.");

		TSharedRef<SVerticalBox> H = SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 4, 0)
					[ BiasButton(ETryllHotwordBias::Default, LOCTEXT("BiasDefaultBtn", "Default")) ]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 4, 0)
					[ BiasButton(ETryllHotwordBias::Off, LOCTEXT("BiasOffBtn", "Off")) ]
				+ SHorizontalBox::Slot().FillWidth(1.f)
					[ BiasButton(ETryllHotwordBias::On, LOCTEXT("BiasOnBtn", "On")) ]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
			[
				SNew(STextBlock).Text(BiasHint).ColorAndOpacity(FSlateColor(Dim))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8)).AutoWrapText(true)
			];
		AddSection(LOCTEXT("HotwordHdr", "HOTWORD BIASING  (THIS PROJECT)"), H);
	}

	// Controls
	{
		TSharedRef<SHorizontalBox> Controls = SNew(SHorizontalBox);
		if (bDownloading)
			Controls->AddSlot().FillWidth(1.f)[ SNew(SButton).IsEnabled(false).HAlign(HAlign_Center).Text(LOCTEXT("Downloading", "Downloading…")) ];
		else if (bOnDisk)
		{
			Controls->AddSlot().FillWidth(1.f).Padding(0, 0, 4, 0)[ SNew(SButton).HAlign(HAlign_Center).Text(LOCTEXT("DeleteFiles", "Delete files"))
				.OnClicked_Lambda([this, Name] { OnDeleteModel(Name); return FReply::Handled(); }) ];
			Controls->AddSlot().FillWidth(1.f)[ SNew(SButton).HAlign(HAlign_Center).Text(bLoaded ? LOCTEXT("Unload", "Unload") : LOCTEXT("Load", "Load"))
				.OnClicked_Lambda([this, Name, bLoaded] { OnLoadToggle(Name, !bLoaded); return FReply::Handled(); }) ];
		}
		else
			Controls->AddSlot().FillWidth(1.f)[ SNew(SButton).HAlign(HAlign_Center).Text(LOCTEXT("Download", "Download"))
				.OnClicked_Lambda([this, Name] { OnDownload(Name); return FReply::Handled(); }) ];
		AddSection(LOCTEXT("ControlsHdr", "CONTROLS"), Controls);
	}

	// Variant
	{
		TSharedRef<SVerticalBox> S = SNew(SVerticalBox);
		S->AddSlot().AutoHeight().Padding(0, 1)[ KV(LOCTEXT("SrcK", "Source"), bLocal ? FString(TEXT("local_path")) : FString(TEXT("huggingface_repo"))) ];
		if (bLocal)
		{
			S->AddSlot().AutoHeight().Padding(0, 1)[ KV(LOCTEXT("RepoK", "Repo"), FString(TEXT("(local path)"))) ];
		}
		else
		{
			const FString Url     = FString::Printf(TEXT("https://huggingface.co/%s"), *M.HuggingFaceRepo);
			const FString RepoStr = M.HuggingFaceRepo;
			S->AddSlot().AutoHeight().Padding(0, 1)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()[ SNew(SBox).WidthOverride(92.f)[ SNew(STextBlock).Text(LOCTEXT("RepoK", "Repo")).ColorAndOpacity(FSlateColor(Dim)) ] ]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(SHyperlink)
					.Style(&FCoreStyle::Get().GetWidgetStyle<FHyperlinkStyle>("Hyperlink"))
					.Text(FText::FromString(RepoStr))
					.ToolTipText(FText::FromString(Url))
					.OnNavigate_Lambda([Url] { FPlatformProcess::LaunchURL(*Url, nullptr, nullptr); })
				]
			];
		}
		if (M.ContextSize > 0) S->AddSlot().AutoHeight().Padding(0, 1)[ KV(LOCTEXT("CtxK", "context_size"), FString::Printf(TEXT("%d"), M.ContextSize)) ];
		if (!M.ToolCallFormat.IsEmpty()) S->AddSlot().AutoHeight().Padding(0, 1)[ KV(LOCTEXT("ToolK", "tool_call"), M.ToolCallFormat) ];
		// Advisory catalog verdict — detail pane only, never in the main list.
		if (!M.ToolCallSupport.IsEmpty()) S->AddSlot().AutoHeight().Padding(0, 1)[ KV(LOCTEXT("ToolSupK", "tool_calls"), M.ToolCallSupport) ];
		if (M.Files.Num() > 0)
		{
			S->AddSlot().AutoHeight().Padding(0, 6, 0, 0)[ SNew(STextBlock).Text(LOCTEXT("FilesK", "files")).ColorAndOpacity(FSlateColor(Dim)).Font(FCoreStyle::GetDefaultFontStyle("Regular", 8)) ];
			for (const FString& F : M.Files)
				S->AddSlot().AutoHeight()[ SNew(STextBlock).Text(FText::FromString(F)).ColorAndOpacity(FSlateColor(Dim)).Font(FCoreStyle::GetDefaultFontStyle("Regular", 8)).AutoWrapText(true) ];
		}
		AddSection(FText::Format(LOCTEXT("VariantHdr", "VARIANT - {0}"), FText::FromString(M.Engine)), S);
	}

	// Licence — read from the local catalog; ListModels does not carry it. Models
	// without one (user catalogs, local paths) say so rather than hiding the section.
	{
		const FString LicenceUrl = FTryllModelCatalogFile::FindLicenceUrl(Name, M.Engine);
		TSharedRef<SVerticalBox> S = SNew(SVerticalBox);
		if (!LicenceUrl.IsEmpty())
		{
			S->AddSlot().AutoHeight().Padding(0, 0, 0, 4)
			[
				SNew(STextBlock).Text(FText::FromString(FTryllEditorUiContract::ModelLicenceNotice)).AutoWrapText(true)
			];
			S->AddSlot().AutoHeight()
			[
				SNew(SHyperlink)
				.Style(&FCoreStyle::Get().GetWidgetStyle<FHyperlinkStyle>("Hyperlink"))
				.Text(FText::FromString(LicenceUrl))
				.ToolTipText(FText::FromString(LicenceUrl))
				.OnNavigate_Lambda([LicenceUrl] { FPlatformProcess::LaunchURL(*LicenceUrl, nullptr, nullptr); })
			];
		}
		else
		{
			S->AddSlot().AutoHeight()
			[
				SNew(STextBlock).Text(FText::FromString(FTryllEditorUiContract::ModelLicenceMissing))
					.ColorAndOpacity(FSlateColor(Dim)).AutoWrapText(true)
			];
		}
		AddSection(LOCTEXT("LicenceHdr", "LICENCE"), S);
	}

	// Default sampling
	if (M.bHasDefaultSampling)
	{
		TSharedRef<SVerticalBox> S = SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[ SNew(STextBlock).Text(FText::FromString(FmtSampling(M.DefaultSampling))).AutoWrapText(true) ];
		AddSection(LOCTEXT("SampHdr", "DEFAULT SAMPLING"), S);
	}

	return Box;
}

// ── Rebuild ─────────────────────────────────────────────────────────────────

TArray<int32> STryllModelManagerPanel::FilteredIndices() const
{
	TArray<int32> Out;
	for (int32 I = 0; I < Models.Num(); ++I)
	{
		const FTryllModelInfo& M = Models[I];
		if (!FTryllModelManagerPresentation::MatchesFilters(
			FTryllModelManagerPresentation::Included(TypeFilter, M.ModelType, true),
			FTryllModelManagerPresentation::Included(EngineFilter, M.Engine, true),
			FTryllModelManagerPresentation::Included(StatusFilter, M.Status, true),
			FTryllModelManagerPresentation::Included(AudienceFilter, M.Audience, true),
			FTryllModelManagerPresentation::Included(RegFilter, RegOf(M.Name), true),
			M.Name,
			M.HuggingFaceRepo,
			SearchText))
		{
			continue;
		}
		Out.Add(I);
	}

	Out.Sort([this](int32 A, int32 B)
	{
		const FTryllModelInfo& Ma = Models[A];
		const FTryllModelInfo& Mb = Models[B];
		switch (SortKey)
		{
		case ESortKey::Type:   if (Ma.ModelType != Mb.ModelType) return Ma.ModelType < Mb.ModelType; break;
		case ESortKey::Size:   if (Ma.SizeBytes != Mb.SizeBytes) return Ma.SizeBytes > Mb.SizeBytes; break;
		case ESortKey::Status: if (Ma.Status   != Mb.Status)    return (uint8)Ma.Status < (uint8)Mb.Status; break;
		case ESortKey::Registration:
		{
			const ETryllModelRegistration Ra = RegOf(Ma.Name);
			const ETryllModelRegistration Rb = RegOf(Mb.Name);
			if (Ra != Rb) return (uint8)Ra > (uint8)Rb;
			break;
		}
		default: break;
		}
		return Ma.Name < Mb.Name;
	});
	return Out;
}

void STryllModelManagerPanel::RebuildList()
{
	if (!ListScroll.IsValid()) return;
	ListScroll->ClearChildren();
	const TArray<int32> Idx = FilteredIndices();
	for (int32 I : Idx) ListScroll->AddSlot()[ MakeListRow(I) ];
	if (CountText.IsValid())
		CountText->SetText(FText::Format(LOCTEXT("NModels", "{0} of {1} models"), FText::AsNumber(Idx.Num()), FText::AsNumber(Models.Num())));
}

void STryllModelManagerPanel::RebuildProperties()
{
	if (PropertiesContainer.IsValid()) PropertiesContainer->SetContent(MakePropertiesContent());
}

void STryllModelManagerPanel::RebuildCounts()
{
	int32 Dl = 0; int64 Bytes = 0; int32 Prod = 0; int32 Exp = 0; int64 ProdBytes = 0;
	for (const FTryllModelInfo& M : Models)
	{
		if (M.Status == ETryllModelStatus::Downloaded || M.Status == ETryllModelStatus::Local || M.Status == ETryllModelStatus::Loaded)
		{
			Dl++;
			Bytes += M.SizeBytes;
		}
		switch (RegOf(M.Name))
		{
		case ETryllModelRegistration::Production:   Prod++; ProdBytes += M.SizeBytes; break;
		case ETryllModelRegistration::Experimental: Exp++;  break;
		default: break;
		}
	}
	if (TotalDownloaded.IsValid()) TotalDownloaded->SetText(FText::AsNumber(Dl));
	if (TotalDisk.IsValid())       TotalDisk->SetText(FText::FromString(FTryllModelManagerPresentation::FmtDecimalBytes(Bytes)));
	if (TotalProd.IsValid())       TotalProd->SetText(FText::AsNumber(Prod));
	if (TotalExp.IsValid())        TotalExp->SetText(FText::AsNumber(Exp));
	if (TotalShip.IsValid())       TotalShip->SetText(FText::FromString(FString::Printf(TEXT("%d · %s"), Prod, *FTryllModelManagerPresentation::FmtDecimalBytes(ProdBytes))));
}

void STryllModelManagerPanel::RebuildAll()
{
	RebuildList();
	RebuildCounts();
	RebuildProperties();
}

void STryllModelManagerPanel::SelectRow(int32 Index)
{
	SelectedIndex = Index;
	RebuildList();
	RebuildProperties();
}

void STryllModelManagerPanel::ResetFilters()
{
	StatusFilter.Empty();
	for (ETryllModelStatus S : { ETryllModelStatus::Loaded, ETryllModelStatus::Downloaded, ETryllModelStatus::Local, ETryllModelStatus::Incomplete, ETryllModelStatus::Absent }) StatusFilter.Add(S);
	TypeFilter.Empty();
	for (const TCHAR* T : { TEXT("language"), TEXT("embedding"), TEXT("stt"), TEXT("tts"), TEXT("vad") }) TypeFilter.Add(T);
	EngineFilter.Empty();
	for (const TCHAR* E : { TEXT("llama-cpp"), TEXT("sherpa-onnx"), TEXT("onnx-runtime") }) EngineFilter.Add(E);
	AudienceFilter.Empty();
	for (const TCHAR* A : { TEXT("public"), TEXT("experimental"), TEXT("internal"), TEXT("") }) AudienceFilter.Add(A);
	RegFilter.Empty();
	for (ETryllModelRegistration R : { ETryllModelRegistration::Production, ETryllModelRegistration::Experimental, ETryllModelRegistration::None }) RegFilter.Add(R);
	SearchText.Empty();
	if (SearchBox.IsValid()) SearchBox->SetText(FText::GetEmpty());
	SortKey = ESortKey::Name;
	RebuildList();
}

// ── Actions ─────────────────────────────────────────────────────────────────

void STryllModelManagerPanel::OnDownload(const FString& ModelName)
{
	if (!EditorSubsystem) return;
	Progress.Add(ModelName, 0.f);
	for (FTryllModelInfo& M : Models)
		if (M.Name == ModelName)
			M.Status = FTryllModelManagerPresentation::StatusAfterDownloadProgress();
	EditorSubsystem->RequestDownloadModel(ModelName);

	if (IsSttModel(ModelName))
	{
		for (const FString& VadName : VadCompanions())
		{
			const FTryllModelInfo* Vad = Models.FindByPredicate(
				[&](const FTryllModelInfo& X) { return X.Name == VadName; });
			if (!Vad) continue;
			if (Vad->Status != ETryllModelStatus::Absent) continue;
			if (Vad->HuggingFaceRepo.IsEmpty()) continue;

			const FString SizeHint = Vad->SizeBytes > 0
				? FTryllModelManagerPresentation::FmtDecimalBytes(Vad->SizeBytes)
				: TEXT("size unknown until downloaded");

			const EAppReturnType::Type Answer = FTryllEditorPrompts::Open(EAppMsgType::YesNo, FText::Format(
				LOCTEXT("VadDownloadCompanion",
					"Also download \"{0}\" ({1}) for automatic end-of-speech detection?\n\n"
					"Choose No if you control utterance boundaries yourself (push-to-talk / hands-free).\n\n"
					"Learn more: {2}"),
				FText::FromString(VadName),
				FText::FromString(SizeHint),
				FText::FromString(TryllDocs::VoiceInput)));
			if (Answer != EAppReturnType::Yes) continue;

			Progress.Add(VadName, 0.f);
			for (FTryllModelInfo& M : Models)
				if (M.Name == VadName)
					M.Status = FTryllModelManagerPresentation::StatusAfterDownloadProgress();
			EditorSubsystem->RequestDownloadModel(VadName);
		}
	}

	RebuildAll();
}

void STryllModelManagerPanel::OnDownloadAll()
{
	if (!EditorSubsystem) return;
	for (int32 I : FilteredIndices())
	{
		if (Models[I].Status != ETryllModelStatus::Absent) continue;
		Progress.Add(Models[I].Name, 0.f);
		Models[I].Status = FTryllModelManagerPresentation::StatusAfterDownloadProgress();
		EditorSubsystem->RequestDownloadModel(Models[I].Name);
	}
	RebuildAll();
}

void STryllModelManagerPanel::OnLoadToggle(const FString& ModelName, bool bLoad)
{
	if (!EditorSubsystem) return;
	// Send the selector's current value, not nothing. The session's hints are a
	// snapshot taken at CreateSession, so a placement chosen afterwards would
	// otherwise be ignored until the session was recreated — which is exactly how
	// this control appeared to do nothing.
	if (bLoad)
		EditorSubsystem->RequestLoadModel(
			ModelName, PlacementOf(ModelName), /*bIsOverride=*/true);
	else       EditorSubsystem->RequestUnloadModel(ModelName);
}

void STryllModelManagerPanel::OnDeleteModel(const FString& ModelName)
{
	if (!EditorSubsystem) return;
	const EAppReturnType::Type Answer = FTryllEditorPrompts::Open(
		EAppMsgType::YesNo,
		FText::Format(LOCTEXT("DeleteConfirm", "Delete the downloaded files for \"{0}\"?\n\nThis removes them from disk; the model can be re-downloaded later."),
			FText::FromString(ModelName)));
	if (Answer != EAppReturnType::Yes) return;
	EditorSubsystem->RequestDeleteModel(ModelName);

	// Deleting files can turn a still-registered model back to Absent; don't let the PIE
	// guard's cached "verified ready" flag mask that on the next Play press.
	FTryllPIEModelGuard::Invalidate();
}

void STryllModelManagerPanel::PromptDownloadMissingRegisteredModels()
{
	if (!EditorSubsystem) return;

	TArray<FString> Downloadable, NonDownloadable;
	for (const FTryllModelInfo& M : Models)
	{
		if (RegOf(M.Name) == ETryllModelRegistration::None) continue;
		if (M.Status != ETryllModelStatus::Absent) continue;
		if (M.HuggingFaceRepo.IsEmpty()) NonDownloadable.Add(M.Name);
		else                             Downloadable.Add(M.Name);
	}

	if (NonDownloadable.Num() > 0)
	{
		FString List;
		for (const FString& N : NonDownloadable) List += FString::Printf(TEXT("  • %s\n"), *N);
		FTryllEditorPrompts::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("NonDownloadableMissing",
				"The following registered models are missing and cannot be downloaded automatically "
				"(no HuggingFace source configured):\n\n{0}\nProvide them via a local path or update the catalog."),
			FText::FromString(List)));
	}

	if (Downloadable.Num() == 0) return;

	FString List;
	for (const FString& N : Downloadable) List += FString::Printf(TEXT("  • %s\n"), *N);
	const EAppReturnType::Type Answer = FTryllEditorPrompts::Open(EAppMsgType::YesNo, FText::Format(
		LOCTEXT("DownloadMissingConfirm",
			"{0} registered (Production/Experimental) model(s) are not downloaded yet:\n\n{1}\nDownload them now?"),
		FText::AsNumber(Downloadable.Num()), FText::FromString(List)));
	if (Answer != EAppReturnType::Yes) return;

	for (const FString& N : Downloadable)
	{
		Progress.Add(N, 0.f);
		for (FTryllModelInfo& M : Models)
			if (M.Name == N)
				M.Status = FTryllModelManagerPresentation::StatusAfterDownloadProgress();
		EditorSubsystem->RequestDownloadModel(N);
	}
	RebuildAll();
}

// ── Build registration (manifest) ─────────────────────────────────────────────

bool STryllModelManagerPanel::SupportsPlacement(const FString& Name) const
{
	// False for VAD, and not because the editor is being cautious. A VAD entry is
	// download/config metadata, not a model the server caches: a detector is created
	// per STT session inside the recognizer, so there is no load for a placement to
	// apply to, and the server pins it to the CPU deliberately (a ~1.8 MB graph fed
	// 32 ms windows is the wrong shape for a device). Offering the control anyway
	// would be worse than not having it — an author would set VRAM, ship it, and
	// never learn that nothing read it.
	const FTryllModelInfo* M = Models.FindByPredicate(
		[&](const FTryllModelInfo& X) { return X.Name == Name; });
	return !M || FTryllModelManagerPresentation::SupportsPlacement(M->ModelType);
}

bool STryllModelManagerPanel::IsSttModel(const FString& Name) const
{
	const FTryllModelInfo* M = Models.FindByPredicate(
		[&](const FTryllModelInfo& X) { return X.Name == Name; });
	return M && M->ModelType == TEXT("stt");
}

TArray<FString> STryllModelManagerPanel::VadCompanions() const
{
	TArray<FString> Out;
	for (const FTryllModelInfo& M : Models)
		if (M.ModelType == TEXT("vad")) Out.Add(M.Name);
	return Out;
}

ETryllModelRegistration STryllModelManagerPanel::RegOf(const FString& ModelName) const
{
	return UTryllModelManifest::GetActive()->Get(ModelName);
}

ETryllMemoryPlacement STryllModelManagerPanel::PlacementOf(const FString& ModelName) const
{
	const UTryllModelManifest* Manifest = UTryllModelManifest::GetActive();
	for (const FTryllModelManifestEntry& E : Manifest->Entries)
	{
		if (E.Name == ModelName)
		{
			return E.Placement;
		}
	}
	return ETryllMemoryPlacement::Auto;
}

namespace
{
/// Map the catalog's engine string (FTryllModelInfo::Engine, e.g. "llama-cpp") onto
/// the client enum, so the manifest can declare it offline.
///
/// Falls back to LlamaCpp for an unrecognised value rather than Mock: Mock is the
/// enum's zero value and a production server refuses it, so defaulting there would
/// turn "unknown engine name" into a refused session with a misleading reason.
ETryllInferenceEngine ParseEngine(const FString& Engine)
{
	return FTryllModelManagerPresentation::ParseEngine(Engine);
}
} // namespace

bool STryllModelManagerPanel::SupportsHotwordBias(const FString& ModelName) const
{
	// STT only: biasing happens inside the speech recognizer's beam search, so a
	// language, embedding, TTS or VAD entry has nothing to apply it to.
	//
	// Not narrowed further. Whether a *particular* STT model can bias also depends
	// on its decoder family and whether a sub-word vocabulary can be derived from
	// its bundle, and neither is knowable without a server — it answers that at
	// load and refuses a lexicon with a reason when it cannot. Hiding the control
	// on a guess would leave an author concluding the feature does not exist.
	const FTryllModelInfo* Row = Models.FindByPredicate(
		[&](const FTryllModelInfo& X){ return X.Name == ModelName; });
	return Row && Row->ModelType.Equals(TEXT("stt"), ESearchCase::IgnoreCase);
}

ETryllHotwordBias STryllModelManagerPanel::HotwordBiasOf(const FString& ModelName) const
{
	const UTryllModelManifest* Manifest = UTryllModelManifest::GetActive();
	if (!Manifest)
	{
		return ETryllHotwordBias::Default;
	}
	for (const FTryllModelManifestEntry& E : Manifest->Entries)
	{
		if (E.Name == ModelName)
		{
			return E.Hotwords;
		}
	}
	return ETryllHotwordBias::Default;
}

void STryllModelManagerPanel::SetHotwordBias(
	const FString& ModelName, ETryllHotwordBias Bias)
{
	UTryllModelManifest* Manifest = UTryllModelManifest::GetMutableActive();
	const ETryllModelRegistration Reg = Manifest->Get(ModelName);
	if (Reg == ETryllModelRegistration::None)
	{
		return;
	}

	FString               ModelType;
	TArray<FString>       Languages;
	ETryllInferenceEngine Engine          = ETryllInferenceEngine::LlamaCpp;
	bool                  bHaveCatalogRow = false;
	if (const FTryllModelInfo* M = Models.FindByPredicate(
			[&](const FTryllModelInfo& X){ return X.Name == ModelName; }))
	{
		ModelType       = M->ModelType;
		Languages       = M->Languages;
		Engine          = ParseEngine(M->Engine);
		bHaveCatalogRow = true;
	}

	Manifest->Set(ModelName, ModelType, Reg,
	              bHaveCatalogRow ? &Languages : nullptr,
	              bHaveCatalogRow ? &Engine : nullptr,
	              nullptr, &Bias);
	Manifest->SaveToConfig();
	RebuildAll();
}

void STryllModelManagerPanel::SetPlacement(
	const FString& ModelName, ETryllMemoryPlacement Placement)
{
	UTryllModelManifest* Manifest = UTryllModelManifest::GetMutableActive();
	const ETryllModelRegistration Reg = Manifest->Get(ModelName);
	if (Reg == ETryllModelRegistration::None)
	{
		return;
	}

	FString               ModelType;
	TArray<FString>       Languages;
	ETryllInferenceEngine Engine          = ETryllInferenceEngine::LlamaCpp;
	bool                  bHaveCatalogRow = false;
	if (const FTryllModelInfo* M = Models.FindByPredicate(
			[&](const FTryllModelInfo& X){ return X.Name == ModelName; }))
	{
		ModelType       = M->ModelType;
		Languages       = M->Languages;
		Engine          = ParseEngine(M->Engine);
		bHaveCatalogRow = true;
	}

	Manifest->Set(ModelName, ModelType, Reg,
	              bHaveCatalogRow ? &Languages : nullptr,
	              bHaveCatalogRow ? &Engine : nullptr,
	              &Placement);
	Manifest->SaveToConfig();
	RebuildAll();
}

void STryllModelManagerPanel::SetRegistration(const FString& ModelName, ETryllModelRegistration Reg)
{
	FString               ModelType;
	TArray<FString>       Languages;
	ETryllInferenceEngine Engine          = ETryllInferenceEngine::LlamaCpp;
	bool                  bHaveCatalogRow = false;
	if (const FTryllModelInfo* M = Models.FindByPredicate([&](const FTryllModelInfo& X){ return X.Name == ModelName; }))
	{
		ModelType       = M->ModelType;
		Languages       = M->Languages;
		Engine          = ParseEngine(M->Engine);
		bHaveCatalogRow = true;
	}
	UTryllModelManifest* Manifest = UTryllModelManifest::GetMutableActive();
	// Cache the catalog languages alongside the registration: the settings
	// dropdown and the model-picker warnings read the manifest offline, long after
	// this live FTryllModelInfo is gone. Pass null when we have no catalog row so
	// an offline re-registration keeps whatever was cached before.
	Manifest->Set(ModelName, ModelType, Reg, bHaveCatalogRow ? &Languages : nullptr,
	              bHaveCatalogRow ? &Engine : nullptr);

	if (ModelType == TEXT("stt")
		&& (Reg == ETryllModelRegistration::Experimental || Reg == ETryllModelRegistration::Production))
	{
		const FText TierLabel = (Reg == ETryllModelRegistration::Production)
			? LOCTEXT("ProdTier", "Production")
			: LOCTEXT("ExpTier", "Experimental");

		for (const FString& VadName : VadCompanions())
		{
			if (RegOf(VadName) != ETryllModelRegistration::None) continue;

			const EAppReturnType::Type Answer = FTryllEditorPrompts::Open(EAppMsgType::YesNo, FText::Format(
				LOCTEXT("VadRegisterCompanion",
					"\"{0}\" uses \"{1}\" for automatic end-of-speech detection.\n\n"
					"Also register \"{1}\" as {2}?\n\n"
					"Choose No if you control utterance boundaries yourself (push-to-talk / hands-free).\n\n"
					"Learn more: {3}"),
				FText::FromString(ModelName),
				FText::FromString(VadName),
				TierLabel,
				FText::FromString(TryllDocs::VoiceInput)));
			if (Answer == EAppReturnType::Yes)
			{
				TArray<FString>       VadLanguages;
				ETryllInferenceEngine VadEngine   = ETryllInferenceEngine::SherpaOnnx;
				bool                  bHaveVadRow = false;
				if (const FTryllModelInfo* V = Models.FindByPredicate(
						[&](const FTryllModelInfo& X){ return X.Name == VadName; }))
				{
					VadLanguages = V->Languages;
					VadEngine    = ParseEngine(V->Engine);
					bHaveVadRow  = true;
				}
				Manifest->Set(VadName, TEXT("vad"), Reg, bHaveVadRow ? &VadLanguages : nullptr,
				              bHaveVadRow ? &VadEngine : nullptr);
			}
		}
	}

	Manifest->SaveToConfig();
	RebuildAll();

	// A newly-registered model may now be Absent-and-required; the PIE guard's cached
	// "verified ready" flag must not mask that on the next Play press.
	FTryllPIEModelGuard::Invalidate();
}

// ── Downloads popover (Chrome-style) ──────────────────────────────────────────

int32 STryllModelManagerPanel::ActiveDownloadCount() const
{
	return FTryllModelManagerPresentation::ActiveDownloadCount(Downloads);
}

void STryllModelManagerPanel::NoteDownloadProgress(const FString& Name, uint64 Bytes, uint64 Total, float Percent)
{
	const bool bNew = FTryllModelManagerPresentation::ApplyDownloadProgress(
		Downloads, Name, Bytes, Total, Percent, FPlatformTime::Seconds());

	if (bNew && DownloadsAnchor.IsValid() && !DownloadsAnchor->IsOpen())
		DownloadsAnchor->SetIsOpen(true);
	if (bNew && DownloadsList.IsValid()) PopulateDownloads();
}

void STryllModelManagerPanel::NoteDownloadComplete(const FString& Name, bool bSuccess, const FString& Error)
{
	FTryllModelManagerPresentation::ApplyDownloadComplete(Downloads, Name, bSuccess, Error);
	if (DownloadsList.IsValid()) PopulateDownloads();
}

TSharedRef<SWidget> STryllModelManagerPanel::BuildDownloadsPopover()
{
	SAssignNew(DownloadsList, SVerticalBox);
	PopulateDownloads();

	return SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("Menu.Background"))
		.Padding(0)
		[
			SNew(SBox).WidthOverride(340.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(12, 10, 12, 6)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
					[ SNew(STextBlock).Text(LOCTEXT("DownloadsHdr", "Downloads")).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10)) ]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SButton).ButtonStyle(FAppStyle::Get(), "NoBorder")
						.ToolTipText(LOCTEXT("ClearDoneTip", "Clear finished downloads"))
						.OnClicked_Lambda([this]
						{
							FTryllModelManagerPresentation::ClearFinishedDownloads(Downloads);
							if (DownloadsList.IsValid()) PopulateDownloads();
							return FReply::Handled();
						})
						[ SNew(STextBlock).Text(LOCTEXT("ClearDone", "Clear")).ColorAndOpacity(FSlateColor(Dim)).Font(FCoreStyle::GetDefaultFontStyle("Regular", 9)) ]
					]
				]
				+ SVerticalBox::Slot().AutoHeight()[ SNew(SSeparator) ]
				+ SVerticalBox::Slot().AutoHeight().MaxHeight(420.f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()[ DownloadsList.ToSharedRef() ]
				]
			]
		];
}

void STryllModelManagerPanel::PopulateDownloads()
{
	if (!DownloadsList.IsValid()) return;
	DownloadsList->ClearChildren();

	if (Downloads.Num() == 0)
	{
		DownloadsList->AddSlot().AutoHeight().Padding(14, 18)
		[ SNew(STextBlock).Text(LOCTEXT("NoDownloads", "No recent downloads.")).ColorAndOpacity(FSlateColor(Dim)) ];
		return;
	}

	for (const FTryllDownloadJob& J : Downloads)
	{
		const bool bActive = J.Phase == FTryllDownloadJob::EPhase::Active;
		const bool bFailed = J.Phase == FTryllDownloadJob::EPhase::Failed;
		const FString      Dot    = bActive ? TEXT("●") : bFailed ? TEXT("✗") : TEXT("✓");
		const FLinearColor DotCol = bActive ? ColDownloading : bFailed ? ColMissing : ColProd;

		TSharedRef<SVerticalBox> Card = SNew(SVerticalBox);
		Card->AddSlot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
			[ SNew(STextBlock).Text(FText::FromString(Dot)).ColorAndOpacity(FSlateColor(DotCol)) ]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[ SNew(STextBlock).Text(FText::FromString(J.Name)).Font(FCoreStyle::GetDefaultFontStyle("Regular", 9)) ]
		];

		if (bActive)
		{
			// Bound by name so steady progress updates these widgets in place
			// (no per-frame rebuild → no flicker). The card list is only rebuilt
			// when a job is added or changes phase.
			const FString JobName = J.Name;

			Card->AddSlot().AutoHeight().Padding(0, 4, 0, 2)
			[
				SNew(SProgressBar)
				.Percent_Lambda([this, JobName]() -> TOptional<float>
				{
					const FTryllDownloadJob* Jb = Downloads.FindByPredicate([&](const FTryllDownloadJob& X){ return X.Name == JobName; });
					if (!Jb) return 0.f;
					return Jb->TotalBytes > 0 ? static_cast<float>(static_cast<double>(Jb->BytesDownloaded) / static_cast<double>(Jb->TotalBytes)) : (Jb->Percent / 100.f);
				})
			];

			Card->AddSlot().AutoHeight()
			[
				SNew(STextBlock)
				.Text_Lambda([this, JobName]
				{
					const FTryllDownloadJob* Jb = Downloads.FindByPredicate([&](const FTryllDownloadJob& X){ return X.Name == JobName; });
					if (!Jb) return FText::GetEmpty();
					const float Remain = (Jb->SpeedBytesPerSec > 1.f && Jb->TotalBytes > Jb->BytesDownloaded)
						? static_cast<float>(static_cast<double>(Jb->TotalBytes - Jb->BytesDownloaded) / Jb->SpeedBytesPerSec) : 0.f;
					return FText::FromString(FTryllModelManagerPresentation::FormatDownloadProgress(
						(int64)Jb->BytesDownloaded, (int64)Jb->TotalBytes, Jb->SpeedBytesPerSec, Remain));
				})
				.ColorAndOpacity(FSlateColor(Dim)).Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			];
		}
		else
		{
			const int64   Size = (int64)(J.TotalBytes > 0 ? J.TotalBytes : J.BytesDownloaded);
			const FString Line = bFailed
				? FTryllModelManagerPresentation::FormatDownloadFailed(J.Error)
				: FTryllModelManagerPresentation::FormatDownloadDone(Size);
			Card->AddSlot().AutoHeight()
			[ SNew(STextBlock).Text(FText::FromString(Line)).ColorAndOpacity(FSlateColor(bFailed ? ColMissing : Dim)).Font(FCoreStyle::GetDefaultFontStyle("Regular", 8)) ];
		}

		DownloadsList->AddSlot().AutoHeight().Padding(12, 8)[ Card ];
		DownloadsList->AddSlot().AutoHeight()[ SNew(SSeparator).Thickness(0.5f) ];
	}
}

// ── Status helpers ──────────────────────────────────────────────────────────

void STryllModelManagerPanel::RefreshStatusChrome()
{
	if (!StatusBar.IsValid())
	{
		return;
	}

	ETryllOwnerPhase Owner = EditorSubsystem
		? EditorSubsystem->GetOwnerPhase()
		: ETryllOwnerPhase::Idle;
	FString Detail = EditorSubsystem ? EditorSubsystem->GetOwnerDetail() : FString();
	if (State == EState::Error)
	{
		Owner = ETryllOwnerPhase::Error;
		Detail = Status;
	}

	const ETryllPanelOperation LocalOp = State == EState::Loading
		? ETryllPanelOperation::Listing
		: ETryllPanelOperation::None;
	StatusBar->Bind(FTryllStatusPresentation::Compose(
		Owner, Detail, LocalOp, ETryllEditorPanel::ModelManager, Models.Num()));

	if (Banner.IsValid())
	{
		if (!OperationalError.IsEmpty())
		{
			Banner->Bind(OperationalError, ETryllStatusSeverity::Error);
		}
		else
		{
			Banner->Clear();
		}
	}
}

void STryllModelManagerPanel::HandleOwnerPhaseChanged(ETryllOwnerPhase /*Phase*/, const FString& /*Detail*/)
{
	++DebugOwnerPhaseHandlerCount;
	RefreshStatusChrome();
}

void STryllModelManagerPanel::SetState(EState S, const FString& InStatus)
{
	State  = S;
	Status = InStatus;
	RefreshStatusChrome();
}

void STryllModelManagerPanel::Fail(const FString& Reason)
{
	OperationalError = Reason;
	SetState(EState::Error, Reason);
	UE_LOG(LogTemp, Warning, TEXT("STryllModelManagerPanel: %s"), *Reason);
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void STryllModelManagerPanel::AutofindSettings()
{
	if (Settings.IsValid()) return;
	Settings = GetMutableDefault<UTryllRuntimeSettings>();
}

void STryllModelManagerPanel::OnRefreshClicked()
{
	if (EditorSubsystem && State == EState::Ready) RequestList();
	else if (State == EState::Idle)        Start();
	else if (State == EState::Error)
	{
		Stop();
		Start();
	}
}

FTryllPanelSnapshot STryllModelManagerPanel::DebugSnapshotForTests() const
{
	static const TCHAR* StateNames[] = { TEXT("Idle"), TEXT("Connecting"), TEXT("Configuring"), TEXT("Loading"), TEXT("Ready"), TEXT("Error") };
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
	for (const FTryllModelInfo& Model : Models)
	{
		Out.Rows.Add(Model.Name);
	}
	Out.bLeaseHeld = bAcquiredConnection;
	Out.ListGeneration = ListGeneration;
	return Out;
}

void STryllModelManagerPanel::RetryOwner()
{
	const bool bSessionReady = EditorSubsystem && EditorSubsystem->IsSessionReady();
	if (TryllEditorConnectionPolicy::ShouldRetryLocalWork(bAcquiredConnection, bSessionReady))
	{
		RequestList();
		return;
	}
	if (bAcquiredConnection)
	{
		Stop();
	}
	Start();
}

void STryllModelManagerPanel::Start()
{
	if (!Settings.IsValid()) AutofindSettings();
	if (!Settings.IsValid()) { Fail(TEXT("No TryllRuntimeSettings available")); return; }

	if (!TryllEditorConnectionPolicy::ShouldBindAndAcquire(bAcquiredConnection))
	{
		if (EditorSubsystem && EditorSubsystem->IsConnected())
		{
			HandleConnectionChanged(true);
		}
		return;
	}

	if (!EditorSubsystem)
	{
		EditorSubsystem = EditorSubsystemOverride
			? EditorSubsystemOverride
			: (GEditor ? GEditor->GetEditorSubsystem<UTryllEditorSubsystem>() : nullptr);
	}
	if (!EditorSubsystem) { Fail(TEXT("Shared editor subsystem unavailable")); return; }

	bMissingModelsChecked = false;

	EditorSubsystem->OnConnectionChangedNative.AddSP(this, &STryllModelManagerPanel::HandleConnectionChanged);
	EditorSubsystem->OnCreateSessionCompleteNative.AddSP(this, &STryllModelManagerPanel::HandleCreateSessionComplete);
	EditorSubsystem->OnErrorNative.AddSP(this, &STryllModelManagerPanel::HandleError);
	OwnerPhaseHandle = EditorSubsystem->OnOwnerPhaseChangedNative.AddSP(
		this, &STryllModelManagerPanel::HandleOwnerPhaseChanged);
	EditorSubsystem->OnDownloadProgressNative.AddSP(this, &STryllModelManagerPanel::HandleDownloadProgress);
	EditorSubsystem->OnDownloadCompleteNative.AddSP(this, &STryllModelManagerPanel::HandleDownloadComplete);
	EditorSubsystem->OnLoadModelCompleteNative.AddSP(this, &STryllModelManagerPanel::HandleLoadModelComplete);
	EditorSubsystem->OnUnloadModelCompleteNative.AddSP(this, &STryllModelManagerPanel::HandleUnloadModelComplete);
	EditorSubsystem->OnDeleteModelCompleteNative.AddSP(this, &STryllModelManagerPanel::HandleDeleteModelComplete);

	SetState(EState::Connecting, FTryllEditorUiContract::StatusConnecting);
	bAcquiredConnection = true;
	EditorSubsystem->AcquireConnection();

	// Join-late: another consumer (PIE, the chat panel) may have already brought
	// the shared connection up before this panel opened.
	if (EditorSubsystem->IsConnected())
	{
		HandleConnectionChanged(true);
	}
}

void STryllModelManagerPanel::Stop()
{
	if (EditorSubsystem)
	{
		EditorSubsystem->OnConnectionChangedNative.RemoveAll(this);
		EditorSubsystem->OnCreateSessionCompleteNative.RemoveAll(this);
		EditorSubsystem->OnErrorNative.RemoveAll(this);
		EditorSubsystem->OnOwnerPhaseChangedNative.Remove(OwnerPhaseHandle);
		OwnerPhaseHandle.Reset();
		EditorSubsystem->OnDownloadProgressNative.RemoveAll(this);
		EditorSubsystem->OnDownloadCompleteNative.RemoveAll(this);
		EditorSubsystem->OnLoadModelCompleteNative.RemoveAll(this);
		EditorSubsystem->OnUnloadModelCompleteNative.RemoveAll(this);
		EditorSubsystem->OnDeleteModelCompleteNative.RemoveAll(this);

		// Mirrors the Unity fix: release our own acquisition rather than forcing
		// a disconnect that would tear down the connection for other consumers.
		if (bAcquiredConnection)
		{
			EditorSubsystem->ReleaseConnection();
		}
	}
	bAcquiredConnection = false;
	EditorSubsystem = nullptr;
	++ListGeneration;
	SetState(EState::Idle, TEXT("Idle"));
}

void STryllModelManagerPanel::RequestList()
{
	if (!EditorSubsystem) return;
	const int32 Generation = ++ListGeneration;
	SetState(EState::Loading, TEXT("Loading models…"));
	TWeakPtr<STryllModelManagerPanel> WeakPanel = SharedThis(this);
	EditorSubsystem->RequestListModels([WeakPanel, Generation](TArray<FTryllModelInfo> NewModels, FTryllError Error)
	{
		TSharedPtr<STryllModelManagerPanel> Panel = WeakPanel.Pin();
		if (!Panel.IsValid()
			|| !FTryllModelManagerPresentation::ShouldApplyListGeneration(Generation, Panel->ListGeneration))
		{
			return;
		}
		if (!Error.IsOk())
		{
			Panel->Fail(FString::Printf(TEXT("ListModels failed: %s"), *Error.Message));
			return;
		}
		Panel->HandleListModelsOk(NewModels);
	});
}

void STryllModelManagerPanel::HandleListModelsOk(const TArray<FTryllModelInfo>& NewModels)
{
	const FString PrevSel = Models.IsValidIndex(SelectedIndex) ? Models[SelectedIndex].Name : FString();
	Models = NewModels;

	// Backfill the catalog data the manifest caches — languages and engine — for
	// projects whose registrations predate either field. Languages keep the offline
	// settings picker and inspector warnings useful; the engine is what lets a session
	// pre-warm the right backend instead of nothing.
	UTryllModelManifest* Manifest = UTryllModelManifest::GetMutableActive();
	bool bManifestChanged = false;
	for (const FTryllModelInfo& Model : Models)
	{
		const ETryllModelRegistration Registration = Manifest->Get(Model.Name);
		if (Registration == ETryllModelRegistration::None)
		{
			continue;
		}
		// Backfill every cached catalog field on every refresh and let Set say whether
		// anything moved. This used to skip the call whenever cached languages matched
		// the server, which meant the cached *engine* was never corrected: a manifest
		// written before that field existed kept the enum default and pre-warmed nothing.
		// Comparing here at all was the mistake — the manifest owns what it caches.
		const ETryllInferenceEngine BackfillEngine = ParseEngine(Model.Engine);
		bManifestChanged |= Manifest->Set(
			Model.Name, Model.ModelType, Registration, &Model.Languages,
			&BackfillEngine);
	}
	if (bManifestChanged)
		Manifest->SaveToConfig();

	SelectedIndex = INDEX_NONE;
	if (!PrevSel.IsEmpty())
	{
		for (int32 I = 0; I < Models.Num(); ++I) if (Models[I].Name == PrevSel) { SelectedIndex = I; break; }
	}
	if (SelectedIndex == INDEX_NONE && Models.Num() > 0) SelectedIndex = 0;
	// The session may have been configured by another window (PIE, the chat
	// panel) with different engines before this panel asked for its own —
	// CreateSession is one-shot per connection, so that config wins silently.
	// A successful list proves the connection and session are back, so any
	// earlier "Disconnected" / CreateSession / ListModels error is stale (Unity
	// LoadModelsAsync clears it the same way).
	OperationalError.Reset();
	SetState(EState::Ready, FString::Printf(TEXT("Ready · %d models (shared session)"), Models.Num()));
	RebuildAll();

	if (!bMissingModelsChecked)
	{
		bMissingModelsChecked = true;
		PromptDownloadMissingRegisteredModels();
	}
}

// ── Shared subsystem event handlers ─────────────────────────────────────────────

void STryllModelManagerPanel::HandleConnectionChanged(bool bConnected)
{
	if (!bConnected)
	{
		const bool bRetriesRemaining =
			EditorSubsystem && EditorSubsystem->HasConnectRetriesRemaining();
		if (TryllEditorConnectionPolicy::ShouldWaitForOwnerRetry(
				State == EState::Connecting,
				bRetriesRemaining))
		{
			SetState(EState::Connecting, TEXT("Waiting for server…"));
			return;
		}
		Fail(LastErrorMessage.IsEmpty() ? FString(TEXT("Disconnected")) : LastErrorMessage);
		return;
	}
	if (!EditorSubsystem) return;

	SetState(EState::Configuring, TEXT("Configuring session…"));
	// The "declare real engines so ListModels surfaces every model type" trick is
	// gone: ListModels is engine-agnostic since v13, reporting every catalog model
	// with its own engine as data. Engines come from the manifest.
	EditorSubsystem->CreateSession(FString());
	// If the session is already configured (join-late), CreateSession() above
	// broadcasts synchronously and HandleCreateSessionComplete() below already
	// subscribed — no separate check needed here.
}

void STryllModelManagerPanel::HandleCreateSessionComplete(bool bSuccess)
{
	if (!bSuccess) { Fail(FString::Printf(TEXT("CreateSession failed: %s"), *LastErrorMessage)); return; }
	if (State == EState::Ready) return;   // already listed models; ignore redundant broadcast
	RequestList();
}

void STryllModelManagerPanel::HandleError(const FString& ErrorMessage)
{
	// Cached for whichever paired bool-only completion fires next — see
	// LastErrorMessage's declaration for why this is always fresh when read.
	LastErrorMessage = ErrorMessage;
}

void STryllModelManagerPanel::HandleDownloadProgress(
	const FString& ModelName,
	uint64         BytesDownloaded,
	uint64         TotalBytes,
	float          Percent)
{
	Progress.Add(ModelName, Percent);
	NoteDownloadProgress(ModelName, BytesDownloaded, TotalBytes, Percent);
	// Only refresh on the first frame (status transition) — the inline bar is
	// lambda-bound, so steady progress needs no rebuild.
	bool bBecameDownloading = false;
	for (FTryllModelInfo& M : Models)
		if (M.Name == ModelName && M.Status != ETryllModelStatus::Downloading)
		{
			M.Status = FTryllModelManagerPresentation::StatusAfterDownloadProgress();
			bBecameDownloading = true;
		}
	if (bBecameDownloading)
	{
		RebuildList();
		if (Models.IsValidIndex(SelectedIndex) && Models[SelectedIndex].Name == ModelName) RebuildProperties();
	}
}

void STryllModelManagerPanel::HandleDownloadComplete(
	const FString& ModelName,
	bool           bSuccess,
	const FString& ErrorMessage)
{
	Progress.Remove(ModelName);
	NoteDownloadComplete(ModelName, bSuccess, bSuccess ? FString() : ErrorMessage);
	if (!bSuccess)
	{
		OperationalError = ErrorMessage.IsEmpty()
			? FString::Printf(TEXT("Download failed: %s"), *ModelName)
			: FString::Printf(TEXT("Download failed: %s"), *ErrorMessage);
		RefreshStatusChrome();
	}
	RequestList(); // re-list to pick up the true status + on-disk size
}

void STryllModelManagerPanel::HandleLoadModelComplete(const FString& ModelName, bool bSuccess)
{
	if (bSuccess)
	{
		RequestList();
		return;
	}
	OperationalError = FString::Printf(TEXT("Load failed: %s"), *ModelName);
	RefreshStatusChrome();
}

void STryllModelManagerPanel::HandleUnloadModelComplete(const FString& ModelName, bool bSuccess)
{
	if (bSuccess)
	{
		RequestList();
		return;
	}
	OperationalError = FString::Printf(TEXT("Unload failed: %s"), *ModelName);
	RefreshStatusChrome();
}

void STryllModelManagerPanel::HandleDeleteModelComplete(const FString& ModelName, bool bSuccess)
{
	(void)ModelName;
	if (bSuccess) RequestList();
	else          Fail(FString::Printf(TEXT("Delete failed: %s"), *LastErrorMessage));
}

// ── Tick ──────────────────────────────────────────────────────────────────────

void STryllModelManagerPanel::Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	// Connection events are pumped centrally by the shared UTryllEditorSubsystem, not here.
}

#undef LOCTEXT_NAMESPACE
