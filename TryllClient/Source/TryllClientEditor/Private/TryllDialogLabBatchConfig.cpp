// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllDialogLabBatchConfig.h"

#include "TryllDialogLabStatistics.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	const TCHAR* const BatchTopKeys[] =
		{ TEXT("format"), TEXT("agent"), TEXT("dialogues"), TEXT("seeds"), TEXT("base_seed"), TEXT("output_dir"),
		  TEXT("turn_timeout_s"), TEXT("settings"), TEXT("thresholds") };
	const TCHAR* const BatchAgentKeys[] =
		{ TEXT("workflow"), TEXT("name"), TEXT("variables"), TEXT("maintain_dialogue_history"), TEXT("model") };
	const TCHAR* const BatchThresholdKeys[] =
		{ TEXT("turns_passed"), TEXT("checks_passed"), TEXT("clean_dialogues"), TEXT("use_lower_bound") };

	/** The member, or null when absent or JSON null (both mean "not given"). */
	TSharedPtr<FJsonValue> BatchField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key)
	{
		const TSharedPtr<FJsonValue>* Value = Object->Values.Find(Key);
		return Value != nullptr && Value->IsValid() && (*Value)->Type != EJson::Null ? *Value : nullptr;
	}

	template <int32 N>
	bool BatchOnlyKeys(const TSharedPtr<FJsonObject>& Object, const TCHAR* const (&Allowed)[N],
		const TCHAR* Prefix, FString& OutError)
	{
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Member : Object->Values)
		{
			bool bKnown = false;
			for (const TCHAR* Key : Allowed)
			{
				bKnown |= Member.Key.Equals(Key, ESearchCase::CaseSensitive);
			}
			if (!bKnown)
			{
				OutError = FString::Printf(TEXT("Unknown key '%s%s'."), Prefix, *Member.Key);
				return false;
			}
		}
		return true;
	}

	bool BatchString(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, const TCHAR* Where,
		bool bRequired, FString& Field, FString& OutError)
	{
		const TSharedPtr<FJsonValue> Value = BatchField(Object, Key);
		if (!Value.IsValid())
		{
			if (bRequired)
			{
				OutError = FString::Printf(TEXT("%s is required."), Where);
			}
			return !bRequired;
		}
		if (Value->Type != EJson::String || (bRequired && Value->AsString().TrimStartAndEnd().IsEmpty()))
		{
			OutError = bRequired
				? FString::Printf(TEXT("%s must be a non-empty string."), Where)
				: FString::Printf(TEXT("%s must be a string."), Where);
			return false;
		}
		Field = Value->AsString();
		return true;
	}

	bool BatchInt(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, const TCHAR* Where,
		bool bRequired, int32 Min, int32& Field, FString& OutError)
	{
		const TSharedPtr<FJsonValue> Value = BatchField(Object, Key);
		if (!Value.IsValid())
		{
			if (bRequired)
			{
				OutError = FString::Printf(TEXT("%s is required."), Where);
			}
			return !bRequired;
		}
		const double Number = Value->Type == EJson::Number ? Value->AsNumber() : -1.0;
		if (Value->Type != EJson::Number || Number != FMath::FloorToDouble(Number) || Number < Min
			|| Number > static_cast<double>(MAX_int32))
		{
			OutError = FString::Printf(TEXT("%s must be a whole number, %d or more."), Where, Min);
			return false;
		}
		Field = static_cast<int32>(Number);
		return true;
	}

	bool BatchBool(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, const TCHAR* Where,
		bool& Field, FString& OutError)
	{
		const TSharedPtr<FJsonValue> Value = BatchField(Object, Key);
		if (!Value.IsValid())
		{
			return true;
		}
		if (Value->Type != EJson::Boolean)
		{
			OutError = FString::Printf(TEXT("%s must be true or false."), Where);
			return false;
		}
		Field = Value->AsBool();
		return true;
	}

	bool BatchRate(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, TOptional<double>& Field, FString& OutError)
	{
		const TSharedPtr<FJsonValue> Value = BatchField(Object, Key);
		if (!Value.IsValid())
		{
			return true;
		}
		if (Value->Type != EJson::Number || Value->AsNumber() < 0.0 || Value->AsNumber() > 1.0)
		{
			OutError = FString::Printf(TEXT("thresholds.%s must be a rate from 0 to 1, or null."), Key);
			return false;
		}
		Field = Value->AsNumber();
		return true;
	}

	const TCHAR* BatchTypeWord(const ETryllVariableType Type)
	{
		switch (Type)
		{
			case ETryllVariableType::Int:       return TEXT("a whole number");
			case ETryllVariableType::Float:     return TEXT("a number");
			case ETryllVariableType::String:    return TEXT("a string");
			case ETryllVariableType::Bool:      return TEXT("true or false");
			case ETryllVariableType::StringSet: return TEXT("an array of strings");
			default:                            return TEXT("a value");
		}
	}
}

