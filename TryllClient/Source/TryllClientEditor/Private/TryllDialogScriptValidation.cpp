// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllDialogScriptValidation.h"

#include "TryllDialogLabChecks.h"
#include "TryllDialogLabScriptData.h"
#include "TryllDialogScriptAsset.h"
#include "TryllGraphDescription.h"
#include "Generated/Nodes/TryllNodeParamsBase.h"
#include "UObject/UnrealType.h"

namespace
{
TSet<FString> CollectLlmIntentIds(const UTryllNodeParamsBase* Params)
{
	TSet<FString> Set;
	if (Params == nullptr)
	{
		return Set;
	}

	if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(
			Params->GetClass()->FindPropertyByName(TEXT("Intents"))))
	{
		FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Params));
		if (Helper.Num() > 0)
		{
			if (const FStructProperty* AsStruct = CastField<FStructProperty>(ArrayProp->Inner))
			{
				if (const FStrProperty* IdProp = CastField<FStrProperty>(
						AsStruct->Struct->FindPropertyByName(TEXT("Id"))))
				{
					for (int32 Index = 0; Index < Helper.Num(); ++Index)
					{
						FString Id = *IdProp->ContainerPtrToValuePtr<FString>(Helper.GetRawPtr(Index));
						Id.TrimStartAndEndInline();
						if (Id.Len() > 0)
						{
							Set.Add(Id);
						}
					}
				}
			}
		}
	}
	return Set;
}
} // namespace

bool FTryllDialogScriptValidation::TryValidate(const UTryllDialogScriptAsset* Script, FString& OutError)
{
	OutError.Reset();
	if (Script == nullptr)
	{
		OutError = TEXT("Assign a Dialog Script.");
		return false;
	}
	return TryValidate(FTryllLabScriptSnapshot::FromAsset(Script), OutError);
}

bool FTryllDialogScriptValidation::TryValidate(const FTryllLabScriptSnapshot& Script, FString& OutError)
{
	OutError.Reset();
	const int32 SeedTurnCount = Script.SeedTurnCount;
	const TArray<FTryllLabStepSnapshot>& Steps = Script.Steps;

	if (SeedTurnCount < 0)
	{
		OutError = TEXT("Dialog Script: SeedTurnCount cannot be negative.");
		return false;
	}
	if (SeedTurnCount > Steps.Num())
	{
		OutError = TEXT("Dialog Script: SeedTurnCount exceeds the number of steps.");
		return false;
	}
	if (Steps.Num() - SeedTurnCount <= 0)
	{
		OutError = TEXT("Dialog Script: add at least one generated step ")
		           TEXT("(a user line after the seeded history).");
		return false;
	}

	for (int32 Index = 0; Index < Steps.Num(); ++Index)
	{
		const FTryllLabStepSnapshot& Step = Steps[Index];
		const bool bSeeded = Index < SeedTurnCount;

		if (Step.User.TrimStartAndEnd().IsEmpty())
		{
			OutError = FString::Printf(
				TEXT("Dialog Script: %s step %d needs a user line."),
				bSeeded ? TEXT("seeded") : TEXT("generated"), Index + 1);
			return false;
		}
		if (bSeeded && Step.Assistant.TrimStartAndEnd().IsEmpty())
		{
			OutError = FString::Printf(
				TEXT("Dialog Script: seeded step %d needs an assistant line."), Index + 1);
			return false;
		}

		for (int32 CheckIndex = 0; CheckIndex < Step.Checks.Num(); ++CheckIndex)
		{
			const FString Where = FString::Printf(TEXT("%s step %d, check %d"),
				bSeeded ? TEXT("seeded") : TEXT("generated"), Index + 1, CheckIndex + 1);
			if (!TryValidateCheck(Step.Checks[CheckIndex], Where, bSeeded, OutError))
			{
				return false;
			}
		}
	}

	for (int32 CheckIndex = 0; CheckIndex < Script.GlobalChecks.Num(); ++CheckIndex)
	{
		if (!TryValidateCheck(Script.GlobalChecks[CheckIndex],
				FString::Printf(TEXT("global check %d"), CheckIndex + 1), /*bSeeded*/ false, OutError))
		{
			return false;
		}
	}

	return true;
}

