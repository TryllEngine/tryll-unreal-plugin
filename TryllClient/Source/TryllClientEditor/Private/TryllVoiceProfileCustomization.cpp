// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllVoiceProfileCustomization.h"

#include "TryllModelCatalogFile.h"
#include "TryllModelInfo.h"
#include "TryllNestedStructCustomization.h"
#include "TryllVoiceProfile.h"
#include "TryllWorkflowValidator.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "HAL/PlatformTime.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailPropertyRow.h"
#include "PropertyHandle.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "TryllVoiceProfileCustomization"

/**
 * Catalog capabilities of the profile's current model. Looked up at most once a
 * second (and on a model change): Slate re-evaluates the visibility lambdas every
 * frame, and the catalog lookup stats the catalog files.
 */
struct FTryllVoiceCapsCache
{
	FString Model;
	TOptional<FTryllTtsCapabilities> Caps;
	double LastCheck = -1.0;

	const FTryllTtsCapabilities* Get(const FString& InModel)
	{
		const double Now = FPlatformTime::Seconds();
		if (LastCheck < 0.0 || InModel != Model || Now - LastCheck > 1.0)
		{
			Model = InModel;
			LastCheck = Now;
			const FTryllTtsCapabilities* Found = FTryllModelCatalogFile::FindTtsCapabilities(InModel);
			Caps = Found ? TOptional<FTryllTtsCapabilities>(*Found) : TOptional<FTryllTtsCapabilities>();
		}
		return Caps.GetPtrOrNull();
	}
};

namespace
{
	const FLinearColor TryllVoiceUiErrorColor(1.f, 0.4f, 0.4f);

	FString TryllVoiceUiString(const TSharedPtr<IPropertyHandle>& Handle)
	{
		FString Value;
		if (Handle.IsValid())
		{
			Handle->GetValue(Value);
		}
		return Value;
	}

	float TryllVoiceUiFloat(const TSharedPtr<IPropertyHandle>& Handle, float Default)
	{
		float Value = Default;
		if (Handle.IsValid())
		{
			Handle->GetValue(Value);
		}
		return Value;
	}

	/** Shown when the model can use the field, the model is unknown, or the field already holds a value. */
	EVisibility TryllVoiceUiFieldVisibility(const FTryllTtsCapabilities* Caps, bool bSupported, bool bHasValue)
	{
		return (!Caps || bSupported || bHasValue) ? EVisibility::Visible : EVisibility::Collapsed;
	}

	UTryllVoiceProfile* TryllVoiceUiOuterProfile(const TSharedRef<IPropertyHandle>& Handle)
	{
		TArray<UObject*> Outers;
		Handle->GetOuterObjects(Outers);
		return Outers.Num() == 1 ? Cast<UTryllVoiceProfile>(Outers[0]) : nullptr;
	}

	FText TryllVoiceUiCapsSummary(const FString& Model, const FTryllTtsCapabilities* Caps)
	{
		if (Model.IsEmpty())
		{
			return LOCTEXT("NoModel", "Pick the TTS model this voice speaks with. It is required: there is no default.");
		}
		if (!Caps)
		{
			return FText::Format(LOCTEXT("UnknownModel",
				"'{0}' has no capabilities in the model catalog, so every field is shown. The server checks the profile when the agent is created."),
				FText::FromString(Model));
		}
		TArray<FString> Parts;
		const int32 Speakers = Caps->NumSpeakers > 0 ? Caps->NumSpeakers : Caps->VoicePresets.Num();
		if (Speakers > 1)                 Parts.Add(FString::Printf(TEXT("%d speakers"), Speakers));
		if (Caps->bReferenceVoice)        Parts.Add(TEXT("voice cloning (reference clips)"));
		if (Caps->bVoiceDescription)      Parts.Add(TEXT("voice description"));
		if (Caps->bDeliveryInstruction)   Parts.Add(TEXT("delivery instructions"));
		if (Caps->bExpressiveness)        Parts.Add(TEXT("expressiveness"));
		if (Caps->bSpeed)                 Parts.Add(TEXT("speed"));
		if (Caps->bLanguagePerCall)       Parts.Add(TEXT("per-call language"));
		const bool bModesCanChange = Caps->bReferenceVoice || Caps->bDeliveryInstruction || Caps->bExpressiveness || Caps->bSpeed;
		return FText::FromString(FString::Printf(TEXT("%s supports: %s.%s"),
			*Model,
			Parts.Num() > 0 ? *FString::Join(Parts, TEXT(", ")) : TEXT("its built-in voice only"),
			bModesCanChange ? TEXT("") : TEXT(" Delivery modes must stay empty on this model; define them anyway to keep game code portable.")));
	}

}

