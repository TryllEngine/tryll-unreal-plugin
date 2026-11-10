// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllAgentComponentCustomization.h"

#include "TryllAgentComponent.h"
#include "TryllAgentVariables.h"
#include "TryllVariableOverrideUtils.h"
#include "TryllWorkflowAsset.h"
#include "TryllClientModule.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailGroup.h"
#include "IPropertyUtilities.h"
#include "PropertyHandle.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SNullWidget.h"

#define LOCTEXT_NAMESPACE "TryllAgentComponentCustomization"

namespace
{
	FString FormatDeclPreview(const FTryllVariableDecl& D)
	{
		switch (D.Type)
		{
		case ETryllVariableType::Int:       return FString::Printf(TEXT("%lld"), D.IntValue);
		case ETryllVariableType::Float:     return FString::Printf(TEXT("%g"), D.FloatValue);
		case ETryllVariableType::String:    return FString::Printf(TEXT("\"%s\""), *D.StringValue);
		case ETryllVariableType::Bool:      return D.bBoolValue ? TEXT("true") : TEXT("false");
		case ETryllVariableType::StringSet:
			return D.StringSetValue.Num() == 0
				? FString(TEXT("(empty set)"))
				: FString::Join(D.StringSetValue, TEXT(", "));
		default: return TEXT("?");
		}
	}

	TSharedPtr<IPropertyHandle> ActiveValueField(
		const TSharedPtr<IPropertyHandle>& ValueHandle, ETryllVariableType Type)
	{
		if (!ValueHandle.IsValid())
		{
			return nullptr;
		}
		switch (Type)
		{
		case ETryllVariableType::Int:
			return ValueHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTryllVariableDecl, IntValue));
		case ETryllVariableType::Float:
			return ValueHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTryllVariableDecl, FloatValue));
		case ETryllVariableType::String:
			return ValueHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTryllVariableDecl, StringValue));
		case ETryllVariableType::Bool:
			return ValueHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTryllVariableDecl, bBoolValue));
		case ETryllVariableType::StringSet:
			return ValueHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTryllVariableDecl, StringSetValue));
		default:
			return nullptr;
		}
	}

	int32 FindOverrideIndex(const TArray<FTryllVariableOverride>& Overrides, const FString& Name)
	{
		return TryllVariableOverrideUtils::FindFirstOverrideIndex(Overrides, Name);
	}
}

TSharedRef<IDetailCustomization> FTryllAgentComponentCustomization::MakeInstance()
{
	return MakeShared<FTryllAgentComponentCustomization>();
}

void FTryllAgentComponentCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	PropertyUtils = DetailBuilder.GetPropertyUtilities();
	OverridesHandle = DetailBuilder.GetProperty(
		GET_MEMBER_NAME_CHECKED(UTryllAgentComponent, VariableOverrides));
	if (!LiveUI.IsValid())
	{
		LiveUI = MakeShared<FTryllLiveVariableUIState>();
	}

	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);
	if (Objects.Num() != 1 || !Objects[0].IsValid())
	{
		// Multi-object: keep the raw array rather than guessing across declarations.
		return;
	}

	UTryllAgentComponent* Component = Cast<UTryllAgentComponent>(Objects[0].Get());
	if (!Component)
	{
		return;
	}

	DetailBuilder.HideProperty(OverridesHandle);

	const TSharedRef<IPropertyHandle> WorkflowHandle = DetailBuilder.GetProperty(
		GET_MEMBER_NAME_CHECKED(UTryllAgentComponent, WorkflowAsset));
	const TSharedRef<IPropertyHandle> AutoCreateHandle = DetailBuilder.GetProperty(
		GET_MEMBER_NAME_CHECKED(UTryllAgentComponent, bAutoCreateOnConnect));
	const TSharedRef<IPropertyHandle> DiagnosticsHandle = DetailBuilder.GetProperty(
		GET_MEMBER_NAME_CHECKED(UTryllAgentComponent, bEnableDiagnostics));
	const TSharedRef<IPropertyHandle> HistoryHandle = DetailBuilder.GetProperty(
		GET_MEMBER_NAME_CHECKED(UTryllAgentComponent, bMaintainDialogueHistory));
	const TSharedRef<IPropertyHandle> SpeakerHandle = DetailBuilder.GetProperty(
		GET_MEMBER_NAME_CHECKED(UTryllAgentComponent, Speaker));
	const TSharedRef<IPropertyHandle> VoiceProfileHandle = DetailBuilder.GetProperty(
		GET_MEMBER_NAME_CHECKED(UTryllAgentComponent, VoiceProfile));
	const TSharedRef<IPropertyHandle> OverrideVoiceDeliveryHandle = DetailBuilder.GetProperty(
		GET_MEMBER_NAME_CHECKED(UTryllAgentComponent, bOverrideVoiceDelivery));
	const TSharedRef<IPropertyHandle> VoiceDeliveryHandle = DetailBuilder.GetProperty(
		GET_MEMBER_NAME_CHECKED(UTryllAgentComponent, VoiceDelivery));
	const TSharedRef<IPropertyHandle> WorkloadHandle = DetailBuilder.GetProperty(
		GET_MEMBER_NAME_CHECKED(UTryllAgentComponent, Workload));
	const TSharedRef<IPropertyHandle> KvCacheInitHandle = DetailBuilder.GetProperty(
		GET_MEMBER_NAME_CHECKED(UTryllAgentComponent, KvCacheInitialization));

	// EditCategory does not honor "Parent|Child" nesting — that only works for
	// UPROPERTY Category. Custom rows must go in an AddGroup under "Tryll".
	// Customized groups always sort above default UPROPERTY subcategories, so
	// rebuild Agent/TTS/Variables/Live as groups in the desired display order.
	//
	// Unlike Unity's custom Inspector, a property left un-enumerated here is
	// still shown — it just lands in its own auto-generated category outside the
	// curated layout, which is how Workload ("Tryll|Agent") and
	// KvCacheInitialization ("Tryll|KV Cache") used to surface. Hiding and
	// re-adding them puts them in the curated order.
	DetailBuilder.HideProperty(WorkflowHandle);
	DetailBuilder.HideProperty(AutoCreateHandle);
	DetailBuilder.HideProperty(DiagnosticsHandle);
	DetailBuilder.HideProperty(HistoryHandle);
	DetailBuilder.HideProperty(SpeakerHandle);
	DetailBuilder.HideProperty(VoiceProfileHandle);
	DetailBuilder.HideProperty(OverrideVoiceDeliveryHandle);
	DetailBuilder.HideProperty(VoiceDeliveryHandle);
	DetailBuilder.HideProperty(WorkloadHandle);
	DetailBuilder.HideProperty(KvCacheInitHandle);
	DetailBuilder.HideCategory(TEXT("Variables"));
	DetailBuilder.HideCategory(TEXT("Tryll|Variables"));
	DetailBuilder.HideCategory(TEXT("Tryll|KV Cache"));

	IDetailCategoryBuilder& TryllCategory = DetailBuilder.EditCategory(TEXT("Tryll"));

	IDetailGroup& AgentGroup = TryllCategory.AddGroup(
		TEXT("Agent"),
		LOCTEXT("AgentCategory", "Agent"),
		/*bForAdvanced*/ false,
		/*bStartExpanded*/ true);
	AgentGroup.AddPropertyRow(WorkflowHandle);
	if (!Component->WorkflowAsset)
	{
		AgentGroup.AddWidgetRow()
			.WholeRowContent()
			[
				SNew(STextBlock)
				.Text(FText::Format(LOCTEXT("MissingWorkflowAsset",
					"Assign a Workflow Asset. The component cannot create an agent without one. {0}"),
					FText::FromString(UTryllAgentComponent::MissingWorkflowAssetHint)))
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.4f, 0.4f)))
				.AutoWrapText(true)
			];
	}
	AgentGroup.AddPropertyRow(AutoCreateHandle);
	// bEnableDiagnostics authoring control retired (Package 2): Chat and Agent Log
	// request diagnostics via a ref-counted lease / transient create override.
	// Property stays hidden (above); runtime field + wire default unchanged.
	AgentGroup.AddPropertyRow(HistoryHandle);

	IDetailGroup& SchedulingGroup = TryllCategory.AddGroup(
		TEXT("Scheduling"),
		LOCTEXT("SchedulingCategory", "Scheduling"),
		/*bForAdvanced*/ false,
		/*bStartExpanded*/ true);
	SchedulingGroup.AddPropertyRow(WorkloadHandle);
	SchedulingGroup.AddPropertyRow(KvCacheInitHandle);

	IDetailGroup& TTSGroup = TryllCategory.AddGroup(
		TEXT("TTS"),
		LOCTEXT("TTSCategory", "TTS"),
		/*bForAdvanced*/ false,
		/*bStartExpanded*/ true);
	TTSGroup.AddPropertyRow(SpeakerHandle);
	TTSGroup.AddPropertyRow(VoiceProfileHandle);
	{
		struct FDeliveryMenu
		{
			TArray<TSharedPtr<FString>> Labels;
			TArray<FString> Values;
		};
		const TWeakObjectPtr<UTryllAgentComponent> WeakComponent(Component);
		const TSharedRef<FDeliveryMenu> Menu = MakeShared<FDeliveryMenu>();
		const TSharedRef<TSharedPtr<SComboBox<TSharedPtr<FString>>>> Combo =
			MakeShared<TSharedPtr<SComboBox<TSharedPtr<FString>>>>();
		const auto Normalize = [](const FString& Mode)
		{
			return Mode.Equals(FTryllVoiceProfileData::NeutralMode) ? FString() : Mode;
		};
		const auto EffectiveMode = [WeakComponent, Normalize]()
		{
			const UTryllAgentComponent* C = WeakComponent.Get();
			if (!C)
			{
				return FString();
			}
			if (C->HasLiveAgent())
			{
				return Normalize(C->GetDeliveryMode());
			}
			if (C->bOverrideVoiceDelivery)
			{
				return Normalize(C->VoiceDelivery);
			}
			return C->VoiceProfile
				? Normalize(C->VoiceProfile->Voice.DefaultDeliveryMode)
				: FString();
		};
		const auto Rebuild = [Menu, WeakComponent, EffectiveMode, Normalize]()
		{
			Menu->Labels.Reset();
			Menu->Values.Reset();
			const auto Add = [Menu](const FString& Value, const FString& Label)
			{
				if (!Menu->Values.Contains(Value))
				{
					Menu->Values.Add(Value);
					Menu->Labels.Add(MakeShared<FString>(Label));
				}
			};
			Add(FString(), TEXT("neutral (built in)"));
			const UTryllAgentComponent* C = WeakComponent.Get();
			if (C && C->VoiceProfile)
			{
				for (const FTryllDeliveryMode& Mode : C->VoiceProfile->Voice.DeliveryModes)
				{
					if (!Mode.Name.IsEmpty())
					{
						Add(Normalize(Mode.Name), Mode.Name);
					}
				}
			}
			const FString Current = EffectiveMode();
			if (!Menu->Values.Contains(Current))
			{
				Add(Current, Current + TEXT(" (not in Voice Profile)"));
			}
		};
		Rebuild();

		TTSGroup.AddWidgetRow()
			.FilterString(LOCTEXT("VoiceDeliveryFilter", "Voice Delivery"))
			.NameContent()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([WeakComponent]()
					{
						const UTryllAgentComponent* C = WeakComponent.Get();
						return C && C->bOverrideVoiceDelivery
							? ECheckBoxState::Checked
							: ECheckBoxState::Unchecked;
					})
					.OnCheckStateChanged_Lambda(
						[WeakComponent, OverrideVoiceDeliveryHandle, VoiceDeliveryHandle,
						 EffectiveMode, Normalize, PropertyUtils = PropertyUtils](ECheckBoxState State)
						{
							UTryllAgentComponent* C = WeakComponent.Get();
							if (!C)
							{
								return;
							}
							const bool bEnabled = State == ECheckBoxState::Checked;
							const FString Current = EffectiveMode();
							if (bEnabled)
							{
								VoiceDeliveryHandle->SetValue(Current);
							}
							OverrideVoiceDeliveryHandle->SetValue(bEnabled);
							const FString Desired = bEnabled
								? Current
								: (C->VoiceProfile
									? Normalize(C->VoiceProfile->Voice.DefaultDeliveryMode)
									: FString());
							if (C->HasLiveAgent())
							{
								C->SetDeliveryMode(Desired);
							}
							if (PropertyUtils.IsValid())
							{
								PropertyUtils->ForceRefresh();
							}
						})
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("VoiceDeliveryLabel", "Voice Delivery"))
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.ToolTipText(LOCTEXT("VoiceDeliveryTooltip",
						"Override the Voice Profile's default delivery for this agent instance. "
						"Changing it while an agent is live applies immediately."))
				]
			]
			.ValueContent()
			.MinDesiredWidth(250.f)
			[
				SAssignNew(*Combo, SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&Menu->Labels)
				.IsEnabled_Lambda([WeakComponent]()
				{
					const UTryllAgentComponent* C = WeakComponent.Get();
					return C && C->bOverrideVoiceDelivery && C->VoiceProfile;
				})
				.OnComboBoxOpening_Lambda([Combo, Rebuild]()
				{
					Rebuild();
					if (Combo->IsValid())
					{
						(*Combo)->RefreshOptions();
					}
				})
				.OnGenerateWidget_Lambda([](const TSharedPtr<FString>& Item)
				{
					return SNew(STextBlock)
						.Font(IDetailLayoutBuilder::GetDetailFont())
						.Text(FText::FromString(Item.IsValid() ? *Item : FString()));
				})
				.OnSelectionChanged_Lambda(
					[Menu, WeakComponent, VoiceDeliveryHandle](const TSharedPtr<FString>& Item, ESelectInfo::Type)
					{
						const int32 Index = Menu->Labels.IndexOfByKey(Item);
						if (!Menu->Values.IsValidIndex(Index))
						{
							return;
						}
						const FString Mode = Menu->Values[Index];
						VoiceDeliveryHandle->SetValue(Mode);
						if (UTryllAgentComponent* C = WeakComponent.Get(); C && C->HasLiveAgent())
						{
							C->SetDeliveryMode(Mode);
						}
					})
				[
					SNew(STextBlock)
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.Text_Lambda([EffectiveMode]()
					{
						const FString Mode = EffectiveMode();
						return FText::FromString(Mode.IsEmpty()
							? FString(TEXT("neutral (built in)"))
							: Mode);
					})
				]
			];
	}
	{
		// Mirrors the server's AgentVoiceRequired (3025): a workflow that speaks needs a voice.
		const TWeakObjectPtr<UTryllAgentComponent> WeakComponent(Component);
		TTSGroup.AddWidgetRow()
			.Visibility(TAttribute<EVisibility>::CreateLambda([WeakComponent]()
			{
				const UTryllAgentComponent* C = WeakComponent.Get();
				const bool bMissing = C && C->WorkflowAsset && !C->VoiceProfile && C->WorkflowAsset->Graph.UsesTts();
				return bMissing ? EVisibility::Visible : EVisibility::Collapsed;
			}))
			.WholeRowContent()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("MissingVoiceProfile",
					"The workflow speaks (Speak / Generate And Speak) but no Voice Profile is assigned, so the server will "
					"refuse to create the agent (AgentVoiceRequired, 3025). Content Browser ▶ Tryll ▶ Voice Profile, then assign it above."))
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.4f, 0.4f)))
				.AutoWrapText(true)
			];
	}

	IDetailGroup& VariablesGroup = TryllCategory.AddGroup(
		TEXT("Variables"),
		LOCTEXT("VariablesCategory", "Variables"),
		/*bForAdvanced*/ false,
		/*bStartExpanded*/ true);

	if (PropertyUtils.IsValid())
	{
		WorkflowHandle->SetOnPropertyValueChanged(
			FSimpleDelegate::CreateSP(PropertyUtils.ToSharedRef(), &IPropertyUtilities::ForceRefresh));
	}

	BuildOverrideRows(VariablesGroup, Component);
	BuildStaleGroup(VariablesGroup, Component);
	// Always build so preview attach / PIE readiness updates without requiring
	// a Details rebuild (widgets poll HasLiveAgent / IsDirty dynamically).
	BuildLiveAgentGroup(TryllCategory, Component);
}