bool FTryllDialogLabBatchConfig::TryParse(const FString& Text, FTryllDialogLabBatchConfig& OutConfig, FString& OutError)
{
	TSharedPtr<FJsonValue> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		OutError = FString::Printf(TEXT("Config is not valid JSON: %s"), *Reader->GetErrorMessage());
		return false;
	}
	if (Root->Type != EJson::Object)
	{
		OutError = TEXT("Config must be a JSON object.");
		return false;
	}
	const TSharedPtr<FJsonObject> Top = Root->AsObject();
	if (!BatchOnlyKeys(Top, BatchTopKeys, TEXT(""), OutError))
	{
		return false;
	}

	const TSharedPtr<FJsonValue> FormatValue = BatchField(Top, TEXT("format"));
	if (!FormatValue.IsValid() || FormatValue->Type != EJson::Number || FormatValue->AsNumber() != Format)
	{
		OutError = FString::Printf(TEXT("format must be %d."), Format);
		return false;
	}

	FTryllDialogLabBatchConfig C;

	const TSharedPtr<FJsonValue> AgentValue = BatchField(Top, TEXT("agent"));
	if (!AgentValue.IsValid() || AgentValue->Type != EJson::Object)
	{
		OutError = TEXT("agent must be an object with a workflow.");
		return false;
	}
	const TSharedPtr<FJsonObject> Agent = AgentValue->AsObject();
	if (!BatchOnlyKeys(Agent, BatchAgentKeys, TEXT("agent."), OutError)
		|| !BatchString(Agent, TEXT("workflow"), TEXT("agent.workflow"), true, C.Workflow, OutError)
		|| !BatchString(Agent, TEXT("name"), TEXT("agent.name"), false, C.AgentName, OutError)
		|| !BatchString(Agent, TEXT("model"), TEXT("agent.model"), false, C.Model, OutError)
		|| !BatchBool(Agent, TEXT("maintain_dialogue_history"), TEXT("agent.maintain_dialogue_history"),
			C.bMaintainDialogueHistory, OutError))
	{
		return false;
	}
	if (const TSharedPtr<FJsonValue> Vars = BatchField(Agent, TEXT("variables")))
	{
		if (Vars->Type != EJson::Object)
		{
			OutError = TEXT("agent.variables must be an object.");
			return false;
		}
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Member : Vars->AsObject()->Values)
		{
			C.Variables.Emplace(Member.Key, Member.Value);
		}
	}

	if (!BatchString(Top, TEXT("dialogues"), TEXT("dialogues"), true, C.Dialogues, OutError)
		|| !BatchInt(Top, TEXT("seeds"), TEXT("seeds"), true, 1, C.Seeds, OutError)
		|| !BatchInt(Top, TEXT("base_seed"), TEXT("base_seed"), false, 1, C.BaseSeed, OutError)
		|| !BatchString(Top, TEXT("output_dir"), TEXT("output_dir"), false, C.OutputDir, OutError)
		|| !BatchString(Top, TEXT("settings"), TEXT("settings"), false, C.Settings, OutError))
	{
		return false;
	}
	if (const TSharedPtr<FJsonValue> Timeout = BatchField(Top, TEXT("turn_timeout_s")))
	{
		if (Timeout->Type != EJson::Number || Timeout->AsNumber() < 0.0)
		{
			OutError = TEXT("turn_timeout_s must be a number of seconds, 0 or more (0 = no timeout).");
			return false;
		}
		C.TurnTimeoutSeconds = Timeout->AsNumber();
	}

	if (const TSharedPtr<FJsonValue> ThresholdsValue = BatchField(Top, TEXT("thresholds")))
	{
		if (ThresholdsValue->Type != EJson::Object)
		{
			OutError = TEXT("thresholds must be an object.");
			return false;
		}
		const TSharedPtr<FJsonObject> Thresholds = ThresholdsValue->AsObject();
		if (!BatchOnlyKeys(Thresholds, BatchThresholdKeys, TEXT("thresholds."), OutError)
			|| !BatchRate(Thresholds, TEXT("turns_passed"), C.TurnsPassed, OutError)
			|| !BatchRate(Thresholds, TEXT("checks_passed"), C.ChecksPassed, OutError)
			|| !BatchRate(Thresholds, TEXT("clean_dialogues"), C.CleanDialogues, OutError)
			|| !BatchBool(Thresholds, TEXT("use_lower_bound"), TEXT("thresholds.use_lower_bound"),
				C.bUseLowerBound, OutError))
		{
			return false;
		}
	}

	// Intervals start at MinSeedsForInterval seeds (the Lab and the stats block show none below
	// it). Gating on one there would fail on a bound the report leaves out, or, at one seed,
	// quietly gate on the plain rate.
	if (C.bUseLowerBound && C.Seeds < FTryllDialogLabStatistics::MinSeedsForInterval)
	{
		OutError = LowerBoundSeedsError(C.Seeds);
		return false;
	}

	OutConfig = MoveTemp(C);
	OutError.Reset();
	return true;
}