// ── FTryllVoiceProfileCustomization ─────────────────────────────────────────────

TSharedRef<IDetailCustomization> FTryllVoiceProfileCustomization::MakeInstance()
{
	return MakeShared<FTryllVoiceProfileCustomization>();
}

void FTryllVoiceProfileCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);
	if (Objects.Num() != 1 || !Objects[0].IsValid())
	{
		return;   // multi-select: the stock layout
	}
	const TWeakObjectPtr<UTryllVoiceProfile> WeakProfile = Cast<UTryllVoiceProfile>(Objects[0].Get());
	if (!WeakProfile.IsValid())
	{
		return;
	}

	const TSharedRef<IPropertyHandle> VoiceHandle =
		DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTryllVoiceProfile, Voice));
	DetailBuilder.HideProperty(VoiceHandle);
	const auto Child = [&VoiceHandle](FName Name) { return VoiceHandle->GetChildHandle(Name); };

	const TSharedPtr<IPropertyHandle> ModelHandle       = Child(GET_MEMBER_NAME_CHECKED(FTryllVoiceProfileData, TtsModelName));
	const TSharedPtr<IPropertyHandle> SpeakerHandle     = Child(GET_MEMBER_NAME_CHECKED(FTryllVoiceProfileData, SpeakerId));
	const TSharedPtr<IPropertyHandle> RefVoiceHandle    = Child(GET_MEMBER_NAME_CHECKED(FTryllVoiceProfileData, ReferenceVoice));
	const TSharedPtr<IPropertyHandle> DescriptionHandle = Child(GET_MEMBER_NAME_CHECKED(FTryllVoiceProfileData, VoiceDescription));
	const TSharedPtr<IPropertyHandle> SpeedHandle       = Child(GET_MEMBER_NAME_CHECKED(FTryllVoiceProfileData, Speed));
	const TSharedPtr<IPropertyHandle> LangHandle        = Child(GET_MEMBER_NAME_CHECKED(FTryllVoiceProfileData, Lang));
	const TSharedPtr<IPropertyHandle> ExprHandle        = Child(GET_MEMBER_NAME_CHECKED(FTryllVoiceProfileData, Expressiveness));
	const TSharedPtr<IPropertyHandle> PrepareHandle     = Child(GET_MEMBER_NAME_CHECKED(FTryllVoiceProfileData, Prepare));
	const TSharedPtr<IPropertyHandle> VisemesHandle     = Child(GET_MEMBER_NAME_CHECKED(FTryllVoiceProfileData, bEmitVisemes));
	const TSharedPtr<IPropertyHandle> ModesHandle       = Child(GET_MEMBER_NAME_CHECKED(FTryllVoiceProfileData, DeliveryModes));
	const TSharedPtr<IPropertyHandle> DefaultModeHandle = Child(GET_MEMBER_NAME_CHECKED(FTryllVoiceProfileData, DefaultDeliveryMode));
	if (!ModelHandle.IsValid() || !SpeakerHandle.IsValid() || !DefaultModeHandle.IsValid())
	{
		return;
	}

	const TSharedRef<FTryllVoiceCapsCache> Cache = MakeShared<FTryllVoiceCapsCache>();
	const auto CapsOf = [WeakProfile, Cache]() -> const FTryllTtsCapabilities*
	{
		const UTryllVoiceProfile* Profile = WeakProfile.Get();
		return Profile ? Cache->Get(Profile->Voice.TtsModelName.Name) : nullptr;
	};
	const auto ModelOf = [WeakProfile]() -> FString
	{
		const UTryllVoiceProfile* Profile = WeakProfile.Get();
		return Profile ? Profile->Voice.TtsModelName.Name : FString();
	};

	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(TEXT("Voice"));

	// ── Model ───────────────────────────────────────────────────────────────
	Category.AddProperty(ModelHandle);
	Category.AddCustomRow(LOCTEXT("CapsFilter", "Capabilities"))
		.WholeRowContent()
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFontItalic())
			.AutoWrapText(true)
			.Text_Lambda([CapsOf, ModelOf]() { return TryllVoiceUiCapsSummary(ModelOf(), CapsOf()); })
		];

	// ── Identity ───────────────────────────────────────────────────────────
	{
		IDetailPropertyRow& Row = Category.AddProperty(SpeakerHandle);
		Row.Visibility(TAttribute<EVisibility>::CreateLambda([CapsOf, WeakProfile]()
		{
			const FTryllTtsCapabilities* Caps = CapsOf();
			const UTryllVoiceProfile* Profile = WeakProfile.Get();
			const bool bMulti = Caps && (Caps->NumSpeakers > 1 || Caps->VoicePresets.Num() > 0);
			return TryllVoiceUiFieldVisibility(Caps, bMulti, Profile && Profile->Voice.SpeakerId != 0);
		}));
		// Presets (Orpheus, Maya1) or 0..num_speakers-1. The option array lives in
		// the shared menu so the combo's raw pointer stays valid after this returns.
		struct FSpeakerMenu
		{
			TArray<TSharedPtr<FString>> Labels;
			TArray<int32> Ids;
		};
		const TSharedRef<FSpeakerMenu> Menu = MakeShared<FSpeakerMenu>();
		const TSharedRef<TSharedPtr<SComboBox<TSharedPtr<FString>>>> Combo =
			MakeShared<TSharedPtr<SComboBox<TSharedPtr<FString>>>>();
		const auto Rebuild = [Menu, CapsOf, SpeakerHandle]()
		{
			int32 Current = 0;
			SpeakerHandle->GetValue(Current);
			Menu->Labels.Reset();
			Menu->Ids.Reset();
			const FTryllTtsCapabilities* Caps = CapsOf();
			const auto Add = [Menu](int32 Id, const FString& Label)
			{
				Menu->Labels.Add(MakeShared<FString>(Label));
				Menu->Ids.Add(Id);
			};
			if (Caps && Caps->VoicePresets.Num() > 0)
			{
				for (int32 Index = 0; Index < Caps->VoicePresets.Num(); ++Index)
				{
					const FTryllTtsVoicePreset& Preset = Caps->VoicePresets[Index];
					const FString Name = Preset.Label.IsEmpty() ? Preset.Id : Preset.Label;
					Add(Index, FString::Printf(TEXT("%d: %s"), Index, *Name));
				}
			}
			else if (Caps && Caps->NumSpeakers > 1)
			{
				for (int32 Index = 0; Index < Caps->NumSpeakers; ++Index)
				{
					Add(Index, FString::FromInt(Index));
				}
			}
			if (!Menu->Ids.Contains(Current))
			{
				Add(Current, FString::Printf(TEXT("%d: (out of range)"), Current));
			}
		};
		Rebuild();

		Row.CustomWidget()
			.NameContent()
			[
				SpeakerHandle->CreatePropertyNameWidget()
			]
			.ValueContent()
			.MinDesiredWidth(250.f)
			[
				SAssignNew(*Combo, SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&Menu->Labels)
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
				{
					return SNew(STextBlock)
						.Font(IDetailLayoutBuilder::GetDetailFont())
						.Text(FText::FromString(Item.IsValid() ? *Item : FString()));
				})
				.OnSelectionChanged_Lambda([Menu, SpeakerHandle](TSharedPtr<FString> Item, ESelectInfo::Type)
				{
					if (!Item.IsValid())
					{
						return;
					}
					const int32 Index = Menu->Labels.IndexOfByPredicate(
						[&](const TSharedPtr<FString>& Label) { return Label == Item; });
					if (Menu->Ids.IsValidIndex(Index))
					{
						SpeakerHandle->SetValue(Menu->Ids[Index]);
					}
				})
				.OnComboBoxOpening_Lambda([Rebuild, Combo]()
				{
					Rebuild();
					if (Combo->IsValid())
					{
						(*Combo)->RefreshOptions();
					}
				})
				[
					SNew(STextBlock)
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.Text_Lambda([Menu, SpeakerHandle]()
					{
						int32 Current = 0;
						SpeakerHandle->GetValue(Current);
						const int32 Index = Menu->Ids.IndexOfByKey(Current);
						return FText::FromString(
							Menu->Labels.IsValidIndex(Index) && Menu->Labels[Index].IsValid()
								? *Menu->Labels[Index]
								: FString::FromInt(Current));
					})
				]
			];
	}

	Category.AddProperty(RefVoiceHandle).Visibility(TAttribute<EVisibility>::CreateLambda([CapsOf, WeakProfile]()
	{
		const FTryllTtsCapabilities* Caps = CapsOf();
		const UTryllVoiceProfile* Profile = WeakProfile.Get();
		return TryllVoiceUiFieldVisibility(Caps, Caps && Caps->bReferenceVoice,
			Profile && !Profile->Voice.ReferenceVoice.Path.IsEmpty());
	}));
	Category.AddProperty(DescriptionHandle).Visibility(TAttribute<EVisibility>::CreateLambda([CapsOf, WeakProfile]()
	{
		const FTryllTtsCapabilities* Caps = CapsOf();
		const UTryllVoiceProfile* Profile = WeakProfile.Get();
		return TryllVoiceUiFieldVisibility(Caps, Caps && Caps->bVoiceDescription,
			Profile && !Profile->Voice.VoiceDescription.IsEmpty());
	}));
	Category.AddProperty(SpeedHandle).Visibility(TAttribute<EVisibility>::CreateLambda([CapsOf, WeakProfile]()
	{
		const FTryllTtsCapabilities* Caps = CapsOf();
		const UTryllVoiceProfile* Profile = WeakProfile.Get();
		return TryllVoiceUiFieldVisibility(Caps, Caps && Caps->bSpeed,
			Profile && !FMath::IsNearlyEqual(Profile->Voice.Speed, 1.f));
	}));
	Category.AddProperty(LangHandle).Visibility(TAttribute<EVisibility>::CreateLambda([CapsOf, WeakProfile]()
	{
		const FTryllTtsCapabilities* Caps = CapsOf();
		const UTryllVoiceProfile* Profile = WeakProfile.Get();
		return TryllVoiceUiFieldVisibility(Caps, Caps && Caps->bLanguagePerCall,
			Profile && !Profile->Voice.Lang.IsEmpty());
	}));
	Category.AddProperty(ExprHandle).Visibility(TAttribute<EVisibility>::CreateLambda([CapsOf, WeakProfile]()
	{
		const FTryllTtsCapabilities* Caps = CapsOf();
		const UTryllVoiceProfile* Profile = WeakProfile.Get();
		return TryllVoiceUiFieldVisibility(Caps, Caps && Caps->bExpressiveness,
			Profile && !FMath::IsNearlyEqual(Profile->Voice.Expressiveness, 0.5f));
	}));

	// ── Pipeline and delivery ─────────────────────────────────────────────
	Category.AddProperty(VisemesHandle);
	Category.AddProperty(ModesHandle);

	Category.AddProperty(DefaultModeHandle).CustomWidget()
		.NameContent()
		[
			DefaultModeHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(200.f)
		[
			SNew(SComboButton)
			.OnGetMenuContent_Lambda([WeakProfile, DefaultModeHandle]()
			{
				FMenuBuilder Menu(/*bInShouldCloseWindowAfterMenuSelection*/ true, nullptr);
				Menu.AddMenuEntry(
					LOCTEXT("NeutralEntry", "(neutral)"),
					LOCTEXT("NeutralTip", "The built-in mode: the base voice with no delivery applied."),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([DefaultModeHandle]() { DefaultModeHandle->SetValue(FString()); })));
				if (const UTryllVoiceProfile* Profile = WeakProfile.Get())
				{
					for (const FString& Name : Profile->Voice.GetModeNames())
					{
						if (!FTryllVoiceProfileData::IsValidModeName(Name))
						{
							continue;
						}
						Menu.AddMenuEntry(
							FText::FromString(Name),
							FText::GetEmpty(),
							FSlateIcon(),
							FUIAction(FExecuteAction::CreateLambda([DefaultModeHandle, Name]() { DefaultModeHandle->SetValue(Name); })));
					}
				}
				return Menu.MakeWidget();
			})
			.ButtonContent()
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text_Lambda([DefaultModeHandle]()
				{
					const FString Value = TryllVoiceUiString(DefaultModeHandle);
					return Value.IsEmpty() ? LOCTEXT("NeutralValue", "(neutral)") : FText::FromString(Value);
				})
			]
		];

	Category.AddProperty(PrepareHandle, EPropertyLocation::Advanced);

	// ── Issues ─────────────────────────────────────────────────────────────
	const auto IssuesText = [WeakProfile, CapsOf]() -> FString
	{
		const UTryllVoiceProfile* Profile = WeakProfile.Get();
		if (!Profile)
		{
			return FString();
		}
		TArray<FTryllWorkflowIssue> Issues;
		Profile->Voice.Validate(TEXT("Voice"), Issues);
		if (const FTryllTtsCapabilities* Caps = CapsOf())
		{
			Profile->Voice.ValidateAgainst(*Caps, TEXT("Voice"), Issues);
		}
		TArray<FString> Lines;
		for (const FTryllWorkflowIssue& Issue : Issues)
		{
			Lines.Add(Issue.Severity == FTryllWorkflowIssue::ESeverity::Error
				? FString::Printf(TEXT("Error: %s"), *Issue.Message)
				: FString::Printf(TEXT("Warning: %s"), *Issue.Message));
		}
		return FString::Join(Lines, TEXT("\n"));
	};
	Category.AddCustomRow(LOCTEXT("IssuesFilter", "Issues"))
		.Visibility(TAttribute<EVisibility>::CreateLambda([IssuesText]()
		{
			return IssuesText().IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
		}))
		.WholeRowContent()
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.ColorAndOpacity(FSlateColor(TryllVoiceUiErrorColor))
			.AutoWrapText(true)
			.Text_Lambda([IssuesText]() { return FText::FromString(IssuesText()); })
		];
}

