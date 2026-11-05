// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllDialogLabChecks.h"

#include "TryllDialogLabIntent.h"
#include "TryllDialogLabScriptData.h"
#include "TryllTurnInspectorModels.h"
#include "Containers/Set.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Internationalization/Regex.h"

const TCHAR* FTryllDialogLabChecks::KindMustContain         = TEXT("must_contain");
const TCHAR* FTryllDialogLabChecks::KindMustNotContain      = TEXT("must_not_contain");
const TCHAR* FTryllDialogLabChecks::KindRegexForbid         = TEXT("regex_forbid");
const TCHAR* FTryllDialogLabChecks::KindRegexMatch          = TEXT("regex_match");
const TCHAR* FTryllDialogLabChecks::KindWordCount           = TEXT("word_count");
const TCHAR* FTryllDialogLabChecks::KindMaxWords            = TEXT("max_words");
const TCHAR* FTryllDialogLabChecks::KindInvalidRegex        = TEXT("invalid_regex");
const TCHAR* FTryllDialogLabChecks::KindIntentIs            = TEXT("intent_is");
const TCHAR* FTryllDialogLabChecks::KindIntentNotFound      = TEXT("intent_not_found");
const TCHAR* FTryllDialogLabChecks::KindIntentUnavailable   = TEXT("intent_unavailable");
const TCHAR* FTryllDialogLabChecks::KindAttributeIs         = TEXT("attribute_is");
const TCHAR* FTryllDialogLabChecks::KindAttributeScore      = TEXT("attribute_score");
const TCHAR* FTryllDialogLabChecks::KindAttributeUnavailable = TEXT("attribute_unavailable");
const TCHAR* FTryllDialogLabChecks::KindToolCall            = TEXT("tool_call");
const TCHAR* FTryllDialogLabChecks::ToolCallOtherArg        = TEXT("*");
const TCHAR* FTryllDialogLabChecks::GateUnexpectedToolCall  = TEXT("unexpected_tool_call");

namespace
{
	FString AbsenceName(ETryllDiagnosticsAbsenceReason Absence)
	{
		switch (Absence)
		{
			case ETryllDiagnosticsAbsenceReason::Seeded:                         return TEXT("Seeded");
			case ETryllDiagnosticsAbsenceReason::Skipped:                        return TEXT("Skipped");
			case ETryllDiagnosticsAbsenceReason::Unavailable:                    return TEXT("Unavailable");
			case ETryllDiagnosticsAbsenceReason::CreatedBeforeDiagnosticsCapture: return TEXT("CreatedBeforeDiagnosticsCapture");
			case ETryllDiagnosticsAbsenceReason::UnsupportedSchema:              return TEXT("UnsupportedSchema");
			case ETryllDiagnosticsAbsenceReason::Empty:                          return TEXT("Empty");
			default:                                                             return TEXT("None");
		}
	}

	FString AbsenceActual(ETryllDiagnosticsAbsenceReason Absence)
	{
		return Absence == ETryllDiagnosticsAbsenceReason::None
			? FString(TEXT("diagnostics absent"))
			: FString(TEXT("diagnostics absent: ")) + AbsenceName(Absence);
	}

	FString IntentIsActual(const FTryllIntentExecution* Execution, const FString& /*Expected*/)
	{
		if (Execution == nullptr)
		{
			return TEXT("got nothing");
		}
		if (!Execution->bAccepted)
		{
			const FString Rejected = FTryllDialogLabIntent::ResolveLabel(*Execution);
			const FString Reason = Execution->NotFoundReason.IsEmpty()
				? FString(TEXT("not_found")) : Execution->NotFoundReason;
			if (Rejected.IsEmpty())
			{
				return FString(TEXT("got not_found: ")) + Reason;
			}
			FString Out = FString(TEXT("got not_found: ")) + Reason + TEXT(", top-1 \"") + Rejected + TEXT("\"");
			if (Execution->Distance.IsSet())
			{
				Out += TEXT(" @ ") + FTryllDialogLabIntent::FormatDistance(Execution->Distance);
			}
			return Out;
		}
		const FString Got = FTryllDialogLabIntent::ResolveLabel(*Execution);
		return FString(TEXT("got ")) + (Got.IsEmpty() ? FString(TEXT("found")) : Got);
	}

