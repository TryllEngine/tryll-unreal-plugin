// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllRuntimeSettingsCustomization.h"
#include "TryllEditorPrompts.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "TryllClientEditorModule.h"
#include "TryllLanguages.h"
#include "TryllModelCatalogFile.h"
#include "TryllModelManifest.h"
#include "TryllRuntimeSettings.h"
#include "TryllSubsystem.h"

#include "DesktopPlatformModule.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/Paths.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "TryllRuntimeSettingsCustomization"

namespace
{

/** Model types whose language coverage actually constrains a session. */
const TArray<FString>& VoiceModelTypes()
{
	static const TArray<FString> Types = { TEXT("tts"), TEXT("stt") };
	return Types;
}

} // namespace

TSharedRef<IDetailCustomization> FTryllRuntimeSettingsCustomization::MakeInstance()
{
	return MakeShared<FTryllRuntimeSettingsCustomization>();
}

void FTryllRuntimeSettingsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	if (UTryllSubsystem::GetAvailableServerVariants().Num() <= 1)
	{
		DetailBuilder.HideProperty(
			GET_MEMBER_NAME_CHECKED(UTryllRuntimeSettings, ServerBuildVariant),
			UTryllRuntimeSettings::StaticClass());
	}

	CustomizeLocaleRow(DetailBuilder);
	CustomizeUserModelsRow(DetailBuilder);
	CustomizeStorageRow(DetailBuilder);
}

void FTryllRuntimeSettingsCustomization::CustomizeUserModelsRow(IDetailLayoutBuilder& DetailBuilder)
{
	TSharedPtr<IPropertyHandle> Handle = DetailBuilder.GetProperty(
		GET_MEMBER_NAME_CHECKED(UTryllRuntimeSettings, UserModelsCatalogPath),
		UTryllRuntimeSettings::StaticClass());
	if (!Handle.IsValid())
	{
		return;
	}

	DetailBuilder.HideProperty(Handle);

	// Captured by the commit lambda. SAssignNew fills it after the lambda is
	// built, and the callback runs later, so the inner pointer is valid then.
	TSharedRef<TSharedPtr<SEditableTextBox>> TextBoxSlot = MakeShared<TSharedPtr<SEditableTextBox>>();

	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(TEXT("Models"));
	Category.AddCustomRow(LOCTEXT("UserModelsFilter", "Custom models catalog"))
	.NameContent()
	[
		Handle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(360.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f)
		[
			SAssignNew(*TextBoxSlot, SEditableTextBox)
			.Text_Lambda([Handle]
			{
				FString Value;
				Handle->GetValue(Value);
				return FText::FromString(Value);
			})
			.OnTextCommitted_Lambda([Handle, TextBoxSlot](const FText& NewText, ETextCommit::Type)
			{
				FString Error;
				if (!UTryllRuntimeSettings::SetUserModelsCatalogPath(NewText.ToString(), Error))
				{
					FString Current;
					Handle->GetValue(Current);
					if (TextBoxSlot->IsValid())
					{
						(*TextBoxSlot)->SetText(FText::FromString(Current));
					}
					UE_LOG(LogTryllEditor, Warning, TEXT("[Tryll] %s"), *Error);
					return;
				}
				if (const UTryllRuntimeSettings* Settings = GetDefault<UTryllRuntimeSettings>())
				{
					Handle->SetValue(Settings->UserModelsCatalogPath);
				}
			})
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f, 0.f, 0.f)
		[
			SNew(SButton)
			.Text(LOCTEXT("UserModelsBrowse", "Browse"))
			.OnClicked_Lambda([Handle]
			{
				TArray<FString> Files;
				const bool bOpened = FTryllEditorPrompts::OpenFileDialog(
					FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
					TEXT("Custom models catalog"),
					FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()),
					TEXT(""),
					TEXT("Models catalog (*.json)|*.json"),
					EFileDialogFlags::None,
					Files);
				if (!bOpened || Files.IsEmpty())
				{
					return FReply::Handled();
				}

				FString Error;
				if (!UTryllRuntimeSettings::SetUserModelsCatalogPath(Files[0], Error))
				{
					UE_LOG(LogTryllEditor, Warning, TEXT("[Tryll] %s"), *Error);
					return FReply::Handled();
				}
				if (const UTryllRuntimeSettings* Settings = GetDefault<UTryllRuntimeSettings>())
				{
					Handle->SetValue(Settings->UserModelsCatalogPath);
				}
				return FReply::Handled();
			})
		]
	];

	Category.AddCustomRow(LOCTEXT("UserModelsHelpFilter", "Custom models catalog help"))
	.WholeRowContent()
	[
		SNew(STextBlock)
		.AutoWrapText(true)
		.Text(LOCTEXT("UserModelsHelpV2",
			"Optional. A models catalog JSON inside this project — any file name, any folder. "
			"Leave empty if you are not adding models. The path is stored relative to the project "
			"and cannot point outside it. Set it before the server starts (GameInstance Init), "
			"or call Set User Models Catalog Path later to restart a running server. "
			"A packaged game copies the saved file to Server/data/user-catalog/."))
	];
}