void FTryllAgentComponentCustomization::BuildOverrideRows(
	IDetailGroup& VariablesGroup, UTryllAgentComponent* Component)
{
	VariablesGroup.AddWidgetRow()
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("OverridesLabel", "Variable Overrides"))
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.ToolTipText(LOCTEXT("OverridesTooltip",
				"Check a variable to override its initial value for this agent instance. "
				"While an agent is live (PIE or Tryll Chat), edits update the local mirror and "
				"flush before the next Send/Resume/ChangeParams (or use Flush Now)."))
		]
		.ValueContent()
		[
			SNullWidget::NullWidget
		];

	const TArray<FTryllVariableDecl> Declared = TryllVariableOverrideUtils::ResolveDeclared(Component);
	if (Declared.Num() == 0)
	{
		VariablesGroup.AddWidgetRow()
			.WholeRowContent()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("NoDeclsText",
					"No variables declared. Add them on the Workflow Asset."))
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.AutoWrapText(true)
			];
		return;
	}

	for (const FTryllVariableDecl& Decl : Declared)
	{
		if (Decl.Name.IsEmpty())
		{
			continue;
		}

		const FString DeclName = Decl.Name;
		const ETryllVariableType DeclType = Decl.Type;
		const FTryllVariableDecl DeclCopy = Decl;

		const int32 Idx = FindOverrideIndex(Component->VariableOverrides, DeclName);
		const bool bHasEntry = Idx != INDEX_NONE;
		// Never mutate serialized overrides while building Details (no Modify /
		// transaction). Type-drifted entries stay inert until the author toggles
		// the override — EnsureEntry reseeds inside FScopedTransaction.
		const bool bTypeDrift = bHasEntry
			&& Component->VariableOverrides[Idx].Value.Type != DeclType;
		const bool bEnabled = bHasEntry
			&& Component->VariableOverrides[Idx].bEnabled
			&& !bTypeDrift;

		TSharedPtr<IPropertyHandle> ValueFieldHandle;
		if (bEnabled && OverridesHandle.IsValid())
		{
			if (TSharedPtr<IPropertyHandleArray> Arr = OverridesHandle->AsArray())
			{
				TSharedRef<IPropertyHandle> Element = Arr->GetElement(Idx);
				TSharedPtr<IPropertyHandle> ValueHandle = Element->GetChildHandle(
					GET_MEMBER_NAME_CHECKED(FTryllVariableOverride, Value));
				if (ValueHandle.IsValid())
				{
					ValueFieldHandle = ActiveValueField(ValueHandle, DeclType);
				}
			}
		}

		TSharedRef<SWidget> ValueWidget = SNullWidget::NullWidget;
		if (bEnabled && ValueFieldHandle.IsValid())
		{
			auto OnValueChanged = FSimpleDelegate::CreateLambda(
				[this, WeakComp = TWeakObjectPtr<UTryllAgentComponent>(Component), DeclName]()
				{
					if (UTryllAgentComponent* Comp = WeakComp.Get())
					{
						ApplyRowLive(Comp, DeclName, true);
					}
				});
			ValueFieldHandle->SetOnPropertyValueChanged(OnValueChanged);
			ValueFieldHandle->SetOnChildPropertyValueChanged(OnValueChanged);
			ValueWidget = ValueFieldHandle->CreatePropertyValueWidget(false);
		}
		else
		{
			const FString PreviewText = bTypeDrift
				? FString::Printf(TEXT("%s (type stale — toggle override to re-seed)"),
					*FormatDeclPreview(DeclCopy))
				: FormatDeclPreview(DeclCopy);
			ValueWidget = SNew(STextBlock)
				.Text(FText::FromString(PreviewText))
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(bTypeDrift
					? FSlateColor(FLinearColor(1.f, 0.7f, 0.2f))
					: FSlateColor::UseSubduedForeground());
		}

		VariablesGroup.AddWidgetRow()
			.NameContent()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 6, 0)
				[
					SNew(SBox)
					.WidthOverride(18.f)
					[
						SNew(SCheckBox)
						.IsChecked_Lambda([WeakComp = TWeakObjectPtr<UTryllAgentComponent>(Component), DeclName]()
						{
							UTryllAgentComponent* Comp = WeakComp.Get();
							if (!Comp) return ECheckBoxState::Unchecked;
							const int32 I = FindOverrideIndex(Comp->VariableOverrides, DeclName);
							return (I != INDEX_NONE && Comp->VariableOverrides[I].bEnabled)
								? ECheckBoxState::Checked
								: ECheckBoxState::Unchecked;
						})
						.OnCheckStateChanged_Lambda(
							[this, WeakComp = TWeakObjectPtr<UTryllAgentComponent>(Component), DeclName, DeclCopy](
								ECheckBoxState NewState)
							{
								UTryllAgentComponent* Comp = WeakComp.Get();
								if (!Comp) return;

								const bool bNowEnabled = (NewState == ECheckBoxState::Checked);
								{
									FScopedTransaction Tx(LOCTEXT("ToggleOverride", "Toggle Variable Override"));
									Comp->Modify();
									FTryllVariableOverride* Entry =
										TryllVariableOverrideUtils::EnsureEntry(Comp->VariableOverrides, DeclCopy);
									if (!Entry) return;
									Entry->bEnabled = bNowEnabled;
									Comp->MarkPackageDirty();
								}
								ApplyRowLive(Comp, DeclName, bNowEnabled);
								if (PropertyUtils.IsValid())
								{
									PropertyUtils->ForceRefresh();
								}
							})
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(DeclName))
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.ToolTipText(FText::FromString(
						FString::Printf(TEXT("Type: %s\nAllow Output Substitution: %s"),
							DeclType == ETryllVariableType::Int ? TEXT("Int") :
							DeclType == ETryllVariableType::Float ? TEXT("Float") :
							DeclType == ETryllVariableType::String ? TEXT("String") :
							DeclType == ETryllVariableType::Bool ? TEXT("Bool") : TEXT("StringSet"),
							Decl.bAllowOutputSubstitution ? TEXT("Yes") : TEXT("No"))))
				]
			]
			.ValueContent()
			.MinDesiredWidth(200.f)
			[
				ValueWidget
			];
	}
}