	bool TryAddress(
		const TSharedPtr<FJsonObject>& DebugInfo,
		const FString& Node,
		TArray<FTryllIntentExecution>& OutAddressed,
		FString& OutUnavailableActual)
	{
		OutAddressed.Reset();
		OutUnavailableActual.Reset();
		if (!DebugInfo.IsValid())
		{
			OutUnavailableActual = TEXT("no classifier ran this turn");
			return false;
		}

		const TArray<TSharedPtr<FJsonValue>>* Nodes = nullptr;
		const TArray<TSharedPtr<FJsonValue>> Empty;
		const TArray<TSharedPtr<FJsonValue>>& NodeValues =
			DebugInfo->TryGetArrayField(TEXT("nodes"), Nodes) && Nodes != nullptr ? *Nodes : Empty;

		if (Node.Len() > 0)
		{
			bool bAny = false;
			for (int32 Index = 0; Index < NodeValues.Num(); ++Index)
			{
				const TSharedPtr<FJsonObject>* Object = nullptr;
				if (!NodeValues[Index].IsValid() || !NodeValues[Index]->TryGetObject(Object)
					|| Object == nullptr)
				{
					continue;
				}
				FString Name;
				(*Object)->TryGetStringField(TEXT("name"), Name);
				if (Name != Node)
				{
					continue;
				}
				bAny = true;
				FString Type;
				(*Object)->TryGetStringField(TEXT("_type"), Type);
				if (!FTryllDialogLabIntent::IsClassifierType(Type))
				{
					OutUnavailableActual = FString(TEXT("node \"")) + Node + TEXT("\" is a ")
						+ (Type.IsEmpty() ? FString(TEXT("unknown node")) : Type);
					OutAddressed.Reset();
					return false;
				}
				OutAddressed.Add(FTryllDialogLabIntent::ParseOne(Index, *Object));
			}
			if (!bAny)
			{
				OutUnavailableActual = TEXT("no classifier ran this turn");
				return false;
			}
			return true;
		}

		for (int32 Index = 0; Index < NodeValues.Num(); ++Index)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			if (!NodeValues[Index].IsValid() || !NodeValues[Index]->TryGetObject(Object)
				|| Object == nullptr)
			{
				continue;
			}
			FString Type;
			(*Object)->TryGetStringField(TEXT("_type"), Type);
			if (!FTryllDialogLabIntent::IsClassifierType(Type))
			{
				continue;
			}
			OutAddressed.Add(FTryllDialogLabIntent::ParseOne(Index, *Object));
		}
		if (OutAddressed.Num() == 0)
		{
			OutUnavailableActual = TEXT("no classifier ran this turn");
			return false;
		}
		return true;
	}

	// Diagnostics type `value` by kind: a string for Choice, a bool for Boolean,
	// a number for Score. Render each the way the attribute's slot does.
	FString AttributeValueText(const TSharedPtr<FJsonValue>& Value)
	{
		if (!Value.IsValid())
		{
			return FString();
		}
		switch (Value->Type)
		{
		case EJson::String:
			return Value->AsString();
		case EJson::Boolean:
			return Value->AsBool() ? TEXT("true") : TEXT("false");
		case EJson::Number:
			return FString::Printf(TEXT("%.4f"), Value->AsNumber());
		default:
			return FString();
		}
	}

	bool TryFindAttribute(
		const TSharedPtr<FJsonObject>& DebugInfo,
		const FString& NodeName,
		const FString& AttributeName,
		FString& OutValue,
		FString& OutUnavailable)
	{
		OutValue.Reset();
		OutUnavailable = FString::Printf(TEXT("attribute \"%s\" did not run"), *AttributeName);
		if (!DebugInfo.IsValid())
		{
			return false;
		}
		const TArray<TSharedPtr<FJsonValue>>* Nodes = nullptr;
		if (!DebugInfo->TryGetArrayField(TEXT("nodes"), Nodes) || Nodes == nullptr)
		{
			return false;
		}
		bool bSawNode = false;
		for (const TSharedPtr<FJsonValue>& Entry : *Nodes)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			if (!Entry.IsValid() || !Entry->TryGetObject(Object) || Object == nullptr)
			{
				continue;
			}
			FString Name;
			(*Object)->TryGetStringField(TEXT("name"), Name);
			if (!NodeName.IsEmpty() && Name != NodeName)
			{
				continue;
			}
			FString Type;
			(*Object)->TryGetStringField(TEXT("_type"), Type);
			if (!NodeName.IsEmpty() && Type != TEXT("ClassifyAttributesLLMNode"))
			{
				OutUnavailable = FString::Printf(
					TEXT("node \"%s\" is a %s"), *NodeName,
					Type.IsEmpty() ? TEXT("unknown node") : *Type);
				return false;
			}
			if (Type != TEXT("ClassifyAttributesLLMNode"))
			{
				continue;
			}
			bSawNode = true;
			const TSharedPtr<FJsonObject>* Diagnostics = nullptr;
			const TSharedPtr<FJsonObject>* Output = nullptr;
			const TSharedPtr<FJsonObject>* Classification = nullptr;
			if (!(*Object)->TryGetObjectField(TEXT("diagnostics"), Diagnostics) || Diagnostics == nullptr
				|| !(*Diagnostics)->TryGetObjectField(TEXT("output"), Output) || Output == nullptr
				|| !(*Output)->TryGetObjectField(TEXT("classification"), Classification)
				|| Classification == nullptr)
			{
				continue;
			}
			const TArray<TSharedPtr<FJsonValue>>* Attributes = nullptr;
			if (!(*Classification)->TryGetArrayField(TEXT("attributes"), Attributes) || Attributes == nullptr)
			{
				continue;
			}
			for (const TSharedPtr<FJsonValue>& AttributeEntry : *Attributes)
			{
				const TSharedPtr<FJsonObject>* Attribute = nullptr;
				if (!AttributeEntry.IsValid() || !AttributeEntry->TryGetObject(Attribute) || Attribute == nullptr)
				{
					continue;
				}
				FString FoundName;
				(*Attribute)->TryGetStringField(TEXT("name"), FoundName);
				if (FoundName != AttributeName)
				{
					continue;
				}
				OutValue = AttributeValueText((*Attribute)->TryGetField(TEXT("value")));
				return true;
			}
		}
		if (!bSawNode && !NodeName.IsEmpty())
		{
			OutUnavailable = FString::Printf(TEXT("node \"%s\" did not run"), *NodeName);
		}
		return false;
	}

	TArray<FString> ActualToolNames(const TSharedPtr<FJsonObject>& DebugInfo)
	{
		TArray<FString> Names;
		if (!DebugInfo.IsValid())
		{
			return Names;
		}
		const TArray<TSharedPtr<FJsonValue>>* Calls = nullptr;
		if (!DebugInfo->TryGetArrayField(TEXT("tool_calls"), Calls) || Calls == nullptr)
		{
			return Names;
		}
		TSet<FString> Seen;
		for (const TSharedPtr<FJsonValue>& Value : *Calls)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			if (!Value.IsValid() || !Value->TryGetObject(Object) || Object == nullptr)
			{
				continue;
			}
			FString Name;
			(*Object)->TryGetStringField(TEXT("name"), Name);
			if (!Name.IsEmpty() && !Seen.Contains(Name))
			{
				Seen.Add(Name);
				Names.Add(Name);
			}
		}
		return Names;
	}

	FTryllLabCheckOutcome Result(
		const FTryllLabCheckSpec& Check, const FString& Kind, const FString& Arg,
		bool bPassed, const FString& Actual = FString())
	{
		FTryllLabCheckOutcome Out;
		Out.CheckType = Check.Type;
		Out.Kind      = Kind;
		Out.Arg       = Arg;
		Out.bPassed   = bPassed;
		Out.Actual    = Actual;
		// A failing check with an author Note keeps the Note and appends what actually
		// happened, so the chip stays recognisable.
		Out.Label = Check.Note.IsEmpty()
			? FTryllDialogLabChecks::LabelFor(Kind, Arg, Actual)
			: (Actual.IsEmpty() ? Check.Note : Check.Note + TEXT(" (") + Actual + TEXT(")"));
		return Out;
	}

	struct FTurnContext
	{
		FString Text;
		FString Lower;
		int32   Words = 0;
		TSharedPtr<FJsonObject> DebugInfo;
		ETryllDiagnosticsAbsenceReason Absence = ETryllDiagnosticsAbsenceReason::None;
		TArray<FString> ActualTools;

		bool HasDiagnostics() const
		{
			return Absence == ETryllDiagnosticsAbsenceReason::None && DebugInfo.IsValid();
		}
	};

	bool ContainsNeedle(const FTurnContext& Ctx, const FString& Needle, bool bCaseSensitive)
	{
		// Unity lowercases both sides and compares ordinally; matching that exactly keeps the
		// same script producing the same verdict on both engines.
		return bCaseSensitive
			? Ctx.Text.Contains(Needle, ESearchCase::CaseSensitive)
			: Ctx.Lower.Contains(Needle.ToLower(), ESearchCase::CaseSensitive);
	}

	FTryllLabCheckOutcome EvaluateRegex(
		const FTryllLabCheckSpec& Check, const FString& Kind, const FString& Arg,
		const FTurnContext& Ctx, bool bForbid)
	{
		if (!FTryllDialogLabChecks::IsPatternValid(Check.Pattern))
		{
			// A malformed pattern fails rather than passing silently, and says why.
			return Result(Check, FTryllDialogLabChecks::KindInvalidRegex, Arg, false);
		}
		const FRegexPattern Compiled(Check.Pattern);
		FRegexMatcher Matcher(Compiled, Ctx.Text);
		const bool bMatch = Matcher.FindNext();
		return Result(Check, Kind, Arg, bForbid ? !bMatch : bMatch);
	}

	FTryllLabCheckOutcome EvaluateIntent(
		const FTryllLabCheckSpec& Check, const FString& Kind, const FString& Arg,
		const FTurnContext& Ctx, bool bExpectNotFound)
	{
		if (!Ctx.HasDiagnostics())
		{
			return Result(Check, FTryllDialogLabChecks::KindIntentUnavailable, Arg, false, AbsenceActual(Ctx.Absence));
		}
		TArray<FTryllIntentExecution> Addressed;
		FString Unavail;
		if (!TryAddress(Ctx.DebugInfo, Check.Node, Addressed, Unavail))
		{
			return Result(Check, FTryllDialogLabChecks::KindIntentUnavailable, Arg, false, Unavail);
		}

		if (bExpectNotFound)
		{
			const FTryllIntentExecution* Accepted = nullptr;
			for (const FTryllIntentExecution& Execution : Addressed)
			{
				if (Execution.bAccepted)
				{
					Accepted = &Execution;
					break;
				}
			}
			if (Accepted == nullptr)
			{
				return Result(Check, Kind, Arg, true);
			}
			const FString Got = FTryllDialogLabIntent::ResolveLabel(*Accepted);
			FString Dist;
			if (Accepted->Distance.IsSet())
			{
				Dist = TEXT(" @ ") + FTryllDialogLabIntent::FormatDistance(Accepted->Distance);
			}
			else if (Accepted->TopProb.IsSet())
			{
				Dist = TEXT(" @ ") + FTryllDialogLabIntent::FormatProb(Accepted->TopProb);
			}
			return Result(Check, Kind, Arg, false,
				FString(TEXT("got ")) + (Got.IsEmpty() ? FString(TEXT("found")) : Got) + Dist);
		}

		const FTryllIntentExecution* Last = nullptr;
		for (const FTryllIntentExecution& Execution : Addressed)
		{
			Last = &Execution;
			if (Execution.bAccepted
				&& FTryllDialogLabIntent::ResolveLabel(Execution).Equals(Check.Intent, ESearchCase::CaseSensitive))
			{
				return Result(Check, Kind, Arg, true);
			}
		}
		return Result(Check, Kind, Arg, false, IntentIsActual(Last, Check.Intent));
	}

	FTryllLabCheckOutcome EvaluateAttribute(
		const FTryllLabCheckSpec& Check, const FString& Kind, const FString& Arg,
		const FTurnContext& Ctx, TFunctionRef<bool(const FString&)> Accept)
	{
		if (!Ctx.HasDiagnostics())
		{
			return Result(Check, FTryllDialogLabChecks::KindAttributeUnavailable, Arg, false, TEXT("diagnostics absent"));
		}
		FString Value;
		FString Unavail;
		if (!TryFindAttribute(Ctx.DebugInfo, Check.Node, Check.Name, Value, Unavail))
		{
			return Result(Check, FTryllDialogLabChecks::KindAttributeUnavailable, Arg, false, Unavail);
		}
		const bool bOk = Accept(Value);
		return Result(Check, Kind, Arg, bOk, bOk ? FString() : FString(TEXT("got ")) + Value);
	}

	FTryllLabCheckOutcome EvaluateOne(const FTryllLabCheckSpec& Check, const FTurnContext& Ctx)
	{
		FString Kind;
		FString Arg;
		FTryllDialogLabChecks::Identity(Check, Kind, Arg);
		const FString& Type = Check.Type;

		if (Type == FTryllLabCheckSpec::TypeMustContain)
		{
			return Result(Check, Kind, Arg, ContainsNeedle(Ctx, Check.Text, Check.bCaseSensitive));
		}
		if (Type == FTryllLabCheckSpec::TypeMustNotContain)
		{
			return Result(Check, Kind, Arg, !ContainsNeedle(Ctx, Check.Text, Check.bCaseSensitive));
		}
		if (Type == FTryllLabCheckSpec::TypeRegexForbid)
		{
			return EvaluateRegex(Check, Kind, Arg, Ctx, true);
		}
		if (Type == FTryllLabCheckSpec::TypeRegexMatch)
		{
			return EvaluateRegex(Check, Kind, Arg, Ctx, false);
		}
		if (Type == FTryllLabCheckSpec::TypeWordCount)
		{
			const bool bOk = (Check.MaxWords <= 0 || Ctx.Words <= Check.MaxWords)
				&& (Check.MinWords <= 0 || Ctx.Words >= Check.MinWords);
			return Result(Check, Kind, Arg, bOk, bOk ? FString() : FString::FromInt(Ctx.Words));
		}
		if (Type == FTryllLabCheckSpec::TypeIntentIs)
		{
			return EvaluateIntent(Check, Kind, Arg, Ctx, false);
		}
		if (Type == FTryllLabCheckSpec::TypeIntentNotFound)
		{
			return EvaluateIntent(Check, Kind, Arg, Ctx, true);
		}
		if (Type == FTryllLabCheckSpec::TypeAttributeEquals)
		{
			return EvaluateAttribute(Check, Kind, Arg, Ctx, [&Check](const FString& Value)
			{
				return Value.Equals(Check.Value, ESearchCase::CaseSensitive);
			});
		}
		if (Type == FTryllLabCheckSpec::TypeAttributeInRange)
		{
			return EvaluateAttribute(Check, Kind, Arg, Ctx, [&Check](const FString& Value)
			{
				if (!Value.IsNumeric())
				{
					return false;
				}
				const float Actual = FCString::Atof(*Value);
				return Actual >= Check.MinScore && Actual <= Check.MaxScore;
			});
		}
		if (Type == FTryllLabCheckSpec::TypeCallsTool)
		{
			const bool bCalled = Ctx.ActualTools.Contains(Check.ToolName);
			return Result(Check, Kind, Arg, bCalled, bCalled ? FString() : FString(TEXT("not called")));
		}
		if (Type == FTryllLabCheckSpec::TypeCallsNoTool)
		{
			const bool bNone = Ctx.ActualTools.Num() == 0;
			return Result(Check, Kind, Arg, bNone,
				bNone ? FString() : FString(TEXT("got ")) + FString::Join(Ctx.ActualTools, TEXT(", ")));
		}
		return Result(Check, Kind, Arg, false, TEXT("unknown check type"));
	}

	TArray<FString> CannedToolNames(const FTryllLabStepSnapshot& Step)
	{
		TArray<FString> Names;
		for (const FTryllCannedToolResult& Canned : Step.ToolResults)
		{
			if (!Canned.ToolName.IsEmpty())
			{
				Names.AddUnique(Canned.ToolName);
			}
		}
		return Names;
	}

	/**
	 * The Lab's built-in assertion on every generated turn: a call to a tool with no canned
	 * result on this step fails, unless the variant uses live tools and the live handler
	 * answered it. Not an authored check, so statistics leave it out of the check denominator,
	 * but it still fails the turn.
	 */
	FTryllLabCheckOutcome EvaluateGate(
		const FTryllLabStepSnapshot& Step, const FTurnContext& Ctx,
		bool bUseLiveTools, const TArray<FString>& MissingResumedTools)
	{
		const TArray<FString> Canned = CannedToolNames(Step);
		TArray<FString> Violations;
		for (const FString& Name : Ctx.ActualTools)
		{
			if (!Canned.Contains(Name) && (!bUseLiveTools || MissingResumedTools.Contains(Name)))
			{
				Violations.Add(Name);
			}
		}
		const FString Arg = Canned.Num() == 0 ? FString() : FString(FTryllDialogLabChecks::ToolCallOtherArg);
		const FString Actual = Violations.Num() == 0
			? FString()
			: (Canned.Num() == 0 ? FString(TEXT("got ")) : FString(TEXT("unexpected ")))
				+ FString::Join(Violations, TEXT(", "));
		FTryllLabCheckOutcome Out;
		Out.Scope     = ETryllLabCheckScope::Gate;
		Out.Index     = 0;
		Out.CheckType = FTryllDialogLabChecks::GateUnexpectedToolCall;
		Out.Kind      = FTryllDialogLabChecks::KindToolCall;
		Out.Arg       = Arg;
		Out.bPassed   = Violations.Num() == 0;
		Out.Actual    = Actual;
		Out.Label     = FTryllDialogLabChecks::LabelFor(Out.Kind, Arg, Actual);
		return Out;
	}

	FTryllLabCheck Row(ETryllLabCheckScope Scope, int32 Index, const FTryllLabCheckSpec& Check)
	{
		FTryllLabCheck Out;
		FTryllDialogLabChecks::Identity(Check, Out.Kind, Out.Arg);
		Out.Label = FTryllDialogLabChecks::AuthoredLabel(Check);
		Out.Scope = Scope;
		Out.Index = Index;
		return Out;
	}
}