bool FTryllDialogScriptValidation::TryValidateCheck(
	const FTryllLabCheckSpec& Check, const FString& Where, bool bSeeded, FString& OutError)
{
	OutError.Reset();
	if (!Check.bEnabled)
	{
		return true;
	}
	const FString Prefix = TEXT("Dialog Script: ") + Where;
	const FString& Type = Check.Type;
	auto Fail = [&OutError](const FString& Message)
	{
		OutError = Message;
		return false;
	};

	const bool bNeedsGraph = Type == FTryllLabCheckSpec::TypeIntentIs || Type == FTryllLabCheckSpec::TypeIntentNotFound
		|| Type == FTryllLabCheckSpec::TypeAttributeEquals || Type == FTryllLabCheckSpec::TypeAttributeInRange
		|| Type == FTryllLabCheckSpec::TypeCallsTool || Type == FTryllLabCheckSpec::TypeCallsNoTool;
	if (bSeeded && bNeedsGraph)
	{
		return Fail(FString::Printf(
			TEXT("%s (%s) needs the graph to run, but a seeded turn is injected with AppendInteractions ")
			TEXT("and never runs it, so there is nothing to check. Move the check to a generated step."),
			*Prefix, *Type.Replace(TEXT("_"), TEXT(" "))));
	}
	if ((Type == FTryllLabCheckSpec::TypeMustContain || Type == FTryllLabCheckSpec::TypeMustNotContain)
		&& Check.Text.IsEmpty())
	{
		return Fail(Prefix + TEXT(": the text is empty."));
	}
	if ((Type == FTryllLabCheckSpec::TypeRegexForbid || Type == FTryllLabCheckSpec::TypeRegexMatch)
		&& Check.Pattern.IsEmpty())
	{
		return Fail(Prefix + TEXT(": the pattern is empty."));
	}
	if (Type == FTryllLabCheckSpec::TypeWordCount)
	{
		if (Check.MinWords <= 0 && Check.MaxWords <= 0)
		{
			return Fail(Prefix + TEXT(": set Min, Max, or both (0 leaves a bound unchecked)."));
		}
		if (Check.MaxWords > 0 && Check.MinWords > Check.MaxWords)
		{
			return Fail(FString::Printf(TEXT("%s: Min (%d) is greater than Max (%d)."),
				*Prefix, Check.MinWords, Check.MaxWords));
		}
	}
	if (Type == FTryllLabCheckSpec::TypeIntentIs && Check.Intent.TrimStartAndEnd().IsEmpty())
	{
		return Fail(Prefix + TEXT(" has an intent check with no expected label."));
	}
	if ((Type == FTryllLabCheckSpec::TypeIntentIs || Type == FTryllLabCheckSpec::TypeIntentNotFound)
		&& (Check.Node.Contains(TEXT("|")) || Check.Intent.Contains(TEXT("|"))))
	{
		return Fail(Prefix + TEXT(": intent check names must not contain \"|\"."));
	}
	if ((Type == FTryllLabCheckSpec::TypeAttributeEquals || Type == FTryllLabCheckSpec::TypeAttributeInRange)
		&& Check.Name.TrimStartAndEnd().IsEmpty())
	{
		return Fail(Prefix + TEXT(": the attribute name is empty."));
	}
	if (Type == FTryllLabCheckSpec::TypeAttributeInRange && Check.MinScore > Check.MaxScore)
	{
		return Fail(FString::Printf(TEXT("%s: Min (%g) is greater than Max (%g)."),
			*Prefix, Check.MinScore, Check.MaxScore));
	}
	if (Type == FTryllLabCheckSpec::TypeCallsTool && Check.ToolName.TrimStartAndEnd().IsEmpty())
	{
		return Fail(Prefix + TEXT(": the tool name is empty."));
	}
	return true;
}

TArray<FString> FTryllDialogScriptValidation::CollectSeededGlobalCheckWarnings(const FTryllLabScriptSnapshot& Script)
{
	TArray<FString> Warnings;
	const int32 Seeded = FMath::Clamp(Script.SeedTurnCount, 0, Script.Steps.Num());
	for (int32 StepIndex = 0; StepIndex < Seeded; ++StepIndex)
	{
		const FString& Line = Script.Steps[StepIndex].Assistant;
		for (int32 CheckIndex = 0; CheckIndex < Script.GlobalChecks.Num(); ++CheckIndex)
		{
			const FTryllLabCheckSpec& Check = Script.GlobalChecks[CheckIndex];
			if (!FTryllDialogLabChecks::AppliesTo(Check, ETryllLabCheckScope::Step, /*bSeeded*/ true))
			{
				continue;
			}
			FTryllLabCheckOutcome Outcome;
			if (!FTryllDialogLabChecks::EvaluateTextOnly(Check, Line, Outcome)
				|| Outcome.bPassed || Outcome.Kind == FTryllDialogLabChecks::KindInvalidRegex)
			{
				continue;
			}
			Warnings.Add(SeededGlobalCheckWarning(StepIndex, CheckIndex, FTryllDialogLabChecks::AuthoredLabel(Check)));
		}
	}
	return Warnings;
}