void FTryllRuntimeSettingsCustomization::CustomizeStorageRow(IDetailLayoutBuilder& DetailBuilder)
{
	TSharedPtr<IPropertyHandle> Handle = DetailBuilder.GetProperty(
		GET_MEMBER_NAME_CHECKED(UTryllRuntimeSettings, StorageDataFolder),
		UTryllRuntimeSettings::StaticClass());
	if (!Handle.IsValid())
	{
		return;
	}

	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(TEXT("Storage"));

	Category.AddCustomRow(LOCTEXT("StorageContentErrorFilter", "Storage under Content"))
	.Visibility(TAttribute<EVisibility>::CreateLambda([Handle]
	{
		FString Value;
		Handle->GetValue(Value);
		return UTryllRuntimeSettings::IsStorageUnderContent(Value)
			? EVisibility::Visible
			: EVisibility::Collapsed;
	}))
	.WholeRowContent()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 2.f)
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.ColorAndOpacity(FSlateColor(FLinearColor(0.91f, 0.30f, 0.24f)))
			.Text(LOCTEXT("StorageUnderContentError",
				"StorageDataFolder must be outside Content/. Unreal packs Content/ into .pak "
				"archives the Tryll server cannot read, and the editor may try to import "
				"server-written files. Default is TryllStorage next to the .uproject."))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("MoveToTryllStorage", "Move to TryllStorage"))
				.ToolTipText(LOCTEXT("MoveToTryllStorageTip",
					"Move the configured folder to <Project>/TryllStorage and update this setting."))
				.OnClicked_Lambda([Handle]
				{
					FString Error;
					if (!UTryllRuntimeSettings::MoveStorageToTryllStorage(Error))
					{
						UE_LOG(LogTryllEditor, Warning, TEXT("[Tryll] %s"), *Error);
						return FReply::Handled();
					}
					if (const UTryllRuntimeSettings* Settings = GetDefault<UTryllRuntimeSettings>())
					{
						Handle->SetValue(Settings->StorageDataFolder);
					}
					return FReply::Handled();
				})
			]
		]
	];

	Category.AddCustomRow(LOCTEXT("StorageHelpFilter", "Storage help"))
	.WholeRowContent()
	[
		SNew(STextBlock)
		.AutoWrapText(true)
		.ColorAndOpacity(FSlateColor(FLinearColor(0.60f, 0.62f, 0.66f)))
		.Text(LOCTEXT("StorageHelp",
			"Process-wide folder for knowledge bases, voices, guardrails, and models. "
			"Must be inside the project and outside Content/. A packaged build stages it "
			"as loose files under TryllStorage next to the project."))
	];
}

FText FTryllRuntimeSettingsCustomization::LocaleOptionLabel(const FString& Code)
{
	if (Code.IsEmpty())
	{
		return LOCTEXT("LocaleNone", "(none - no session language)");
	}

	const FText Base = FText::Format(
		LOCTEXT("LocaleLabel", "{0} ({1})"),
		FText::FromString(UTryllLanguages::LanguageDisplayName(Code)),
		FText::FromString(Code));

	// Annotate with how many models in the shipped catalog can serve it, so a
	// language that hangs off a single model is visible before it is chosen.
	// Silent when the catalog cannot be read - a missing count is better than a
	// zero, which would read as "nothing supports this".
	if (const int32* Count = FTryllModelCatalogFile::CountsByLanguage().Find(Code.ToLower()))
	{
		return FText::Format(
			(*Count == 1)
				? LOCTEXT("LocaleLabelOneModel",  "{0}  -  1 model")
				: LOCTEXT("LocaleLabelNModels",   "{0}  -  {1} models"),
			Base, FText::AsNumber(*Count));
	}
	return Base;
}

FString FTryllRuntimeSettingsCustomization::CurrentLocale() const
{
	FString Value;
	if (LocaleHandle.IsValid())
	{
		LocaleHandle->GetValue(Value);
	}
	return Value;
}