FString FTryllDialogLabChecks::LabelFor(const FString& Kind, const FString& Arg, const FString& Actual)
{
	if (Kind == KindMustContain)
	{
		return FString::Printf(TEXT("must contain \"%s\""), *Arg);
	}
	if (Kind == KindMustNotContain)
	{
		return FString::Printf(TEXT("must not contain \"%s\""), *Arg);
	}
	if (Kind == KindRegexForbid)
	{
		return FString::Printf(TEXT("forbidden /%s/"), *Arg);
	}
	if (Kind == KindRegexMatch)
	{
		return FString::Printf(TEXT("must match /%s/"), *Arg);
	}
	if (Kind == KindWordCount)
	{
		TArray<FString> Parts;
		Arg.ParseIntoArray(Parts, TEXT("|"), false);
		const int32 Min = Parts.Num() > 0 ? FCString::Atoi(*Parts[0]) : 0;
		const int32 Max = Parts.Num() > 1 ? FCString::Atoi(*Parts[1]) : 0;
		const FString Range = Min > 0 && Max > 0 ? FString::Printf(TEXT("%d–%d words"), Min, Max)
			: Max > 0 ? FString::Printf(TEXT("≤ %d words"), Max)
			: FString::Printf(TEXT("≥ %d words"), Min);
		return Actual.IsEmpty() ? Range : FString::Printf(TEXT("%s (got %s)"), *Range, *Actual);
	}
	if (Kind == KindInvalidRegex)
	{
		return FString::Printf(TEXT("invalid regex /%s/"), *Arg);
	}
	if (Kind == KindMaxWords)
	{
		return Actual.IsEmpty()
			? FString::Printf(TEXT("≤ %s words"), *Arg)
			: FString::Printf(TEXT("≤ %s words (got %s)"), *Arg, *Actual);
	}
	if (Kind == KindIntentIs)
	{
		FString Node;
		FString Intent;
		TryParseIntentArg(Arg, Node, Intent);
		const FString BaseLabel = Node.IsEmpty()
			? FString::Printf(TEXT("intent is \"%s\""), *Intent)
			: FString::Printf(TEXT("%s: intent is \"%s\""), *Node, *Intent);
		return Actual.IsEmpty() ? BaseLabel : BaseLabel + TEXT(" (") + Actual + TEXT(")");
	}
	if (Kind == KindIntentNotFound)
	{
		const FString Node = Arg;
		const FString BaseLabel = Node.IsEmpty()
			? FString(TEXT("intent not found"))
			: Node + TEXT(": intent not found");
		return Actual.IsEmpty() ? BaseLabel : BaseLabel + TEXT(" (") + Actual + TEXT(")");
	}
	if (Kind == KindIntentUnavailable)
	{
		return Actual.IsEmpty()
			? FString(TEXT("intent unavailable"))
			: FString(TEXT("intent unavailable (")) + Actual + TEXT(")");
	}
	if (Kind == KindAttributeIs || Kind == KindAttributeScore)
	{
		TArray<FString> Parts;
		Arg.ParseIntoArray(Parts, TEXT("|"), false);
		const FString Node = Parts.Num() > 0 ? Parts[0] : FString();
		const FString Name = Parts.Num() > 1 ? Parts[1] : FString();
		FString Rest;
		for (int32 Index = 2; Index < Parts.Num(); ++Index)
		{
			if (!Rest.IsEmpty()) Rest += TEXT("|");
			Rest += Parts[Index];
		}
		const FString Who = Node.IsEmpty() ? Name : Node + TEXT(": ") + Name;
		const FString BaseLabel = Kind == KindAttributeScore
			? Who + TEXT(" in ") + Rest
			: FString::Printf(TEXT("%s is \"%s\""), *Who, *Rest);
		return Actual.IsEmpty() ? BaseLabel : BaseLabel + TEXT(" (") + Actual + TEXT(")");
	}
	if (Kind == KindAttributeUnavailable)
	{
		return Actual.IsEmpty()
			? FString(TEXT("attribute unavailable"))
			: FString(TEXT("attribute unavailable (")) + Actual + TEXT(")");
	}
	if (Kind == KindToolCall)
	{
		const FString BaseLabel = Arg == ToolCallOtherArg
			? FString(TEXT("must not call other tools"))
			: Arg.IsEmpty()
				? FString(TEXT("must not call a tool"))
				: FString(TEXT("must call ")) + Arg;
		return Actual.IsEmpty() ? BaseLabel : BaseLabel + TEXT(" (") + Actual + TEXT(")");
	}
	return Kind;
}

