// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllDialogCheckJson.h"

#include "TryllDialogLabChecks.h"
#include "TryllDialogLabExport.h"
#include "TryllDialogLabScriptData.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

namespace
{
	FString CheckJsonQuoted(const FString& Value)
	{
		return TEXT("\"") + FTryllDialogLabExport::EscapeJson(Value) + TEXT("\"");
	}

	void Kv(FString& Out, const TCHAR* Key, const FString& Value)
	{
		Out += FString::Printf(TEXT(",\"%s\":%s"), Key, *CheckJsonQuoted(Value));
	}

	void Kv(FString& Out, const TCHAR* Key, bool bValue)
	{
		Out += FString::Printf(TEXT(",\"%s\":%s"), Key, bValue ? TEXT("true") : TEXT("false"));
	}

	void Kv(FString& Out, const TCHAR* Key, int32 Value)
	{
		Out += FString::Printf(TEXT(",\"%s\":%d"), Key, Value);
	}

	void KvFloat(FString& Out, const TCHAR* Key, float Value)
	{
		Out += FString::Printf(TEXT(",\"%s\":%s"), Key, *FTryllDialogCheckJson::FormatFloatR(Value));
	}

	FString Str(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key)
	{
		FString Value;
		Object->TryGetStringField(Key, Value);
		return Value;
	}

	bool Bool(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, bool bDefault)
	{
		bool bValue = bDefault;
		Object->TryGetBoolField(Key, bValue);
		return bValue;
	}

	double Num(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, double Default)
	{
		double Value = Default;
		Object->TryGetNumberField(Key, Value);
		return Value;
	}

	const TCHAR* ScopeName(ETryllLabCheckScope Scope)
	{
		switch (Scope)
		{
			case ETryllLabCheckScope::Global: return TEXT("global");
			case ETryllLabCheckScope::Gate:   return TEXT("gate");
			default:                          return TEXT("step");
		}
	}

	ETryllLabCheckScope ParseScope(const FString& Name)
	{
		if (Name == TEXT("global")) return ETryllLabCheckScope::Global;
		if (Name == TEXT("gate"))   return ETryllLabCheckScope::Gate;
		return ETryllLabCheckScope::Step;
	}
}

FString FTryllDialogCheckJson::FormatFloatR(float Value)
{
	if (FMath::IsNaN(Value))
	{
		return TEXT("NaN");
	}
	// Six decimals with trailing zeros (and a bare point) trimmed. For the plain decimal values
	// a script holds (0.25, 0.7, 1) that is exactly what .NET's "R" writes; FString::Printf
	// takes literal formats only, so the shortest-%g search .NET does is not available here.
	return FString::SanitizeFloat(static_cast<double>(Value), 0);
}

FString FTryllDialogCheckJson::Write(const FTryllLabCheckSpec& Check)
{
	FString Out = FString::Printf(TEXT("{\"type\":%s"), *CheckJsonQuoted(Check.Type));
	Kv(Out, TEXT("enabled"), Check.bEnabled);
	Kv(Out, TEXT("note"), Check.Note);
	const FString& Type = Check.Type;
	if (Type == FTryllLabCheckSpec::TypeMustContain || Type == FTryllLabCheckSpec::TypeMustNotContain)
	{
		Kv(Out, TEXT("text"), Check.Text);
		Kv(Out, TEXT("case_sensitive"), Check.bCaseSensitive);
	}
	else if (Type == FTryllLabCheckSpec::TypeRegexForbid || Type == FTryllLabCheckSpec::TypeRegexMatch)
	{
		Kv(Out, TEXT("pattern"), Check.Pattern);
	}
	else if (Type == FTryllLabCheckSpec::TypeWordCount)
	{
		Kv(Out, TEXT("min"), Check.MinWords);
		Kv(Out, TEXT("max"), Check.MaxWords);
	}
	else if (Type == FTryllLabCheckSpec::TypeIntentIs)
	{
		Kv(Out, TEXT("node"), Check.Node);
		Kv(Out, TEXT("intent"), Check.Intent);
	}
	else if (Type == FTryllLabCheckSpec::TypeIntentNotFound)
	{
		Kv(Out, TEXT("node"), Check.Node);
	}
	else if (Type == FTryllLabCheckSpec::TypeAttributeEquals)
	{
		Kv(Out, TEXT("node"), Check.Node);
		Kv(Out, TEXT("name"), Check.Name);
		Kv(Out, TEXT("value"), Check.Value);
	}
	else if (Type == FTryllLabCheckSpec::TypeAttributeInRange)
	{
		Kv(Out, TEXT("node"), Check.Node);
		Kv(Out, TEXT("name"), Check.Name);
		KvFloat(Out, TEXT("min"), Check.MinScore);
		KvFloat(Out, TEXT("max"), Check.MaxScore);
	}
	else if (Type == FTryllLabCheckSpec::TypeCallsTool)
	{
		Kv(Out, TEXT("tool_name"), Check.ToolName);
	}
	return Out + TEXT("}");
}

