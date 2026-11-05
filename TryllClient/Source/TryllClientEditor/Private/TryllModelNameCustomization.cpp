// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllModelNameCustomization.h"

#include "TryllModelName.h"
#include "TryllModelManifest.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"
#include "Styling/AppStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SComboBox.h"
#include "TryllLanguages.h"
#include "TryllRuntimeSettings.h"
#include "UI/TryllEditorStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "TryllModelNameCustomization"

namespace
{
	FText TierLabel(const FString& ModelName)
	{
		if (ModelName.IsEmpty()) return FText::GetEmpty();
		switch (UTryllModelManifest::GetActive()->Get(ModelName))
		{
		case ETryllModelRegistration::Production:   return LOCTEXT("TierProd", "Production");
		case ETryllModelRegistration::Experimental: return LOCTEXT("TierExp", "Experimental");
		default:                                    return LOCTEXT("TierNone", "Unregistered");
		}
	}

	FSlateColor TierColor(const FString& ModelName)
	{
		switch (UTryllModelManifest::GetActive()->Get(ModelName))
		{
		case ETryllModelRegistration::Production:   return FSlateColor(FTryllEditorStyle::ColOk);
		case ETryllModelRegistration::Experimental: return FSlateColor(FTryllEditorStyle::ColWarn);
		default:                                    return FSlateColor(FTryllEditorStyle::Dim);
		}
	}
}

TSharedRef<IPropertyTypeCustomization> FTryllModelNameCustomization::MakeInstance()
{
	return MakeShared<FTryllModelNameCustomization>();
}

FString FTryllModelNameCustomization::CurrentValue() const
{
	FString Value;
	if (NameHandle.IsValid()) NameHandle->GetValue(Value);
	return Value;
}

void FTryllModelNameCustomization::SetValue(const FString& InValue)
{
	if (NameHandle.IsValid()) NameHandle->SetValue(InValue);
}

void FTryllModelNameCustomization::RebuildOptions()
{
	Options.Reset();
	Options.Add(MakeShared<FString>(FString())); // "(none)"
	for (const FString& M : UTryllModelManifest::GetActive()->ModelsOfType(ModelType))
		Options.Add(MakeShared<FString>(M));

	// Keep the current value visible even if it is no longer registered.
	const FString Cur = CurrentValue();
	if (!Cur.IsEmpty() && !Options.ContainsByPredicate([&](const TSharedPtr<FString>& O){ return O.IsValid() && *O == Cur; }))
		Options.Add(MakeShared<FString>(Cur));
}

void FTryllModelNameCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& /*CustomizationUtils*/)
{
	NameHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTryllModelName, Name));
	if (!NameHandle.IsValid()) return;

	ModelType = StructPropertyHandle->GetMetaData(TEXT("TryllModelType"));
	RebuildOptions();

	HeaderRow.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(280.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 6.f, 0.f)
		[
			SNew(SComboBox<TSharedPtr<FString>>)
			.OptionsSource(&Options)
			.OnComboBoxOpening_Lambda([this] { RebuildOptions(); })
			.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
			{
				const FString N = InItem.IsValid() ? *InItem : FString();
				return SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f)
					[ SNew(STextBlock).Text(N.IsEmpty() ? LOCTEXT("None", "(none)") : FText::FromString(N)) ]
					+ SHorizontalBox::Slot().AutoWidth().Padding(10.f, 0.f, 0.f, 0.f)
					[ SNew(STextBlock).Text(TierLabel(N)).ColorAndOpacity(TierColor(N)) ];
			})
			.OnSelectionChanged_Lambda([this](TSharedPtr<FString> InItem, ESelectInfo::Type)
			{
				if (InItem.IsValid()) SetValue(*InItem);
			})
			[
				SNew(STextBlock).Text_Lambda([this]
				{
					const FString Cur = CurrentValue();
					return Cur.IsEmpty() ? LOCTEXT("None", "(none)") : FText::FromString(Cur);
				})
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text_Lambda([this] { return TierLabel(CurrentValue()); })
			.ColorAndOpacity_Lambda([this] { return TierColor(CurrentValue()); })
		]
		// Language mismatch against the project's selected language. Mirrors the
		// server's TTS-agent / STT-voice-input creation checks at pick time.
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
		[
			SNew(SImage)
			.Image(FAppStyle::GetBrush("Icons.Warning"))
			.ColorAndOpacity(FSlateColor(FTryllEditorStyle::ColWarn))
			.ToolTipText_Lambda([this] { return LanguageWarning(); })
			.Visibility_Lambda([this]
			{
				return LanguageWarning().IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
			})
		]
	];
}

FText FTryllModelNameCustomization::LanguageWarning() const
{
	// Silent in three cases, all meaning "no evidence of a problem" rather than
	// "fine": not a voice model, nothing picked / no project language, or the
	// model declares no languages at all. Only TTS and STT consume the session
	// locale - warning on a language model would be noise, since language
	// competence is a quality gradient we deliberately do not declare.
	if (ModelType != TEXT("tts") && ModelType != TEXT("stt"))
		return FText::GetEmpty();

	const FString ModelName = CurrentValue();
	if (ModelName.IsEmpty())
		return FText::GetEmpty();

	const FString Locale = GetDefault<UTryllRuntimeSettings>()->Locale;
	if (Locale.IsEmpty())
		return FText::GetEmpty();

	const TArray<FString> Declared = UTryllModelManifest::GetActive()->LanguagesOf(ModelName);
	if (Declared.Num() == 0)
		return FText::GetEmpty();

	const FString Lang = UTryllLanguages::LanguageOf(Locale);
	for (const FString& Code : Declared)
		if (Code.Equals(Lang, ESearchCase::IgnoreCase))
			return FText::GetEmpty();

	const FText NameText = FText::FromString(ModelName);
	const FText DisplayText = FText::FromString(UTryllLanguages::LanguageDisplayName(Lang));
	const FText LangText = FText::FromString(Lang);
	if (ModelType == TEXT("tts"))
	{
		return FText::Format(LOCTEXT("TtsModelLangMismatch",
			"'{0}' does not support the project language {1} ({2}). Set this node's "
			"Tts Lang explicitly, pick another model, or change the language in "
			"Project Settings > Tryll Client. Agent creation fails otherwise."),
			NameText, DisplayText, LangText);
	}
	return FText::Format(LOCTEXT("SttModelLangMismatch",
		"'{0}' does not support the project language {1} ({2}). Pick another STT "
		"model or change the language in Project Settings > Tryll Client. Voice "
		"input creation fails otherwise."),
		NameText, DisplayText, LangText);
}

void FTryllModelNameCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& /*ChildBuilder*/,
	IPropertyTypeCustomizationUtils& /*CustomizationUtils*/)
{
	uint32 NumChildren = 0;
	StructPropertyHandle->GetNumChildren(NumChildren);
	for (uint32 Index = 0; Index < NumChildren; ++Index)
	{
		if (const TSharedPtr<IPropertyHandle> Child = StructPropertyHandle->GetChildHandle(Index))
			Child->MarkHiddenByCustomization();
	}
}

#undef LOCTEXT_NAMESPACE
