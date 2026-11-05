// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllVariableDeclCustomization.h"
#include "TryllVariableDecl.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "PropertyHandle.h"

TSharedRef<IPropertyTypeCustomization> FTryllVariableDeclCustomization::MakeInstance()
{
	return MakeShared<FTryllVariableDeclCustomization>();
}

void FTryllVariableDeclCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& /*CustomizationUtils*/)
{
	HeaderRow.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	];
}

void FTryllVariableDeclCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& ChildBuilder,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	const TSharedPtr<IPropertyHandle> NameHandle   = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTryllVariableDecl, Name));
	TypeHandle                                     = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTryllVariableDecl, Type));
	const TSharedPtr<IPropertyHandle> AllowHandle  = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTryllVariableDecl, bAllowOutputSubstitution));
	const TSharedPtr<IPropertyHandle> IntHandle     = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTryllVariableDecl, IntValue));
	const TSharedPtr<IPropertyHandle> FloatHandle   = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTryllVariableDecl, FloatValue));
	const TSharedPtr<IPropertyHandle> StringHandle  = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTryllVariableDecl, StringValue));
	const TSharedPtr<IPropertyHandle> BoolHandle    = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTryllVariableDecl, bBoolValue));
	const TSharedPtr<IPropertyHandle> SetHandle     = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTryllVariableDecl, StringSetValue));

	if (NameHandle.IsValid())
		ChildBuilder.AddProperty(NameHandle.ToSharedRef());

	if (TypeHandle.IsValid())
	{
		ChildBuilder.AddProperty(TypeHandle.ToSharedRef());
		// Re-render this row when Type changes so the visible value field follows.
		if (const TSharedPtr<IPropertyUtilities> Utils = CustomizationUtils.GetPropertyUtilities())
		{
			TypeHandle->SetOnPropertyValueChanged(
				FSimpleDelegate::CreateLambda([Type = TypeHandle, Allow = AllowHandle, Utils]()
				{
					uint8 NewRawType = 0;
					if (Type.IsValid() && Type->GetValue(NewRawType) == FPropertyAccess::Success &&
						static_cast<ETryllVariableType>(NewRawType) == ETryllVariableType::StringSet &&
						Allow.IsValid())
					{
						Allow->SetValue(false);
					}
					Utils->ForceRefresh();
				}));
		}
	}

	uint8 RawType = 0;
	if (TypeHandle.IsValid())
	{
		TypeHandle->GetValue(RawType);
	}

	const ETryllVariableType Type = static_cast<ETryllVariableType>(RawType);
	if (AllowHandle.IsValid())
	{
		if (Type == ETryllVariableType::StringSet)
		{
			AllowHandle->SetValue(false);
		}
		else
		{
			ChildBuilder.AddProperty(AllowHandle.ToSharedRef());
		}
	}

	switch (Type)
	{
	case ETryllVariableType::Int:
		if (IntHandle.IsValid()) ChildBuilder.AddProperty(IntHandle.ToSharedRef());
		break;
	case ETryllVariableType::Float:
		if (FloatHandle.IsValid()) ChildBuilder.AddProperty(FloatHandle.ToSharedRef());
		break;
	case ETryllVariableType::String:
		if (StringHandle.IsValid()) ChildBuilder.AddProperty(StringHandle.ToSharedRef());
		break;
	case ETryllVariableType::Bool:
		if (BoolHandle.IsValid()) ChildBuilder.AddProperty(BoolHandle.ToSharedRef());
		break;
	case ETryllVariableType::StringSet:
		if (SetHandle.IsValid()) ChildBuilder.AddProperty(SetHandle.ToSharedRef());
		break;
	default:
		break;
	}
}