FString FTryllDialogScriptValidation::SeededGlobalCheckWarning(int32 StepIndex, int32 CheckIndex, const FString& Label)
{
	return FString::Printf(
		TEXT("seeded step %d: the assistant line breaks global check %d (%s). ")
		TEXT("Global checks skip seeded turns, so this is not counted; edit the line if the check should hold there too."),
		StepIndex + 1, CheckIndex + 1, *Label);
}

TArray<FString> FTryllDialogScriptValidation::CollectIntentCheckWarnings(
	const FTryllLabScriptSnapshot& Script,
	const FTryllGraphDescription* Graph)
{
	TArray<FString> Warnings;

	TSet<FString> NodeNames;
	int32 ClassifierCount = 0;
	TMap<FString, TSet<FString>> LlmLabels;
	if (Graph != nullptr)
	{
		for (const FTryllNodeDescription& Node : Graph->Nodes)
		{
			if (Node.Name.IsEmpty())
			{
				continue;
			}
			NodeNames.Add(Node.Name);
			const ETryllNodeType NodeType = Node.Params != nullptr
				? Node.Params->GetNodeType()
				: static_cast<ETryllNodeType>(255);
			if (NodeType == ETryllNodeType::ClassifyIntent
				|| NodeType == ETryllNodeType::ClassifyIntentLLM)
			{
				++ClassifierCount;
			}
			if (NodeType == ETryllNodeType::ClassifyIntentLLM && Node.Params != nullptr)
			{
				TSet<FString> Set = CollectLlmIntentIds(Node.Params);
				if (Set.Num() > 0)
				{
					LlmLabels.Add(Node.Name, MoveTemp(Set));
				}
			}
		}
	}

	for (int32 Index = FMath::Max(0, Script.SeedTurnCount); Index < Script.Steps.Num(); ++Index)
	{
		for (const FTryllLabCheckSpec& Check : Script.Steps[Index].Checks)
		{
			const bool bIs = Check.Type == FTryllLabCheckSpec::TypeIntentIs;
			if (!Check.bEnabled || (!bIs && Check.Type != FTryllLabCheckSpec::TypeIntentNotFound))
			{
				continue;
			}
			if (!Check.Node.IsEmpty() && !NodeNames.Contains(Check.Node))
			{
				Warnings.Add(FString::Printf(
					TEXT("generated step %d: intent check names node \"%s\" which is not in this workflow."),
					Index + 1, *Check.Node));
			}
			if (Check.Node.IsEmpty() && ClassifierCount > 1)
			{
				Warnings.Add(FString::Printf(
					TEXT("generated step %d: unscoped intent check will address all %d classifier nodes."),
					Index + 1, ClassifierCount));
			}
			if (bIs && !Check.Intent.IsEmpty() && !Check.Node.IsEmpty())
			{
				if (const TSet<FString>* Labels = LlmLabels.Find(Check.Node))
				{
					if (!Labels->Contains(Check.Intent))
					{
						Warnings.Add(FString::Printf(
							TEXT("generated step %d: intent \"%s\" is not in node \"%s\" intents."),
							Index + 1, *Check.Intent, *Check.Node));
					}
				}
			}
		}
	}
	return Warnings;
}

bool FTryllDialogScriptValidation::TryValidateForAgent(
	int32 SeedTurnCount,
	bool bMaintainDialogueHistory,
	const FString& AgentName,
	FString& OutError)
{
	OutError.Reset();
	if (bMaintainDialogueHistory || SeedTurnCount <= 0)
	{
		return true;
	}

	OutError = FString::Printf(
		TEXT("agent '%s' is stateless (Maintain Dialogue History is off), but the script seeds ")
		TEXT("%d turn(s) of history. The server clears a stateless agent's dialogue after every ")
		TEXT("turn, so the seed would only reach the first generated turn. Set SeedTurnCount to ")
		TEXT("0, or run this script against a stateful agent."),
		*AgentName, SeedTurnCount);
	return false;
}