FString FTryllDialogLabChecks::AttributeArg(
	const FString& Node, const FString& Name, const FString& Value)
{
	return Node + TEXT("|") + Name + TEXT("|") + Value;
}

FString FTryllDialogLabChecks::AttributeScoreArg(
	const FString& Node, const FString& Name, float Min, float Max)
{
	return FString::Printf(TEXT("%s|%s|%.4f|%.4f"), *Node, *Name, Min, Max);
}

FString FTryllDialogLabChecks::IntentArg(const FString& Node, const FString& Intent)
{
	return Node.IsEmpty() ? Intent : Node + TEXT("|") + Intent;
}

bool FTryllDialogLabChecks::TryParseIntentArg(const FString& Arg, FString& OutNode, FString& OutIntent)
{
	OutNode.Reset();
	OutIntent = Arg;
	int32 Bar = INDEX_NONE;
	if (Arg.FindChar(TEXT('|'), Bar))
	{
		OutNode   = Arg.Left(Bar);
		OutIntent = Arg.Mid(Bar + 1);
	}
	return true;
}

bool FTryllDialogLabChecks::IsPatternValid(const FString& Pattern)
{
	const FRegexPattern Probe(FString::Printf(TEXT("(?:%s)|(?:)"), *Pattern));
	FRegexMatcher Matcher(Probe, TEXT("x"));
	return Matcher.FindNext();
}

