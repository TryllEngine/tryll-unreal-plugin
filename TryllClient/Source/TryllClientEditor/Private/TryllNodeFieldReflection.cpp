// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllNodeFieldReflection.h"

#include "Graph/TryllNodeStructuralReflection.h"
#include "TryllGraphDescription.h"
#include "Generated/Nodes/TryllNodeParamsBase.h"
#include "Generated/Nodes/TryllOutputFilterOverrides.h"
#include "Generated/Nodes/TryllSamplingOverrides.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* KindName(ETryllFieldKind Kind)
	{
		switch (Kind)
		{
			case ETryllFieldKind::Int:   return TEXT("int");
			case ETryllFieldKind::Float: return TEXT("float");
			case ETryllFieldKind::Bool:  return TEXT("bool");
			default:                     return TEXT("value");
		}
	}

	/**
	 * Bounds render the way .NET's invariant ToString does, because these strings are asserted
	 * against Unity's: a whole number loses its fractional part ("2", not "2.0").
	 */
	FString FormatBound(double Value)
	{
		const double Rounded = FMath::RoundToDouble(Value);
		return FMath::IsNearlyEqual(Value, Rounded)
			? FString::Printf(TEXT("%lld"), static_cast<int64>(Rounded))
			: FString::SanitizeFloat(Value);
	}

	FString SplitPascalCase(const FString& Name)
	{
		FString Out;
		for (int32 Index = 0; Index < Name.Len(); ++Index)
		{
			const TCHAR Ch = Name[Index];
			if (Index > 0 && FChar::IsUpper(Ch) && !FChar::IsUpper(Name[Index - 1]))
			{
				Out.AppendChar(TEXT(' '));
			}
			Out.AppendChar(Ch);
		}
		return Out;
	}

	/** A bOverrideX toggle is machinery, not a field a user overrides. */
	bool IsOverrideToggle(const FProperty* Prop)
	{
		return Prop->IsA<FBoolProperty>() && Prop->GetName().StartsWith(TEXT("bOverride"));
	}

	/** Text-ID companions are not wire fields. Shipping builds strip the meta, so the names are the contract. */
	bool IsLocalizedCompanion(const FProperty* Prop)
	{
		const FString Name = Prop->GetName();
		if (Name.EndsWith(TEXT("TextId")))
		{
			return true;
		}
		return Prop->IsA<FBoolProperty>() && Name.EndsWith(TEXT("Localized"));
	}

	/**
	 * Display name for one property. Unreal's codegen prefixes bools with `b`, which Unity's
	 * fields do not carry — dropping it here keeps both windows reading the same, while the
	 * FieldPath (what gets persisted and exported) stays the real property name.
	 */
	FString DisplayNameOf(const FProperty* Prop)
	{
		FString Name = Prop->GetName();
		if (Prop->IsA<FBoolProperty>() && Name.Len() > 1 && Name[0] == TEXT('b')
			&& FChar::IsUpper(Name[1]))
		{
			Name.RightChopInline(1);
		}
		return SplitPascalCase(Name);
	}

	FBoolProperty* FindToggleFor(const UStruct* Owner, const FProperty* Value)
	{
		return CastField<FBoolProperty>(
			Owner->FindPropertyByName(*(TEXT("bOverride") + Value->GetName())));
	}

	void ReadBounds(const FProperty* Prop, TOptional<double>& OutMin, TOptional<double>& OutMax)
	{
#if WITH_METADATA
		const FString Min = Prop->GetMetaData(TEXT("ClampMin"));
		const FString Max = Prop->GetMetaData(TEXT("ClampMax"));
		if (!Min.IsEmpty()) { OutMin = FCString::Atod(*Min); }
		if (!Max.IsEmpty()) { OutMax = FCString::Atod(*Max); }
#endif
	}

	bool IsMultiline(const FProperty* Prop)
	{
#if WITH_METADATA
		return Prop->HasMetaData(TEXT("MultiLine"));
#else
		return false;
#endif
	}

	bool KindOf(const FProperty* Prop, ETryllFieldKind& OutKind, const UEnum*& OutEnum)
	{
		OutEnum = nullptr;
		if (Prop->IsA<FStrProperty>())
		{
			OutKind = IsMultiline(Prop) ? ETryllFieldKind::MultilineString : ETryllFieldKind::String;
			return true;
		}
		if (Prop->IsA<FIntProperty>())   { OutKind = ETryllFieldKind::Int;   return true; }
		if (Prop->IsA<FFloatProperty>()
			|| Prop->IsA<FDoubleProperty>()) { OutKind = ETryllFieldKind::Float; return true; }
		if (Prop->IsA<FBoolProperty>())  { OutKind = ETryllFieldKind::Bool;  return true; }
		if (const FEnumProperty* AsEnum = CastField<FEnumProperty>(Prop))
		{
			OutKind = ETryllFieldKind::Enum;
			OutEnum = AsEnum->GetEnum();
			return true;
		}
		if (const FByteProperty* AsByte = CastField<FByteProperty>(Prop))
		{
			if (AsByte->Enum != nullptr)
			{
				OutKind = ETryllFieldKind::Enum;
				OutEnum = AsByte->Enum;
				return true;
			}
		}
		return false;
	}

	/** Resolves a dotted path to the leaf property and the address of its container. */
	bool ResolvePath(
		const UStruct*   OwnerStruct,
		void*            OwnerAddress,
		const FString&   FieldPath,
		FProperty*&      OutLeaf,
		void*&           OutContainer,
		const UStruct*&  OutLeafOwner)
	{
		FString Head;
		FString Tail = FieldPath;
		const UStruct* Struct = OwnerStruct;
		void* Address = OwnerAddress;

		while (Tail.Split(TEXT("."), &Head, &Tail))
		{
			FStructProperty* Nested = CastField<FStructProperty>(Struct->FindPropertyByName(*Head));
			if (Nested == nullptr)
			{
				return false;
			}
			Address = Nested->ContainerPtrToValuePtr<void>(Address);
			Struct  = Nested->Struct;
		}

		OutLeaf = Struct->FindPropertyByName(*Tail);
		OutContainer = Address;
		OutLeafOwner = Struct;
		return OutLeaf != nullptr;
	}
}