void FTryllAgentComponentCustomization::BuildStaleGroup(
	IDetailGroup& VariablesGroup, UTryllAgentComponent* Component)
{
	const TArray<FTryllVariableDecl> Declared = TryllVariableOverrideUtils::ResolveDeclared(Component);
	const TArray<int32> Legacy = TryllVariableOverrideUtils::CollectLegacyIndices(
		Component->VariableOverrides, Declared);
	if (Legacy.Num() == 0)
	{
		return;
	}

	VariablesGroup.AddWidgetRow()
		.WholeRowContent()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("StaleExplain",
					"Stale or duplicate overrides (kept for recovery; ignored at CreateAgent):"))
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.AutoWrapText(true)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("RemoveAllStale", "Remove All Stale"))
					.OnClicked_Lambda(
						[this, WeakComp = TWeakObjectPtr<UTryllAgentComponent>(Component)]()
						{
							if (UTryllAgentComponent* Comp = WeakComp.Get())
							{
								FScopedTransaction Tx(LOCTEXT("RemoveAllStaleTx", "Remove Stale Variable Overrides"));
								Comp->Modify();
								const TArray<FTryllVariableDecl> D =
									TryllVariableOverrideUtils::ResolveDeclared(Comp);
								TryllVariableOverrideUtils::RemoveLegacyEntries(Comp->VariableOverrides, D);
								Comp->MarkPackageDirty();
								if (PropertyUtils.IsValid()) PropertyUtils->ForceRefresh();
							}
							return FReply::Handled();
						})
				]
			]
		];

	for (int32 LegacyIdx : Legacy)
	{
		if (!Component->VariableOverrides.IsValidIndex(LegacyIdx))
		{
			continue;
		}
		const FString Name = Component->VariableOverrides[LegacyIdx].Name.IsEmpty()
			? FString(TEXT("(empty name)"))
			: Component->VariableOverrides[LegacyIdx].Name;
		const int32 CapturedIdx = LegacyIdx;

		VariablesGroup.AddWidgetRow()
			.NameContent()
			[
				SNew(STextBlock)
				.Text(FText::FromString(Name))
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FLinearColor(1.f, 0.7f, 0.2f))
			]
			.ValueContent()
			[
				SNew(SButton)
				.Text(LOCTEXT("RemoveStale", "Remove"))
				.OnClicked_Lambda(
					[this, WeakComp = TWeakObjectPtr<UTryllAgentComponent>(Component), CapturedIdx]()
					{
						if (UTryllAgentComponent* Comp = WeakComp.Get())
						{
							if (Comp->VariableOverrides.IsValidIndex(CapturedIdx))
							{
								FScopedTransaction Tx(LOCTEXT("RemoveStaleTx", "Remove Stale Variable Override"));
								Comp->Modify();
								Comp->VariableOverrides.RemoveAt(CapturedIdx);
								Comp->MarkPackageDirty();
								if (PropertyUtils.IsValid()) PropertyUtils->ForceRefresh();
							}
						}
						return FReply::Handled();
					})
			];
	}
}