FString FTryllDialogLabChecks::WordCountArg(int32 Min, int32 Max)
{
	return FString::Printf(TEXT("%d|%d"), Min, Max);
}

void FTryllDialogLabChecks::Identity(const FTryllLabCheckSpec& Check, FString& OutKind, FString& OutArg)
{
	const FString& Type = Check.Type;
	if (Type == FTryllLabCheckSpec::TypeMustContain)          { OutKind = KindMustContain;    OutArg = Check.Text; }
	else if (Type == FTryllLabCheckSpec::TypeMustNotContain)  { OutKind = KindMustNotContain; OutArg = Check.Text; }
	else if (Type == FTryllLabCheckSpec::TypeRegexForbid)     { OutKind = KindRegexForbid;    OutArg = Check.Pattern; }
	else if (Type == FTryllLabCheckSpec::TypeRegexMatch)      { OutKind = KindRegexMatch;     OutArg = Check.Pattern; }
	else if (Type == FTryllLabCheckSpec::TypeWordCount)       { OutKind = KindWordCount;      OutArg = WordCountArg(Check.MinWords, Check.MaxWords); }
	else if (Type == FTryllLabCheckSpec::TypeIntentIs)        { OutKind = KindIntentIs;       OutArg = IntentArg(Check.Node, Check.Intent); }
	else if (Type == FTryllLabCheckSpec::TypeIntentNotFound)  { OutKind = KindIntentNotFound; OutArg = Check.Node; }
	else if (Type == FTryllLabCheckSpec::TypeAttributeEquals) { OutKind = KindAttributeIs;    OutArg = AttributeArg(Check.Node, Check.Name, Check.Value); }
	else if (Type == FTryllLabCheckSpec::TypeAttributeInRange){ OutKind = KindAttributeScore; OutArg = AttributeScoreArg(Check.Node, Check.Name, Check.MinScore, Check.MaxScore); }
	else if (Type == FTryllLabCheckSpec::TypeCallsTool)       { OutKind = KindToolCall;       OutArg = Check.ToolName; }
	else if (Type == FTryllLabCheckSpec::TypeCallsNoTool)     { OutKind = KindToolCall;       OutArg = FString(); }
	else                                                      { OutKind = Type;               OutArg = FString(); }
}