TArray<FTryllOverridableField> FTryllNodeFieldReflection::EnumerateOverridableFields(
	const UClass* ParamsClass)
{
	TArray<FTryllOverridableField> Result;
	if (ParamsClass == nullptr)
	{
		return Result;
	}

	for (TFieldIterator<FProperty> It(ParamsClass); It; ++It)
	{
		FProperty* Prop = *It;
		if (FTryllNodeStructuralReflection::IsStructural(Prop) || IsOverrideToggle(Prop) || IsLocalizedCompanion(Prop))
		{
			continue;
		}

		if (const FStructProperty* AsStruct = CastField<FStructProperty>(Prop))
		{
			// Both typed override groups expand into one entry per sub-field. Anything else
			// nested is not overridable, so it is skipped rather than half-offered.
			if (AsStruct->Struct != FTryllSamplingOverrides::StaticStruct()
				&& AsStruct->Struct != FTryllOutputFilterOverrides::StaticStruct())
			{
				continue;
			}
			for (TFieldIterator<FProperty> Sub(AsStruct->Struct); Sub; ++Sub)
			{
				FProperty* SubProp = *Sub;
				if (IsOverrideToggle(SubProp) || IsLocalizedCompanion(SubProp))
				{
					continue;
				}
				ETryllFieldKind Kind;
				const UEnum* EnumType = nullptr;
				if (!KindOf(SubProp, Kind, EnumType))
				{
					continue;
				}
				FTryllOverridableField Field;
				Field.FieldPath   = Prop->GetName() + TEXT(".") + SubProp->GetName();
				Field.DisplayName = FString::Printf(
					TEXT("%s ▶ %s"), *SplitPascalCase(Prop->GetName()), *DisplayNameOf(SubProp));
				Field.Kind        = Kind;
				Field.EnumType    = EnumType;
				Field.bIsOptional = FindToggleFor(AsStruct->Struct, SubProp) != nullptr;
				ReadBounds(SubProp, Field.Min, Field.Max);
				Result.Add(MoveTemp(Field));
			}
			continue;
		}

		ETryllFieldKind Kind;
		const UEnum* EnumType = nullptr;
		if (!KindOf(Prop, Kind, EnumType))
		{
			continue;
		}

		FTryllOverridableField Field;
		Field.FieldPath   = Prop->GetName();
		Field.DisplayName = DisplayNameOf(Prop);
		Field.Kind        = Kind;
		Field.EnumType    = EnumType;
		Field.bIsOptional = FindToggleFor(ParamsClass, Prop) != nullptr;
		ReadBounds(Prop, Field.Min, Field.Max);
		Result.Add(MoveTemp(Field));
	}

	return Result;
}

bool FTryllNodeFieldReflection::TryGetField(
	const UClass* ParamsClass, const FString& FieldPath, FTryllOverridableField& OutField)
{
	for (const FTryllOverridableField& Field : EnumerateOverridableFields(ParamsClass))
	{
		if (Field.FieldPath == FieldPath)
		{
			OutField = Field;
			return true;
		}
	}
	return false;
}