void FTryllRuntimeSettingsCustomization::RebuildLocaleOptions()
{
	LocaleOptions.Reset();

	// Empty first: "no session language" is a meaningful choice (pre-locale
	// behaviour), not a missing value.
	LocaleOptions.Add(MakeShared<FString>(FString()));

	for (const FString& Code : UTryllModelManifest::GetActive()->DeclaredLanguages())
	{
		LocaleOptions.Add(MakeShared<FString>(Code));
	}

	// A value set by hand (or left over after unregistering the model that
	// declared it) must never vanish from the list and be silently rewritten.
	const FString Current = CurrentLocale();
	if (!Current.IsEmpty())
	{
		const bool bPresent = LocaleOptions.ContainsByPredicate(
			[&Current](const TSharedPtr<FString>& Option)
			{
				return Option.IsValid() && *Option == Current;
			});
		if (!bPresent)
		{
			LocaleOptions.Add(MakeShared<FString>(Current));
		}
	}
}

FText FTryllRuntimeSettingsCustomization::LocaleSummary() const
{
	const FString Current = CurrentLocale();
	if (Current.IsEmpty())
	{
		return LOCTEXT("LocaleSummaryNone",
			"No language selected. Voice output uses each model's own default, storage "
			"paths resolve to the linked file, and the {{language}} template key renders "
			"empty.");
	}

	const FString Language = UTryllLanguages::LanguageOf(Current);

	// The offered list is a union across registered models, so the pick may be
	// one some registered model cannot serve. Name them - it is a warning, not a
	// block, because a node with its own Tts Lang is unaffected.
	const TArray<FString> Missing =
		UTryllModelManifest::GetActive()->ModelsMissingLanguage(Language, VoiceModelTypes());
	if (Missing.Num() > 0)
	{
		return FText::Format(
			LOCTEXT("LocaleSummaryMissing",
				"{0} registered voice model(s) cannot do this language: {1}.\n"
				"That only matters for nodes relying on the session language - a node with "
				"its own Tts Lang is unaffected. Agents that do rely on it are rejected at "
				"creation with a clear error."),
			FText::AsNumber(Missing.Num()),
			FText::FromString(FString::Join(Missing, TEXT(", "))));
	}

	if (FTryllModelCatalogFile::IsAvailable())
	{
		const int32* Count = FTryllModelCatalogFile::CountsByLanguage().Find(Language.ToLower());
		return FText::Format(
			LOCTEXT("LocaleSummaryCatalog", "{0} of {1} catalog models declare this language."),
			FText::AsNumber(Count ? *Count : 0),
			FText::AsNumber(FTryllModelCatalogFile::ModelCount()));
	}
	return FText::GetEmpty();
}

void FTryllRuntimeSettingsCustomization::CustomizeLocaleRow(IDetailLayoutBuilder& DetailBuilder)
{
	LocaleHandle = DetailBuilder.GetProperty(
		GET_MEMBER_NAME_CHECKED(UTryllRuntimeSettings, Locale),
		UTryllRuntimeSettings::StaticClass());
	if (!LocaleHandle.IsValid())
	{
		return;
	}

	// Replace the default text row rather than adding beside it.
	DetailBuilder.HideProperty(LocaleHandle);
	RebuildLocaleOptions();

	IDetailCategoryBuilder& Category =
		DetailBuilder.EditCategory(TEXT("Localization"));

	Category.AddCustomRow(LOCTEXT("LocaleFilter", "Locale"))
	.NameContent()
	[
		LocaleHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(320.f)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SComboBox<TSharedPtr<FString>>)
			.OptionsSource(&LocaleOptions)
			.OnComboBoxOpening_Lambda([this] { RebuildLocaleOptions(); })
			.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
			{
				return SNew(STextBlock)
					.Text(LocaleOptionLabel(Item.IsValid() ? *Item : FString()));
			})
			.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type)
			{
				if (Item.IsValid() && LocaleHandle.IsValid())
				{
					// Store the bare code, never the annotated label.
					LocaleHandle->SetValue(*Item);
				}
			})
			[
				SNew(STextBlock)
				.Text_Lambda([this] { return LocaleOptionLabel(CurrentLocale()); })
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
		[
			SNew(STextBlock)
			.Text_Lambda([this] { return LocaleSummary(); })
			.ColorAndOpacity(FSlateColor(FLinearColor(0.60f, 0.62f, 0.66f)))
			.AutoWrapText(true)
			.Visibility_Lambda([this]
			{
				return LocaleSummary().IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
			})
		]
	];
}

#undef LOCTEXT_NAMESPACE