FString FTryllDialogLabChecks::AuthoredLabel(const FTryllLabCheckSpec& Check)
{
	if (!Check.Note.IsEmpty())
	{
		return Check.Note;
	}
	FString Kind;
	FString Arg;
	Identity(Check, Kind, Arg);
	return LabelFor(Kind, Arg);
}

bool FTryllDialogLabChecks::AppliesTo(const FTryllLabCheckSpec& Check, ETryllLabCheckScope Scope, bool bSeeded)
{
	if (!Check.bEnabled)
	{
		return false;
	}
	if (bSeeded && Scope == ETryllLabCheckScope::Global)
	{
		return false;
	}
	if (bSeeded && (Check.Type == FTryllLabCheckSpec::TypeCallsTool || Check.Type == FTryllLabCheckSpec::TypeCallsNoTool))
	{
		return false;
	}
	if (Check.Type == FTryllLabCheckSpec::TypeMustContain || Check.Type == FTryllLabCheckSpec::TypeMustNotContain)
	{
		return !Check.Text.IsEmpty();
	}
	if (Check.Type == FTryllLabCheckSpec::TypeRegexForbid || Check.Type == FTryllLabCheckSpec::TypeRegexMatch)
	{
		return !Check.Pattern.IsEmpty();
	}
	if (Check.Type == FTryllLabCheckSpec::TypeWordCount)
	{
		return Check.MinWords > 0 || Check.MaxWords > 0;
	}
	return true;
}

