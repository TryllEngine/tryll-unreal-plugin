// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllClassifyAttributeDefCustomization.h"
#include "TryllNestedStructCustomization.h"

#include "Generated/Nodes/TryllClassifyAttributeChoice.h"
#include "Generated/Nodes/TryllClassifyAttributeDef.h"
#include "Generated/Nodes/TryllClassifyAttributesLLMParams.h"
#include "TryllGraphDescription.h"
#include "TryllRenamePropagation.h"
#include "TryllUncertainChoice.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "PropertyHandle.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "TryllClassifyAttributeDefCustomization"

TSharedRef<IPropertyTypeCustomization> FTryllClassifyAttributeDefCustomization::MakeInstance()
{
	return MakeShared<FTryllClassifyAttributeDefCustomization>();
}

void FTryllClassifyAttributeDefCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& /*CustomizationUtils*/)
{
	HeaderRow.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	];
}

void FTryllClassifyAttributeDefCustomization::OnNameChanged()
{
	if (!NameHandle.IsValid()) return;
	FString NewName;
	if (NameHandle->GetValue(NewName) != FPropertyAccess::Success)
		return;
	const FString OldName = PreviousName;
	PreviousName = NewName;
	if (OldName.IsEmpty() || NewName.IsEmpty() || OldName == NewName)
		return;

	UTryllClassifyAttributesLLMParams* Params = OwningParams.Get();
	if (!Params) return;
	const FTryllGraphDescription* Graph = UTryllNodeParamsBase::FindEnclosingGraph(Params);
	if (!Graph) return;

	FScopedTransaction Transaction(LOCTEXT("RenameAttribute", "Rename Classify Attribute"));
	TryllRenamePropagation::PropagateAttributeRename(
		*const_cast<FTryllGraphDescription*>(Graph),
		Params,
		Params->GetOwnerNodeName(),
		OldName,
		NewName);
	Params->MarkPackageDirty();
}

void FTryllClassifyAttributeDefCustomization::RebuildUncertainOptions()
{
	UncertainOptions.Reset();
	UncertainOptions.Add(MakeShared<FString>(Tryll::kUncertainChoiceId));
	TSet<FString> Seen;
	Seen.Add(Tryll::kUncertainChoiceId);

	const TSharedPtr<IPropertyHandle> ChoicesHandle = StructHandle.IsValid()
		? StructHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTryllClassifyAttributeDef, Choices))
		: nullptr;
	uint32 ChoiceCount = 0;
	if (ChoicesHandle.IsValid())
		ChoicesHandle->GetNumChildren(ChoiceCount);
	for (uint32 Index = 0; Index < ChoiceCount; ++Index)
	{
		const TSharedPtr<IPropertyHandle> Element = ChoicesHandle->GetChildHandle(Index);
		const TSharedPtr<IPropertyHandle> IdHandle = Element.IsValid()
			? Element->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTryllClassifyAttributeChoice, Id))
			: nullptr;
		FString Id;
		if (!IdHandle.IsValid() || IdHandle->GetValue(Id) != FPropertyAccess::Success)
			continue;
		if (Id.IsEmpty() || Seen.Contains(Id))
			continue;
		Seen.Add(Id);
		UncertainOptions.Add(MakeShared<FString>(Id));
	}

	FString Current;
	if (UncertainHandle.IsValid()
		&& UncertainHandle->GetValue(Current) == FPropertyAccess::Success
		&& !Current.IsEmpty()
		&& !Seen.Contains(Current))
	{
		UncertainOptions.Add(MakeShared<FString>(Current));
	}
}

void FTryllClassifyAttributeDefCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& ChildBuilder,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructHandle = StructPropertyHandle;
	KindHandle = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FTryllClassifyAttributeDef, Kind));
	auto Add = [&](FName Name)
	{
		if (const TSharedPtr<IPropertyHandle> Child = StructPropertyHandle->GetChildHandle(Name))
		{
			TryllMultilineProperty::Add(ChildBuilder, Child);
		}
	};

	NameHandle = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FTryllClassifyAttributeDef, Name));
	if (NameHandle.IsValid())
	{
		NameHandle->GetValue(PreviousName);
		TArray<UObject*> OuterObjects;
		StructPropertyHandle->GetOuterObjects(OuterObjects);
		OwningParams = OuterObjects.Num() > 0
			? Cast<UTryllClassifyAttributesLLMParams>(OuterObjects[0]) : nullptr;
		NameHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([this]()
		{
			OnNameChanged();
		}));
		Add(GET_MEMBER_NAME_CHECKED(FTryllClassifyAttributeDef, Name));
	}
	if (KindHandle.IsValid())
	{
		ChildBuilder.AddProperty(KindHandle.ToSharedRef());
		if (const TSharedPtr<IPropertyUtilities> Utils = CustomizationUtils.GetPropertyUtilities())
		{
			KindHandle->SetOnPropertyValueChanged(
				FSimpleDelegate::CreateLambda([Utils]() { Utils->ForceRefresh(); }));
		}
	}

	uint8 RawKind = 0;
	if (KindHandle.IsValid())
		KindHandle->GetValue(RawKind);
	const ETryllAttributeKind Kind = static_cast<ETryllAttributeKind>(RawKind);

	if (Kind != ETryllAttributeKind::Boolean)
		Add(GET_MEMBER_NAME_CHECKED(FTryllClassifyAttributeDef, Choices));
	if (Kind == ETryllAttributeKind::Boolean)
		Add(GET_MEMBER_NAME_CHECKED(FTryllClassifyAttributeDef, Statement));
	Add(GET_MEMBER_NAME_CHECKED(FTryllClassifyAttributeDef, bOverrideSystemPrompt));
	Add(GET_MEMBER_NAME_CHECKED(FTryllClassifyAttributeDef, SystemPrompt));
	if (Kind != ETryllAttributeKind::Score)
		Add(GET_MEMBER_NAME_CHECKED(FTryllClassifyAttributeDef, Threshold));
	if (Kind == ETryllAttributeKind::Choice)
	{
		Add(GET_MEMBER_NAME_CHECKED(FTryllClassifyAttributeDef, Margin));
		UncertainHandle = StructPropertyHandle->GetChildHandle(
			GET_MEMBER_NAME_CHECKED(FTryllClassifyAttributeDef, UncertainChoice));
		if (UncertainHandle.IsValid())
		{
			RebuildUncertainOptions();
			ChildBuilder.AddCustomRow(LOCTEXT("UncertainFilter", "Uncertain"))
			.NameContent()
			[
				UncertainHandle->CreatePropertyNameWidget(LOCTEXT("UncertainLabel", "Uncertain"))
			]
			.ValueContent()
			.MinDesiredWidth(220.f)
			[
				SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&UncertainOptions)
				.OnComboBoxOpening_Lambda([this] { RebuildUncertainOptions(); })
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
				{
					const FString Label = Item.IsValid() ? *Item : FString();
					return SNew(STextBlock).Text(FText::FromString(Label));
				})
				.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type)
				{
					if (Item.IsValid() && UncertainHandle.IsValid())
						UncertainHandle->SetValue(*Item);
				})
				[
					SNew(STextBlock)
					.Text_Lambda([this]
					{
						FString Current;
						if (!UncertainHandle.IsValid()
							|| UncertainHandle->GetValue(Current) != FPropertyAccess::Success
							|| Current.IsEmpty())
						{
							return LOCTEXT("UncertainUnset", "Select");
						}
						return FText::FromString(Current);
					})
				]
			];
		}
	}
	if (Kind == ETryllAttributeKind::Boolean)
		Add(GET_MEMBER_NAME_CHECKED(FTryllClassifyAttributeDef, BooleanTrigger));
	if (Kind == ETryllAttributeKind::Score)
	{
		Add(GET_MEMBER_NAME_CHECKED(FTryllClassifyAttributeDef, ScoreTrigger));
		Add(GET_MEMBER_NAME_CHECKED(FTryllClassifyAttributeDef, ScoreThreshold));
	}
}

#undef LOCTEXT_NAMESPACE