FString FTryllDialogLabBatchConfig::LowerBoundSeedsError(const int32 Seeds, const TCHAR* What)
{
	return FString::Printf(
		TEXT("thresholds.use_lower_bound needs at least %d seeds (intervals start there); this %s has %d."),
		FTryllDialogLabStatistics::MinSeedsForInterval, What, Seeds);
}

bool FTryllDialogLabBatchConfig::TryBuildOverrides(const TArray<FTryllVariableDecl>& Declared,
	TArray<FTryllVariableOverride>& OutOverrides, FString& OutError) const
{
	OutOverrides.Reset();
	for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Variables)
	{
		const FString Where = FString::Printf(TEXT("agent.variables.%s"), *Entry.Key);
		const FTryllVariableDecl* Decl = Declared.FindByPredicate(
			[&Entry](const FTryllVariableDecl& D) { return D.Name == Entry.Key; });
		if (Decl == nullptr)
		{
			OutError = FString::Printf(TEXT("%s: the workflow declares no variable '%s'."), *Where, *Entry.Key);
			return false;
		}
		FTryllVariableDecl Value;
		Value.Name = Decl->Name;
		Value.Type = Decl->Type;
		const TSharedPtr<FJsonValue>& V = Entry.Value;
		const EJson Kind = V.IsValid() ? V->Type : EJson::Null;
		bool bOk = false;
		switch (Decl->Type)
		{
			case ETryllVariableType::Int:
				bOk = Kind == EJson::Number && V->AsNumber() == FMath::FloorToDouble(V->AsNumber());
				if (bOk) { Value.IntValue = static_cast<int64>(V->AsNumber()); }
				break;
			case ETryllVariableType::Float:
				bOk = Kind == EJson::Number;
				if (bOk) { Value.FloatValue = V->AsNumber(); }
				break;
			case ETryllVariableType::String:
				bOk = Kind == EJson::String;
				if (bOk) { Value.StringValue = V->AsString(); }
				break;
			case ETryllVariableType::Bool:
				bOk = Kind == EJson::Boolean;
				if (bOk) { Value.bBoolValue = V->AsBool(); }
				break;
			case ETryllVariableType::StringSet:
				bOk = Kind == EJson::Array;
				if (bOk)
				{
					for (const TSharedPtr<FJsonValue>& Item : V->AsArray())
					{
						bOk &= Item.IsValid() && Item->Type == EJson::String;
						if (bOk) { Value.StringSetValue.Add(Item->AsString()); }
					}
				}
				break;
			default:
				break;
		}
		if (!bOk)
		{
			OutError = FString::Printf(TEXT("%s: expected %s."), *Where, BatchTypeWord(Decl->Type));
			return false;
		}
		FTryllVariableOverride Override;
		Override.Name     = Decl->Name;
		Override.bEnabled = true;
		Override.Value    = Value;
		OutOverrides.Add(MoveTemp(Override));
	}
	OutError.Reset();
	return true;
}