TArray<FTryllLabCheck> FTryllDialogLabChecks::AuthoredChecks(
	const FTryllLabScriptSnapshot& Script, const FTryllLabStepSnapshot& Step, bool bSeeded)
{
	TArray<FTryllLabCheck> Rows;
	for (int32 Index = 0; Index < Step.Checks.Num(); ++Index)
	{
		if (AppliesTo(Step.Checks[Index], ETryllLabCheckScope::Step, bSeeded))
		{
			Rows.Add(Row(ETryllLabCheckScope::Step, Index, Step.Checks[Index]));
		}
	}
	for (int32 Index = 0; Index < Script.GlobalChecks.Num(); ++Index)
	{
		if (AppliesTo(Script.GlobalChecks[Index], ETryllLabCheckScope::Global, bSeeded))
		{
			Rows.Add(Row(ETryllLabCheckScope::Global, Index, Script.GlobalChecks[Index]));
		}
	}
	if (!bSeeded)
	{
		FTryllLabCheck Gate;
		Gate.Scope = ETryllLabCheckScope::Gate;
		Gate.Index = 0;
		Gate.Kind  = KindToolCall;
		Gate.Arg   = CannedToolNames(Step).Num() == 0 ? FString() : FString(ToolCallOtherArg);
		Gate.Label = LabelFor(Gate.Kind, Gate.Arg);
		Rows.Add(MoveTemp(Gate));
	}
	return Rows;
}