FString FTryllNodeFieldReflection::GetValue(
	const UTryllNodeParamsBase* Params, const FTryllOverridableField& Field)
{
	if (Params == nullptr)
	{
		return FString();
	}

	FProperty* Leaf = nullptr;
	void* Container = nullptr;
	const UStruct* LeafOwner = nullptr;
	if (!ResolvePath(Params->GetClass(), const_cast<UTryllNodeParamsBase*>(Params),
			Field.FieldPath, Leaf, Container, LeafOwner))
	{
		return FString();
	}

	if (Field.bIsOptional)
	{
		if (const FBoolProperty* Toggle = FindToggleFor(LeafOwner, Leaf))
		{
			if (!Toggle->GetPropertyValue_InContainer(Container))
			{
				return FString();
			}
		}
	}

	switch (Field.Kind)
	{
		case ETryllFieldKind::String:
		case ETryllFieldKind::MultilineString:
			return *CastField<FStrProperty>(Leaf)->ContainerPtrToValuePtr<FString>(Container);
		case ETryllFieldKind::Int:
			return FString::FromInt(CastField<FIntProperty>(Leaf)->GetPropertyValue_InContainer(Container));
		case ETryllFieldKind::Float:
		{
			const double Value = CastField<FFloatProperty>(Leaf) != nullptr
				? CastField<FFloatProperty>(Leaf)->GetPropertyValue_InContainer(Container)
				: CastField<FDoubleProperty>(Leaf)->GetPropertyValue_InContainer(Container);
			return FString::SanitizeFloat(Value);
		}
		case ETryllFieldKind::Bool:
			return CastField<FBoolProperty>(Leaf)->GetPropertyValue_InContainer(Container)
				? TEXT("true") : TEXT("false");
		case ETryllFieldKind::Enum:
		{
			FString Out;
			Leaf->ExportTextItem_InContainer(Out, Container, nullptr, nullptr, PPF_None);
			return Out;
		}
	}
	return FString();
}

FString FTryllNodeFieldReflection::ValidateValue(
	const FTryllOverridableField& Field, const FString& Value)
{
	if (Field.Kind == ETryllFieldKind::String || Field.Kind == ETryllFieldKind::MultilineString)
	{
		return FString();
	}
	if (Field.Kind == ETryllFieldKind::Bool)
	{
		return (Value == TEXT("true") || Value == TEXT("false"))
			? FString() : FString(TEXT("invalid bool"));
	}
	if (Field.Kind == ETryllFieldKind::Enum)
	{
		const bool bKnown = Field.EnumType != nullptr
			&& Field.EnumType->GetIndexByNameString(Value) != INDEX_NONE;
		return bKnown
			? FString()
			: FString::Printf(TEXT("invalid enum value '%s'"), *Value);
	}

	if (Value.TrimStartAndEnd().IsEmpty())
	{
		return Field.bIsOptional ? FString() : FString(TEXT("value required"));
	}
	if (!Value.IsNumeric())
	{
		return FString::Printf(TEXT("invalid %s"), KindName(Field.Kind));
	}
	if (Field.Kind == ETryllFieldKind::Int && Value.Contains(TEXT(".")))
	{
		return FString(TEXT("invalid int"));
	}

	const double Parsed = FCString::Atod(*Value);
	if (Field.Min.IsSet() && Parsed < Field.Min.GetValue())
	{
		return FString::Printf(TEXT("must be ≥ %s"), *FormatBound(Field.Min.GetValue()));
	}
	if (Field.Max.IsSet() && Parsed > Field.Max.GetValue())
	{
		return FString::Printf(TEXT("must be ≤ %s"), *FormatBound(Field.Max.GetValue()));
	}
	return FString();
}

