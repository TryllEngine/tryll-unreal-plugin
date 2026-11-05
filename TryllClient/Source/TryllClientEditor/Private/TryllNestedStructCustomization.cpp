// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllNestedStructCustomization.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailPropertyRow.h"
#include "PropertyHandle.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SBox.h"

#define LOCTEXT_NAMESPACE "TryllNestedStructCustomization"

namespace
{
	constexpr float MultilineMinHeight = 68.f;
	constexpr float MultilineMaxHeight = 180.f;

	bool IsMultilineString(const FProperty* Property)
	{
		return Property
			&& Property->IsA<FStrProperty>()
			&& Property->GetBoolMetaData(TEXT("MultiLine"));
	}
}

IDetailPropertyRow* TryllMultilineProperty::Add(
	IDetailChildrenBuilder& ChildBuilder, const TSharedPtr<IPropertyHandle>& Handle)
{
	if (!Handle.IsValid())
	{
		return nullptr;
	}

	IDetailPropertyRow& Row = ChildBuilder.AddProperty(Handle.ToSharedRef());
	if (!IsMultilineString(Handle->GetProperty()))
	{
		return &Row;
	}

	// A top-level params string (Respond's system prompt) grows with its text.
	// The same meta on a string inside an array element stays one row while
	// the prompt is empty, which is every new choice and system prompt.
	const TSharedRef<IPropertyHandle> HandleRef = Handle.ToSharedRef();
	TArray<TSharedPtr<IPropertyHandle>> Handles;
	Handles.Add(Handle);

	Row.CustomWidget()
		.FilterString(Handle->GetPropertyDisplayName())
		.PropertyHandleList(Handles)
		.NameContent()
		.VAlign(VAlign_Center)
		[
			Handle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(250.f)
		.MaxDesiredWidth(600.f)
		[
			SNew(SBox)
			.MinDesiredHeight(MultilineMinHeight)
			.MaxDesiredHeight(MultilineMaxHeight)
			[
				SNew(SMultiLineEditableTextBox)
				.Text_Lambda([HandleRef]()
				{
					FString Value;
					if (HandleRef->GetValue(Value) == FPropertyAccess::MultipleValues)
					{
						return LOCTEXT("MultipleValues", "Multiple Values");
					}
					return FText::FromString(Value);
				})
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.AutoWrapText(true)
				.ModiferKeyForNewLine(EModifierKey::Shift)
				.SelectAllTextWhenFocused(false)
				.ClearKeyboardFocusOnCommit(false)
				.SelectAllTextOnCommit(false)
				.OnTextCommitted_Lambda([HandleRef](const FText& NewText, ETextCommit::Type)
				{
					const FString Next = NewText.ToString();
					FString Current;
					const FPropertyAccess::Result Access = HandleRef->GetValue(Current);
					if (Access == FPropertyAccess::MultipleValues)
					{
						if (Next != LOCTEXT("MultipleValues", "Multiple Values").ToString())
						{
							HandleRef->SetValue(Next);
						}
						return;
					}
					if (Current != Next)
					{
						HandleRef->SetValue(Next);
					}
				})
			]
		];
	return &Row;
}

TSharedRef<IPropertyTypeCustomization> FTryllNestedStructCustomization::MakeInstance()
{
	return MakeShared<FTryllNestedStructCustomization>();
}

void FTryllNestedStructCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& /*CustomizationUtils*/)
{
	HeaderRow.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	];
}

void FTryllNestedStructCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& ChildBuilder,
	IPropertyTypeCustomizationUtils& /*CustomizationUtils*/)
{
	uint32 Count = 0;
	StructPropertyHandle->GetNumChildren(Count);
	for (uint32 Index = 0; Index < Count; ++Index)
	{
		TryllMultilineProperty::Add(ChildBuilder, StructPropertyHandle->GetChildHandle(Index));
	}
}

#undef LOCTEXT_NAMESPACE