FString FTryllDialogCheckJson::WriteList(const TArray<FTryllLabCheckSpec>& Checks)
{
	TArray<FString> Items;
	for (const FTryllLabCheckSpec& Check : Checks)
	{
		Items.Add(Write(Check));
	}
	return TEXT("[") + FString::Join(Items, TEXT(",")) + TEXT("]");
}

bool FTryllDialogCheckJson::Read(
	const TSharedPtr<FJsonObject>& Object, FTryllLabCheckSpec& OutCheck, FString& OutError)
{
	OutCheck = FTryllLabCheckSpec();
	if (!Object.IsValid())
	{
		OutError = TEXT("check is not an object");
		return false;
	}
	const FString Type = Str(Object, TEXT("type"));
	if (!FTryllLabCheckSpec::IsKnownType(Type))
	{
		OutError = FString::Printf(TEXT("unknown check type \"%s\""), *Type);
		return false;
	}
	OutCheck.Type     = Type;
	OutCheck.bEnabled = Bool(Object, TEXT("enabled"), true);
	OutCheck.Note     = Str(Object, TEXT("note"));
	OutCheck.Text     = Str(Object, TEXT("text"));
	OutCheck.bCaseSensitive = Bool(Object, TEXT("case_sensitive"), false);
	OutCheck.Pattern  = Str(Object, TEXT("pattern"));
	OutCheck.Node     = Str(Object, TEXT("node"));
	OutCheck.Intent   = Str(Object, TEXT("intent"));
	OutCheck.Name     = Str(Object, TEXT("name"));
	OutCheck.Value    = Str(Object, TEXT("value"));
	OutCheck.ToolName = Str(Object, TEXT("tool_name"));
	if (Type == FTryllLabCheckSpec::TypeWordCount)
	{
		OutCheck.MinWords = static_cast<int32>(Num(Object, TEXT("min"), 0));
		OutCheck.MaxWords = static_cast<int32>(Num(Object, TEXT("max"), 0));
	}
	else if (Type == FTryllLabCheckSpec::TypeAttributeInRange)
	{
		OutCheck.MinScore = static_cast<float>(Num(Object, TEXT("min"), 0));
		OutCheck.MaxScore = static_cast<float>(Num(Object, TEXT("max"), 1));
	}
	return true;
}

bool FTryllDialogCheckJson::TryReadList(
	const TArray<TSharedPtr<FJsonValue>>& Values, TArray<FTryllLabCheckSpec>& Out, FString& OutError)
{
	for (const TSharedPtr<FJsonValue>& Value : Values)
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		FTryllLabCheckSpec Check;
		if (!Value.IsValid() || !Value->TryGetObject(Object) || Object == nullptr
			|| !Read(*Object, Check, OutError))
		{
			if (OutError.IsEmpty())
			{
				OutError = TEXT("check is not an object");
			}
			return false;
		}
		Out.Add(MoveTemp(Check));
	}
	return true;
}

FString FTryllDialogCheckJson::WriteOutcomes(const TArray<FTryllLabCheckOutcome>& Outcomes)
{
	TArray<FString> Items;
	for (const FTryllLabCheckOutcome& O : Outcomes)
	{
		FString Item = FString::Printf(TEXT("{\"scope\":\"%s\""), ScopeName(O.Scope));
		Kv(Item, TEXT("index"), O.Index);
		Kv(Item, TEXT("type"), O.CheckType);
		Kv(Item, TEXT("kind"), O.Kind);
		Kv(Item, TEXT("passed"), O.bPassed);
		Kv(Item, TEXT("arg"), O.Arg);
		if (!O.Actual.IsEmpty())
		{
			Kv(Item, TEXT("actual"), O.Actual);
		}
		Kv(Item, TEXT("label"), O.Label);
		Items.Add(Item + TEXT("}"));
	}
	return TEXT("[") + FString::Join(Items, TEXT(",")) + TEXT("]");
}

TArray<FTryllLabCheckOutcome> FTryllDialogCheckJson::ReadOutcomes(const TArray<TSharedPtr<FJsonValue>>& Values)
{
	TArray<FTryllLabCheckOutcome> Out;
	for (const TSharedPtr<FJsonValue>& Value : Values)
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		if (!Value.IsValid() || !Value->TryGetObject(Object) || Object == nullptr)
		{
			continue;
		}
		FTryllLabCheckOutcome O;
		FString ScopeText = TEXT("step");
		(*Object)->TryGetStringField(TEXT("scope"), ScopeText);
		O.Scope     = ParseScope(ScopeText);
		O.Index     = static_cast<int32>(Num(*Object, TEXT("index"), -1));
		O.CheckType = Str(*Object, TEXT("type"));
		O.Kind      = Str(*Object, TEXT("kind"));
		O.bPassed   = Bool(*Object, TEXT("passed"), false);
		O.Arg       = Str(*Object, TEXT("arg"));
		O.Actual    = Str(*Object, TEXT("actual"));
		O.Label     = Str(*Object, TEXT("label"));
		Out.Add(MoveTemp(O));
	}
	return Out;
}
