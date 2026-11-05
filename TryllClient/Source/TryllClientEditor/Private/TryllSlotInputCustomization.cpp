// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllSlotInputCustomization.h"

#include "TryllSlotInput.h"
#include "TryllSlotGraphAnalysis.h"
#include "TryllGraphDescription.h"
#include "Generated/Nodes/TryllNodeParamsBase.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "TryllSlotInputCustomization"

namespace
{
	const FString kUserMessageSlot = TEXT("user_message");
	const FSlateColor kInvalidColor = FSlateColor(FLinearColor(1.f, 0.4f, 0.4f));
}

TSharedRef<IPropertyTypeCustomization> FTryllSlotInputCustomization::MakeInstance()
{
	return MakeShared<FTryllSlotInputCustomization>();
}

FString FTryllSlotInputCustomization::CurrentValue() const
{
	FString Value;
	if (ValueHandle.IsValid()) ValueHandle->GetValue(Value);
	return Value;
}

void FTryllSlotInputCustomization::SetValue(const FString& InValue)
{
	if (ValueHandle.IsValid()) ValueHandle->SetValue(InValue);
}

FString FTryllSlotInputCustomization::LabelForValue(const FString& Value) const
{
	// "User Message" — every input-bearing node (including Speak) defaults an
	// empty `input` to the "user_message" slot.
	if (Value.IsEmpty())
		return TEXT("User Message");
	return FString::Printf(TEXT("Slot %s"), *Value);
}

bool FTryllSlotInputCustomization::IsCurrentValueInvalid() const
{
	const FString Cur = CurrentValue();
	if (Cur.IsEmpty() || Cur == kUserMessageSlot) return false;

	const FTryllGraphDescription* Graph = OwningParams.IsValid()
		? UTryllNodeParamsBase::FindEnclosingGraph(OwningParams.Get())
		: nullptr;
	if (!Graph) return true; // no graph context at all -- can't be resolved, treat as invalid

	const FString OwnerNodeName = OwningParams->GetOwnerNodeName();
	for (const FTryllSlotOption& Option : TryllSlotGraphAnalysis::ClassifySlots(*Graph, OwnerNodeName, Cur))
		if (Option.Value == Cur)
			return !Option.IsEnabled();
	return true;
}

FText FTryllSlotInputCustomization::CurrentValueTooltip() const
{
	const FString Cur = CurrentValue();
	if (Cur.IsEmpty() || Cur == kUserMessageSlot) return FText::GetEmpty();

	const FTryllGraphDescription* Graph = OwningParams.IsValid()
		? UTryllNodeParamsBase::FindEnclosingGraph(OwningParams.Get())
		: nullptr;
	if (!Graph) return FText::GetEmpty();

	const FString OwnerNodeName = OwningParams->GetOwnerNodeName();
	for (const FTryllSlotOption& Option : TryllSlotGraphAnalysis::ClassifySlots(*Graph, OwnerNodeName, Cur))
		if (Option.Value == Cur && !Option.IsEnabled())
			return FText::FromString(Option.LongReason(OwnerNodeName));
	return FText::GetEmpty();
}

TSharedRef<SWidget> FTryllSlotInputCustomization::BuildMenu()
{
	TArray<FTryllSlotOption> Options;
	const FTryllGraphDescription* Graph = OwningParams.IsValid()
		? UTryllNodeParamsBase::FindEnclosingGraph(OwningParams.Get())
		: nullptr;
	FString OwnerNodeName;
	if (Graph)
	{
		OwnerNodeName = OwningParams->GetOwnerNodeName();
		Options = TryllSlotGraphAnalysis::ClassifySlots(*Graph, OwnerNodeName, CurrentValue());
	}

	const FString Cur = CurrentValue();

	FMenuBuilder MenuBuilder(/*bCloseSelfOnly=*/true, nullptr);

	MenuBuilder.BeginSection(NAME_None, LOCTEXT("AvailableSection", "Available"));
	{
		// Default entry (empty value) is always enabled and always first.
		MenuBuilder.AddMenuEntry(
			FText::FromString(LabelForValue(FString())),
			LOCTEXT("DefaultEntryTooltip", "Use the context-specific default input for this node."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this] { SetValue(FString()); })),
			NAME_None, EUserInterfaceActionType::Button);

		for (const FTryllSlotOption& Option : Options)
		{
			if (!Option.IsEnabled()) continue;
			const FString Note = Option.ShortReason();
			const FString Label = Note.IsEmpty()
				? LabelForValue(Option.Value)
				: FString::Printf(TEXT("%s  %s"), *LabelForValue(Option.Value), *Note);
			const FString Value = Option.Value;

			MenuBuilder.AddMenuEntry(
				FText::FromString(Label),
				FText::FromString(Option.LongReason(OwnerNodeName)),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([this, Value] { SetValue(Value); })),
				NAME_None, EUserInterfaceActionType::Button);
		}
	}
	MenuBuilder.EndSection();

	const bool bHasDisabled = Options.ContainsByPredicate([](const FTryllSlotOption& O) { return !O.IsEnabled(); });
	if (bHasDisabled)
	{
		MenuBuilder.BeginSection(NAME_None, LOCTEXT("UnavailableSection", "Unavailable"));
		for (const FTryllSlotOption& Option : Options)
		{
			if (Option.IsEnabled()) continue;
			const FString Note = Option.ShortReason();
			const FString Label = Note.IsEmpty()
				? LabelForValue(Option.Value)
				: FString::Printf(TEXT("%s  %s"), *LabelForValue(Option.Value), *Note);

			MenuBuilder.AddMenuEntry(
				FText::FromString(Label),
				FText::FromString(Option.LongReason(OwnerNodeName)),
				FSlateIcon(),
				FUIAction(
					FExecuteAction(),
					FCanExecuteAction::CreateLambda([] { return false; })),
				NAME_None, EUserInterfaceActionType::Check);
		}
		MenuBuilder.EndSection();
	}

	return MenuBuilder.MakeWidget();
}

void FTryllSlotInputCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& /*CustomizationUtils*/)
{
	ValueHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTryllSlotInput, Value));
	if (!ValueHandle.IsValid()) return;

	TArray<UObject*> OuterObjects;
	StructPropertyHandle->GetOuterObjects(OuterObjects);
	if (OuterObjects.Num() > 0)
		OwningParams = Cast<UTryllNodeParamsBase>(OuterObjects[0]);

	HeaderRow.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(280.f)
	[
		SNew(SComboButton)
		.OnGetMenuContent(FOnGetContent::CreateSP(this, &FTryllSlotInputCustomization::BuildMenu))
		.ButtonContent()
		[
			SNew(STextBlock)
			.Text_Lambda([this] { return FText::FromString(LabelForValue(CurrentValue())); })
			.ToolTipText_Lambda([this] { return CurrentValueTooltip(); })
			.ColorAndOpacity_Lambda([this]
			{
				return IsCurrentValueInvalid() ? kInvalidColor : FSlateColor::UseForeground();
			})
		]
	];
}

void FTryllSlotInputCustomization::CustomizeChildren(
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