// ── FTryllDeliveryModeCustomization ─────────────────────────────────────────────

TSharedRef<IPropertyTypeCustomization> FTryllDeliveryModeCustomization::MakeInstance()
{
	return MakeShared<FTryllDeliveryModeCustomization>();
}

void FTryllDeliveryModeCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& /*CustomizationUtils*/)
{
	const TSharedPtr<IPropertyHandle> NameHandle =
		StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTryllDeliveryMode, Name));

	HeaderRow
		.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(250.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text_Lambda([NameHandle]()
				{
					const FString Name = TryllVoiceUiString(NameHandle);
					return Name.IsEmpty() ? LOCTEXT("UnnamedMode", "(unnamed mode)") : FText::FromString(Name);
				})
				.ColorAndOpacity_Lambda([NameHandle]()
				{
					return FTryllVoiceProfileData::IsValidModeName(TryllVoiceUiString(NameHandle))
						? FSlateColor::UseForeground()
						: FSlateColor(TryllVoiceUiErrorColor);
				})
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				StructPropertyHandle->CreateDefaultPropertyButtonWidgets()
			]
		];
}

void FTryllDeliveryModeCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& ChildBuilder,
	IPropertyTypeCustomizationUtils& /*CustomizationUtils*/)
{
	const auto Child = [&StructPropertyHandle](FName Name) { return StructPropertyHandle->GetChildHandle(Name); };
	const TSharedPtr<IPropertyHandle> NameHandle   = Child(GET_MEMBER_NAME_CHECKED(FTryllDeliveryMode, Name));
	const TSharedPtr<IPropertyHandle> ClipHandle   = Child(GET_MEMBER_NAME_CHECKED(FTryllDeliveryMode, ReferenceVoice));
	const TSharedPtr<IPropertyHandle> InstrHandle  = Child(GET_MEMBER_NAME_CHECKED(FTryllDeliveryMode, Instruction));
	const TSharedPtr<IPropertyHandle> ExprHandle   = Child(GET_MEMBER_NAME_CHECKED(FTryllDeliveryMode, Expressiveness));
	const TSharedPtr<IPropertyHandle> ScaleHandle  = Child(GET_MEMBER_NAME_CHECKED(FTryllDeliveryMode, SpeedScale));
	const TSharedPtr<IPropertyHandle> ClipPathHandle = ClipHandle.IsValid()
		? ClipHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTryllStoragePath, Path))
		: TSharedPtr<IPropertyHandle>();

	const TWeakObjectPtr<UTryllVoiceProfile> WeakProfile = TryllVoiceUiOuterProfile(StructPropertyHandle);
	const TSharedRef<FTryllVoiceCapsCache> Cache = MakeShared<FTryllVoiceCapsCache>();
	const auto CapsOf = [WeakProfile, Cache]() -> const FTryllTtsCapabilities*
	{
		// Not inside a voice profile (e.g. a struct in some other asset): show everything.
		const UTryllVoiceProfile* Profile = WeakProfile.Get();
		return Profile ? Cache->Get(Profile->Voice.TtsModelName.Name) : nullptr;
	};

	if (NameHandle.IsValid())
	{
		ChildBuilder.AddProperty(NameHandle.ToSharedRef());
	}
	if (ClipHandle.IsValid())
	{
		ChildBuilder.AddProperty(ClipHandle.ToSharedRef()).Visibility(TAttribute<EVisibility>::CreateLambda([CapsOf, ClipPathHandle]()
		{
			const FTryllTtsCapabilities* Caps = CapsOf();
			return TryllVoiceUiFieldVisibility(Caps, Caps && Caps->bReferenceVoice, !TryllVoiceUiString(ClipPathHandle).IsEmpty());
		}));
	}
	if (IDetailPropertyRow* InstrRow = TryllMultilineProperty::Add(ChildBuilder, InstrHandle))
	{
		InstrRow->Visibility(TAttribute<EVisibility>::CreateLambda([CapsOf, InstrHandle]()
		{
			const FTryllTtsCapabilities* Caps = CapsOf();
			return TryllVoiceUiFieldVisibility(Caps, Caps && Caps->bDeliveryInstruction, !TryllVoiceUiString(InstrHandle).IsEmpty());
		}));
	}
	if (ExprHandle.IsValid())
	{
		ChildBuilder.AddProperty(ExprHandle.ToSharedRef()).Visibility(TAttribute<EVisibility>::CreateLambda([CapsOf, ExprHandle]()
		{
			const FTryllTtsCapabilities* Caps = CapsOf();
			return TryllVoiceUiFieldVisibility(Caps, Caps && Caps->bExpressiveness, TryllVoiceUiFloat(ExprHandle, -1.f) >= 0.f);
		}));
	}
	if (ScaleHandle.IsValid())
	{
		ChildBuilder.AddProperty(ScaleHandle.ToSharedRef()).Visibility(TAttribute<EVisibility>::CreateLambda([CapsOf, ScaleHandle]()
		{
			const FTryllTtsCapabilities* Caps = CapsOf();
			return TryllVoiceUiFieldVisibility(Caps, Caps && Caps->bSpeed, !FMath::IsNearlyEqual(TryllVoiceUiFloat(ScaleHandle, 1.f), 1.f));
		}));
	}
}

#undef LOCTEXT_NAMESPACE