void FTryllAgentComponentCustomization::BuildLiveAgentGroup(
	IDetailCategoryBuilder& TryllCategory, UTryllAgentComponent* Component)
{
	IDetailGroup& Category = TryllCategory.AddGroup(
		TEXT("LiveVariables"),
		LOCTEXT("LiveVarsCategory", "Live Agent — Variables"),
		/*bForAdvanced*/ false,
		/*bStartExpanded*/ true);

	TSharedPtr<FTryllLiveVariableUIState> State = LiveUI;
	TSharedPtr<IPropertyUtilities> Utils = PropertyUtils;

	Category.AddWidgetRow()
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("LiveStateLabel", "Agent"))
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		[
			SNew(STextBlock)
			.Text_Lambda([WeakComp = TWeakObjectPtr<UTryllAgentComponent>(Component)]()
			{
				UTryllAgentComponent* Comp = WeakComp.Get();
				return (Comp && Comp->HasLiveAgent())
					? LOCTEXT("LiveYes", "Live")
					: LOCTEXT("LiveNo", "No live agent");
			})
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.ColorAndOpacity_Lambda([WeakComp = TWeakObjectPtr<UTryllAgentComponent>(Component)]()
			{
				UTryllAgentComponent* Comp = WeakComp.Get();
				return (Comp && Comp->HasLiveAgent())
					? FSlateColor(FLinearColor(0.5f, 1.f, 0.5f))
					: FSlateColor::UseSubduedForeground();
			})
		];

	Category.AddWidgetRow()
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("DirtyLabel", "Dirty"))
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		[
			SNew(STextBlock)
			.Text_Lambda([WeakComp = TWeakObjectPtr<UTryllAgentComponent>(Component)]()
			{
				UTryllAgentComponent* Comp = WeakComp.Get();
				FTryllAgentVariables* Vars = Comp ? Comp->GetLiveVariables() : nullptr;
				if (!Vars)
				{
					return LOCTEXT("DirtyN/A", "—");
				}
				return Vars->IsDirty()
					? LOCTEXT("DirtyYes", "Yes — pending flush")
					: LOCTEXT("DirtyNo", "No");
			})
			.Font(IDetailLayoutBuilder::GetDetailFont())
		];

	Category.AddWidgetRow()
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FlushLabel", "Flush"))
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		[
			SNew(SButton)
			.Text(LOCTEXT("FlushNow", "Flush Now"))
			.IsEnabled_Lambda([WeakComp = TWeakObjectPtr<UTryllAgentComponent>(Component)]()
			{
				UTryllAgentComponent* Comp = WeakComp.Get();
				FTryllAgentVariables* Vars = Comp ? Comp->GetLiveVariables() : nullptr;
				return Vars && Vars->IsDirty();
			})
			.OnClicked_Lambda(
				[State, Utils, WeakComp = TWeakObjectPtr<UTryllAgentComponent>(Component)]()
				{
					UTryllAgentComponent* Comp = WeakComp.Get();
					FTryllAgentVariables* Vars = Comp ? Comp->GetLiveVariables() : nullptr;
					if (!Vars || !State.IsValid())
					{
						if (State.IsValid())
						{
							State->LiveStatus = TEXT("No live agent.");
							State->bLiveStatusIsError = true;
						}
						return FReply::Handled();
					}
					UE_LOG(LogTryll, Log, TEXT("[Tryll] VariableOverrides: Flush Now requested."));
					Vars->FlushIfDirty([State, Utils](const FTryllError& Err)
					{
						if (!State.IsValid())
						{
							return;
						}
						if (Err.IsOk())
						{
							State->LiveStatus = TEXT("Flush succeeded — server has the staged values.");
							State->bLiveStatusIsError = false;
							UE_LOG(LogTryll, Log, TEXT("[Tryll] VariableOverrides: Flush Now succeeded."));
						}
						else
						{
							State->LiveStatus = FString::Printf(
								TEXT("Flush failed: %d %s"), Err.Code, *Err.Message);
							State->bLiveStatusIsError = true;
							UE_LOG(LogTryll, Warning,
								TEXT("[Tryll] VariableOverrides: Flush Now failed: %d %s"),
								Err.Code, *Err.Message);
						}
						if (Utils.IsValid())
						{
							Utils->ForceRefresh();
						}
					});
					return FReply::Handled();
				})
		];

	Category.AddWidgetRow()
		.WholeRowContent()
		[
			SNew(STextBlock)
			.Text_Lambda([State]()
			{
				return State.IsValid() && !State->LiveStatus.IsEmpty()
					? FText::FromString(State->LiveStatus)
					: LOCTEXT("StatusIdle", "Override edits stage on the local mirror; Flush Now or next Send/Resume/ChangeParams.");
			})
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.ColorAndOpacity_Lambda([State]()
			{
				return (State.IsValid() && State->bLiveStatusIsError)
					? FSlateColor(FLinearColor(1.f, 0.4f, 0.4f))
					: FSlateColor::UseSubduedForeground();
			})
			.AutoWrapText(true)
		];
}