ETryllLabCellStatus FTryllDialogLabChecks::EvaluateStep(
	const FTryllLabScriptSnapshot&  Script,
	const FTryllLabStepSnapshot&    Step,
	const FString&                  AnswerText,
	const TSharedPtr<FJsonObject>&  DebugInfo,
	ETryllDiagnosticsAbsenceReason  Absence,
	TArray<FTryllLabCheckOutcome>&  OutOutcomes,
	bool                            bUseLiveTools,
	const TArray<FString>&          MissingResumedTools)
{
	OutOutcomes.Reset();
	const bool bSeeded = Absence == ETryllDiagnosticsAbsenceReason::Seeded;

	FTurnContext Ctx;
	Ctx.Text        = AnswerText;
	Ctx.Lower       = AnswerText.ToLower();
	Ctx.Words       = CountWords(AnswerText);
	Ctx.DebugInfo   = DebugInfo;
	Ctx.Absence     = Absence;
	Ctx.ActualTools = ActualToolNames(DebugInfo);

	for (int32 Index = 0; Index < Step.Checks.Num(); ++Index)
	{
		if (AppliesTo(Step.Checks[Index], ETryllLabCheckScope::Step, bSeeded))
		{
			FTryllLabCheckOutcome Outcome = EvaluateOne(Step.Checks[Index], Ctx);
			Outcome.Scope = ETryllLabCheckScope::Step;
			Outcome.Index = Index;
			OutOutcomes.Add(MoveTemp(Outcome));
		}
	}
	for (int32 Index = 0; Index < Script.GlobalChecks.Num(); ++Index)
	{
		if (AppliesTo(Script.GlobalChecks[Index], ETryllLabCheckScope::Global, bSeeded))
		{
			FTryllLabCheckOutcome Outcome = EvaluateOne(Script.GlobalChecks[Index], Ctx);
			Outcome.Scope = ETryllLabCheckScope::Global;
			Outcome.Index = Index;
			OutOutcomes.Add(MoveTemp(Outcome));
		}
	}
	if (!bSeeded)
	{
		OutOutcomes.Add(EvaluateGate(Step, Ctx, bUseLiveTools, MissingResumedTools));
	}

	for (const FTryllLabCheckOutcome& Outcome : OutOutcomes)
	{
		if (!Outcome.bPassed)
		{
			return ETryllLabCellStatus::CheckFail;
		}
	}
	return ETryllLabCellStatus::Ok;
}

bool FTryllDialogLabChecks::EvaluateTextOnly(
	const FTryllLabCheckSpec& Check, const FString& Text, FTryllLabCheckOutcome& OutOutcome)
{
	const FString& Type = Check.Type;
	if (Type != FTryllLabCheckSpec::TypeMustContain && Type != FTryllLabCheckSpec::TypeMustNotContain
		&& Type != FTryllLabCheckSpec::TypeRegexForbid && Type != FTryllLabCheckSpec::TypeRegexMatch
		&& Type != FTryllLabCheckSpec::TypeWordCount)
	{
		return false;
	}
	FTurnContext Ctx;
	Ctx.Text    = Text;
	Ctx.Lower   = Text.ToLower();
	Ctx.Words   = CountWords(Text);
	Ctx.Absence = ETryllDiagnosticsAbsenceReason::Seeded;
	OutOutcome = EvaluateOne(Check, Ctx);
	return true;
}

bool FTryllDialogLabChecks::Matches(const FTryllLabCheckOutcome& Outcome, const FTryllLabCheck& Authored)
{
	return Outcome.Scope == Authored.Scope && Outcome.Index == Authored.Index;
}

int32 FTryllDialogLabChecks::CountWords(const FString& Text)
{
	// Unity splits on any Unicode whitespace and drops empties; ParseIntoArrayWS only knows
	// a handful of ASCII separators, so count runs directly.
	int32 Count = 0;
	bool  bInWord = false;
	for (const TCHAR Ch : Text)
	{
		if (FChar::IsWhitespace(Ch))
		{
			bInWord = false;
		}
		else if (!bInWord)
		{
			bInWord = true;
			++Count;
		}
	}
	return Count;
}
