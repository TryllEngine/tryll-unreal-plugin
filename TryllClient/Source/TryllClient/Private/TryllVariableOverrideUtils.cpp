// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllVariableOverrideUtils.h"
#include "TryllAgentComponent.h"
#include "TryllAgentVariables.h"
#include "TryllWorkflowAsset.h"
#include "TryllClientModule.h"

TArray<FTryllVariableDecl> TryllVariableOverrideUtils::ResolveDeclared(const UTryllAgentComponent* Component)
{
	if (!Component)
	{
		return {};
	}
	if (Component->WorkflowAsset)
	{
		return Component->WorkflowAsset->Variables;
	}
	return {};
}

int32 TryllVariableOverrideUtils::FindFirstOverrideIndex(
	const TArray<FTryllVariableOverride>& Overrides, const FString& Name)
{
	if (Name.IsEmpty())
	{
		return INDEX_NONE;
	}
	for (int32 i = 0; i < Overrides.Num(); ++i)
	{
		if (Overrides[i].Name == Name)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

FTryllVariableDecl TryllVariableOverrideUtils::CloneDecl(const FTryllVariableDecl& Source)
{
	FTryllVariableDecl Copy = Source;
	Copy.StringSetValue = Source.StringSetValue;
	return Copy;
}

void TryllVariableOverrideUtils::SeedFromDeclaration(
	FTryllVariableOverride& Override, const FTryllVariableDecl& Decl)
{
	Override.Name  = Decl.Name;
	Override.Value = CloneDecl(Decl);
}

FTryllVariableOverride* TryllVariableOverrideUtils::EnsureEntry(
	TArray<FTryllVariableOverride>& Overrides, const FTryllVariableDecl& Decl)
{
	if (Decl.Name.IsEmpty())
	{
		return nullptr;
	}

	const int32 Idx = FindFirstOverrideIndex(Overrides, Decl.Name);
	if (Idx == INDEX_NONE)
	{
		FTryllVariableOverride Created;
		Created.Name     = Decl.Name;
		Created.bEnabled = false;
		Created.Value    = CloneDecl(Decl);
		return &Overrides.Add_GetRef(MoveTemp(Created));
	}

	FTryllVariableOverride& Existing = Overrides[Idx];
	Existing.Name = Decl.Name;
	if (Existing.Value.Type != Decl.Type)
	{
		Existing.Value = CloneDecl(Decl);
	}
	else
	{
		Existing.Value.Name = Decl.Name;
	}
	return &Existing;
}

TArray<int32> TryllVariableOverrideUtils::CollectLegacyIndices(
	const TArray<FTryllVariableOverride>& Overrides,
	const TArray<FTryllVariableDecl>& Declared)
{
	TArray<int32> Result;
	TSet<FString> DeclaredNames;
	for (const FTryllVariableDecl& D : Declared)
	{
		if (!D.Name.IsEmpty())
		{
			DeclaredNames.Add(D.Name);
		}
	}

	TSet<FString> SeenNames;
	for (int32 i = 0; i < Overrides.Num(); ++i)
	{
		const FTryllVariableOverride& O = Overrides[i];
		if (O.Name.IsEmpty())
		{
			Result.Add(i);
			continue;
		}
		if (!DeclaredNames.Contains(O.Name) || SeenNames.Contains(O.Name))
		{
			Result.Add(i);
			continue;
		}
		SeenNames.Add(O.Name);
	}
	return Result;
}

int32 TryllVariableOverrideUtils::RemoveLegacyEntries(
	TArray<FTryllVariableOverride>& Overrides,
	const TArray<FTryllVariableDecl>& Declared)
{
	const TArray<int32> Legacy = CollectLegacyIndices(Overrides, Declared);
	for (int32 i = Legacy.Num() - 1; i >= 0; --i)
	{
		Overrides.RemoveAt(Legacy[i]);
	}
	return Legacy.Num();
}

TArray<FTryllVariableDecl> TryllVariableOverrideUtils::MergeVariableOverrides(
	const TArray<FTryllVariableDecl>& Declared,
	const TArray<FTryllVariableOverride>& Overrides)
{
	TArray<FTryllVariableDecl> Result;
	Result.Reserve(Declared.Num());

	// First serialized entry per name is canonical (matches CollectLegacyIndices /
	// Details). Later duplicates are ignored even if enabled; a disabled first
	// entry means the declaration default is used.
	TSet<FString> DeclaredNames;
	for (const FTryllVariableDecl& D : Declared)
	{
		if (D.Name.IsEmpty())
		{
			continue;
		}
		DeclaredNames.Add(D.Name);

		const int32 Idx = FindFirstOverrideIndex(Overrides, D.Name);
		if (Idx != INDEX_NONE && Overrides[Idx].bEnabled)
		{
			const FTryllVariableOverride& Ov = Overrides[Idx];
			if (Ov.Value.Type == D.Type)
			{
				FTryllVariableDecl Merged = Ov.Value;
				Merged.Name = D.Name;
				Merged.Type = D.Type;
				Merged.bAllowOutputSubstitution = D.bAllowOutputSubstitution;
				Result.Add(MoveTemp(Merged));
				continue;
			}
			UE_LOG(LogTryll, Warning,
				TEXT("UTryllAgentComponent: variable override '%s' type mismatch — using the declared initial value"),
				*D.Name);
		}
		Result.Add(D);
	}

	for (const FTryllVariableOverride& O : Overrides)
	{
		if (O.bEnabled && !O.Name.IsEmpty() && !DeclaredNames.Contains(O.Name))
		{
			UE_LOG(LogTryll, Warning,
				TEXT("UTryllAgentComponent: variable override '%s' does not match any declared variable — ignoring (stale?)"),
				*O.Name);
		}
	}

	return Result;
}

FTryllError TryllVariableOverrideUtils::AssignFromDecl(
	FTryllAgentVariables& Vars,
	const FString& Name,
	ETryllVariableType Type,
	const FTryllVariableDecl& Source)
{
	switch (Type)
	{
	case ETryllVariableType::Int:       return Vars.SetInt(Name, Source.IntValue);
	case ETryllVariableType::Float:     return Vars.SetFloat(Name, Source.FloatValue);
	case ETryllVariableType::String:    return Vars.SetString(Name, Source.StringValue);
	case ETryllVariableType::Bool:      return Vars.SetBool(Name, Source.bBoolValue);
	case ETryllVariableType::StringSet: return Vars.SetStringSet(Name, Source.StringSetValue);
	default:
		return FTryllError(ETryllErrorCode::InvalidVariableValue,
			FString::Printf(TEXT("Unsupported type for variable '%s'"), *Name));
	}
}

FTryllError TryllVariableOverrideUtils::ApplyEffectiveValue(
	FTryllAgentVariables& Vars,
	const FTryllVariableDecl& Declared,
	const FTryllVariableDecl* OverrideValue,
	bool bEnabled)
{
	if (Declared.Name.IsEmpty())
	{
		return FTryllError(ETryllErrorCode::InvalidVariableValue, TEXT("Missing declaration"));
	}
	const FTryllVariableDecl& Source =
		(bEnabled && OverrideValue != nullptr) ? *OverrideValue : Declared;
	return AssignFromDecl(Vars, Declared.Name, Declared.Type, Source);
}