void FTryllAgentComponentCustomization::ApplyRowLive(
	UTryllAgentComponent* Component, const FString& DeclName, bool bEnabled)
{
	if (!Component || !Component->HasLiveAgent())
	{
		return;
	}
	FTryllAgentVariables* Vars = Component->GetLiveVariables();
	if (!Vars)
	{
		return;
	}

	const TArray<FTryllVariableDecl> Declared = TryllVariableOverrideUtils::ResolveDeclared(Component);
	const FTryllVariableDecl* DeclPtr = nullptr;
	for (const FTryllVariableDecl& D : Declared)
	{
		if (D.Name == DeclName)
		{
			DeclPtr = &D;
			break;
		}
	}
	if (!DeclPtr)
	{
		return;
	}

	const FTryllVariableDecl* OverrideValue = nullptr;
	const int32 Idx = FindOverrideIndex(Component->VariableOverrides, DeclName);
	if (Idx != INDEX_NONE)
	{
		OverrideValue = &Component->VariableOverrides[Idx].Value;
	}

	const FTryllError Err = TryllVariableOverrideUtils::ApplyEffectiveValue(
		*Vars, *DeclPtr, OverrideValue, bEnabled);
	if (!LiveUI.IsValid())
	{
		LiveUI = MakeShared<FTryllLiveVariableUIState>();
	}
	if (!Err.IsOk())
	{
		LiveUI->LiveStatus = FString::Printf(TEXT("Apply failed: %d %s"), Err.Code, *Err.Message);
		LiveUI->bLiveStatusIsError = true;
		UE_LOG(LogTryll, Warning,
			TEXT("[Tryll] VariableOverrides: apply '%s' failed: %d %s"),
			*DeclName, Err.Code, *Err.Message);
		return;
	}

	const FString Preview = FormatDeclPreview(bEnabled && OverrideValue ? *OverrideValue : *DeclPtr);
	LiveUI->LiveStatus = bEnabled
		? FString::Printf(TEXT("Updated '%s'=%s on local mirror (flush deferred until next Send/Resume/ChangeParams, or Flush Now)."),
			*DeclName, *Preview)
		: FString::Printf(TEXT("'%s' restored to workflow default (%s) on local mirror (flush deferred)."),
			*DeclName, *Preview);
	LiveUI->bLiveStatusIsError = false;
	UE_LOG(LogTryll, Log,
		TEXT("[Tryll] VariableOverrides: staged '%s'=%s (enabled=%s, dirty=%s). Flush on next Send/Resume/ChangeParams or Flush Now."),
		*DeclName, *Preview, bEnabled ? TEXT("true") : TEXT("false"),
		Vars->IsDirty() ? TEXT("true") : TEXT("false"));
}

#undef LOCTEXT_NAMESPACE