FString FTryllNodeFieldReflection::TrySet(
	UTryllNodeParamsBase* Params, const FTryllOverridableField& Field, const FString& Value)
{
	if (Params == nullptr)
	{
		return TEXT("missing params target");
	}

	FProperty* Leaf = nullptr;
	void* Container = nullptr;
	const UStruct* LeafOwner = nullptr;
	if (!ResolvePath(Params->GetClass(), Params, Field.FieldPath, Leaf, Container, LeafOwner))
	{
		return FString::Printf(TEXT("unknown field '%s'"), *Field.FieldPath);
	}

	FBoolProperty* Toggle = Field.bIsOptional ? FindToggleFor(LeafOwner, Leaf) : nullptr;

	// Clearing an optional switches its toggle off, which is how the codegen represents
	// "no override" — the analogue of Unity writing TryllOptional<T>::None.
	const bool bNumeric = Field.Kind == ETryllFieldKind::Int || Field.Kind == ETryllFieldKind::Float;
	if (Toggle != nullptr && bNumeric && Value.TrimStartAndEnd().IsEmpty())
	{
		Toggle->SetPropertyValue_InContainer(Container, false);
		return FString();
	}

	const FString Error = ValidateValue(Field, Value);
	if (!Error.IsEmpty())
	{
		return Error;
	}

	switch (Field.Kind)
	{
		case ETryllFieldKind::String:
		case ETryllFieldKind::MultilineString:
			*CastField<FStrProperty>(Leaf)->ContainerPtrToValuePtr<FString>(Container) = Value;
			// An override on a Localized field wins: untick it in this copy so the
			// text provider does not replace the value at agent creation.
			if (FBoolProperty* Localized = CastField<FBoolProperty>(
				LeafOwner->FindPropertyByName(*(FString(TEXT("b")) + Leaf->GetName() + TEXT("Localized")))))
			{
				Localized->SetPropertyValue_InContainer(Container, false);
			}
			break;
		case ETryllFieldKind::Int:
			CastField<FIntProperty>(Leaf)->SetPropertyValue_InContainer(
				Container, FCString::Atoi(*Value));
			break;
		case ETryllFieldKind::Float:
			if (FFloatProperty* AsFloat = CastField<FFloatProperty>(Leaf))
			{
				AsFloat->SetPropertyValue_InContainer(Container, FCString::Atof(*Value));
			}
			else
			{
				CastField<FDoubleProperty>(Leaf)->SetPropertyValue_InContainer(
					Container, FCString::Atod(*Value));
			}
			break;
		case ETryllFieldKind::Bool:
			CastField<FBoolProperty>(Leaf)->SetPropertyValue_InContainer(
				Container, Value == TEXT("true"));
			break;
		case ETryllFieldKind::Enum:
			if (Leaf->ImportText_InContainer(*Value, Container, nullptr, PPF_None) == nullptr)
			{
				return FString::Printf(TEXT("invalid enum value '%s'"), *Value);
			}
			break;
	}

	if (Toggle != nullptr)
	{
		Toggle->SetPropertyValue_InContainer(Container, true);
	}
	return FString();
}

bool FTryllNodeFieldReflection::TryApply(
	UTryllNodeParamsBase* Params,
	const TArray<FTryllLabParamOverride>& Overrides,
	FString& OutError)
{
	OutError.Reset();
	if (Params == nullptr)
	{
		OutError = TEXT("missing params target");
		return false;
	}

	const TArray<FTryllOverridableField> Fields = EnumerateOverridableFields(Params->GetClass());
	for (const FTryllLabParamOverride& Override : Overrides)
	{
		const FTryllOverridableField* Field = Fields.FindByPredicate(
			[&Override](const FTryllOverridableField& Candidate)
			{
				return Candidate.FieldPath == Override.FieldPath;
			});
		if (Field == nullptr)
		{
			OutError = FString::Printf(TEXT("unknown field '%s'"), *Override.FieldPath);
			return false;
		}
		const FString Error = TrySet(Params, *Field, Override.Value);
		if (!Error.IsEmpty())
		{
			OutError = FString::Printf(TEXT("%s: %s"), *Override.FieldPath, *Error);
			return false;
		}
	}
	return true;
}

TArray<FString> FTryllNodeFieldReflection::CollectOverrideProblems(
	const FTryllGraphDescription& Graph,
	const TArray<FTryllLabParamOverride>& Overrides)
{
	TArray<FString> Problems;
	TMap<FString, const UClass*> ByName;
	for (const FTryllNodeDescription& Node : Graph.Nodes)
	{
		if (Node.Params != nullptr)
		{
			ByName.Add(Node.Name, Node.Params->GetClass());
		}
	}

	for (const FTryllLabParamOverride& Override : Overrides)
	{
		const UClass* const* ParamsClass = ByName.Find(Override.NodeName);
		if (ParamsClass == nullptr)
		{
			Problems.Add(FString::Printf(
				TEXT("%s.%s: node not in this workflow"), *Override.NodeName, *Override.FieldPath));
			continue;
		}
		FTryllOverridableField Field;
		if (!TryGetField(*ParamsClass, Override.FieldPath, Field))
		{
			Problems.Add(FString::Printf(
				TEXT("%s.%s: field not overridable on %s"),
				*Override.NodeName, *Override.FieldPath, *(*ParamsClass)->GetName()));
			continue;
		}
		const FString Error = ValidateValue(Field, Override.Value);
		if (!Error.IsEmpty())
		{
			Problems.Add(FString::Printf(
				TEXT("%s.%s: %s"), *Override.NodeName, *Override.FieldPath, *Error));
		}
	}
	return Problems;
}
