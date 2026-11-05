// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllDialogLabScriptData.h"

#include "TryllDialogChecks.h"

const TCHAR* FTryllLabCheckSpec::TypeMustContain      = TEXT("must_contain");
const TCHAR* FTryllLabCheckSpec::TypeMustNotContain   = TEXT("must_not_contain");
const TCHAR* FTryllLabCheckSpec::TypeRegexForbid      = TEXT("regex_forbid");
const TCHAR* FTryllLabCheckSpec::TypeRegexMatch       = TEXT("regex_match");
const TCHAR* FTryllLabCheckSpec::TypeWordCount        = TEXT("word_count");
const TCHAR* FTryllLabCheckSpec::TypeIntentIs         = TEXT("intent_is");
const TCHAR* FTryllLabCheckSpec::TypeIntentNotFound   = TEXT("intent_not_found");
const TCHAR* FTryllLabCheckSpec::TypeAttributeEquals  = TEXT("attribute_equals");
const TCHAR* FTryllLabCheckSpec::TypeAttributeInRange = TEXT("attribute_in_range");
const TCHAR* FTryllLabCheckSpec::TypeCallsTool        = TEXT("calls_tool");
const TCHAR* FTryllLabCheckSpec::TypeCallsNoTool      = TEXT("calls_no_tool");

bool FTryllLabCheckSpec::IsKnownType(const FString& Type)
{
	static const TCHAR* Known[] =
	{
		TypeMustContain, TypeMustNotContain, TypeRegexForbid, TypeRegexMatch, TypeWordCount,
		TypeIntentIs, TypeIntentNotFound, TypeAttributeEquals, TypeAttributeInRange,
		TypeCallsTool, TypeCallsNoTool,
	};
	for (const TCHAR* Id : Known)
	{
		if (Type == Id)
		{
			return true;
		}
	}
	return false;
}

FTryllLabCheckSpec FTryllLabCheckSpec::FromCheck(const UTryllDialogCheck* Check)
{
	FTryllLabCheckSpec Spec;
	if (Check == nullptr)
	{
		return Spec;
	}
	Spec.Type     = Check->GetTypeId();
	Spec.bEnabled = Check->bEnabled;
	Spec.Note     = Check->Note;
	if (const UTryllMustContainCheck* C = Cast<UTryllMustContainCheck>(Check))
	{
		Spec.Text = C->Text;
		Spec.bCaseSensitive = C->bCaseSensitive;
	}
	else if (const UTryllMustNotContainCheck* N = Cast<UTryllMustNotContainCheck>(Check))
	{
		Spec.Text = N->Text;
		Spec.bCaseSensitive = N->bCaseSensitive;
	}
	else if (const UTryllRegexForbidCheck* F = Cast<UTryllRegexForbidCheck>(Check))
	{
		Spec.Pattern = F->Pattern;
	}
	else if (const UTryllRegexMatchCheck* M = Cast<UTryllRegexMatchCheck>(Check))
	{
		Spec.Pattern = M->Pattern;
	}
	else if (const UTryllWordCountCheck* W = Cast<UTryllWordCountCheck>(Check))
	{
		Spec.MinWords = W->Min;
		Spec.MaxWords = W->Max;
	}
	else if (const UTryllIntentIsCheck* I = Cast<UTryllIntentIsCheck>(Check))
	{
		Spec.Node = I->Node;
		Spec.Intent = I->Intent;
	}
	else if (const UTryllIntentNotFoundCheck* NF = Cast<UTryllIntentNotFoundCheck>(Check))
	{
		Spec.Node = NF->Node;
	}
	else if (const UTryllAttributeEqualsCheck* AE = Cast<UTryllAttributeEqualsCheck>(Check))
	{
		Spec.Node = AE->Node;
		Spec.Name = AE->Name;
		Spec.Value = AE->Value;
	}
	else if (const UTryllAttributeInRangeCheck* AR = Cast<UTryllAttributeInRangeCheck>(Check))
	{
		Spec.Node = AR->Node;
		Spec.Name = AR->Name;
		Spec.MinScore = AR->Min;
		Spec.MaxScore = AR->Max;
	}
	else if (const UTryllCallsToolCheck* T = Cast<UTryllCallsToolCheck>(Check))
	{
		Spec.ToolName = T->ToolName;
	}
	return Spec;
}

UTryllDialogCheck* FTryllLabCheckSpec::ToCheck(UObject* Outer) const
{
	auto New = [Outer](UClass* Class) -> UTryllDialogCheck*
	{
		return NewObject<UTryllDialogCheck>(
			Outer != nullptr ? Outer : GetTransientPackage(), Class, NAME_None, RF_Transactional);
	};

	UTryllDialogCheck* Out = nullptr;
	if (Type == TypeMustContain)
	{
		UTryllMustContainCheck* C = CastChecked<UTryllMustContainCheck>(New(UTryllMustContainCheck::StaticClass()));
		C->Text = Text;
		C->bCaseSensitive = bCaseSensitive;
		Out = C;
	}
	else if (Type == TypeMustNotContain)
	{
		UTryllMustNotContainCheck* C = CastChecked<UTryllMustNotContainCheck>(New(UTryllMustNotContainCheck::StaticClass()));
		C->Text = Text;
		C->bCaseSensitive = bCaseSensitive;
		Out = C;
	}
	else if (Type == TypeRegexForbid)
	{
		UTryllRegexForbidCheck* C = CastChecked<UTryllRegexForbidCheck>(New(UTryllRegexForbidCheck::StaticClass()));
		C->Pattern = Pattern;
		Out = C;
	}
	else if (Type == TypeRegexMatch)
	{
		UTryllRegexMatchCheck* C = CastChecked<UTryllRegexMatchCheck>(New(UTryllRegexMatchCheck::StaticClass()));
		C->Pattern = Pattern;
		Out = C;
	}
	else if (Type == TypeWordCount)
	{
		UTryllWordCountCheck* C = CastChecked<UTryllWordCountCheck>(New(UTryllWordCountCheck::StaticClass()));
		C->Min = MinWords;
		C->Max = MaxWords;
		Out = C;
	}
	else if (Type == TypeIntentIs)
	{
		UTryllIntentIsCheck* C = CastChecked<UTryllIntentIsCheck>(New(UTryllIntentIsCheck::StaticClass()));
		C->Node = Node;
		C->Intent = Intent;
		Out = C;
	}
	else if (Type == TypeIntentNotFound)
	{
		UTryllIntentNotFoundCheck* C = CastChecked<UTryllIntentNotFoundCheck>(New(UTryllIntentNotFoundCheck::StaticClass()));
		C->Node = Node;
		Out = C;
	}
	else if (Type == TypeAttributeEquals)
	{
		UTryllAttributeEqualsCheck* C = CastChecked<UTryllAttributeEqualsCheck>(New(UTryllAttributeEqualsCheck::StaticClass()));
		C->Node = Node;
		C->Name = Name;
		C->Value = Value;
		Out = C;
	}
	else if (Type == TypeAttributeInRange)
	{
		UTryllAttributeInRangeCheck* C = CastChecked<UTryllAttributeInRangeCheck>(New(UTryllAttributeInRangeCheck::StaticClass()));
		C->Node = Node;
		C->Name = Name;
		C->Min = MinScore;
		C->Max = MaxScore;
		Out = C;
	}
	else if (Type == TypeCallsTool)
	{
		UTryllCallsToolCheck* C = CastChecked<UTryllCallsToolCheck>(New(UTryllCallsToolCheck::StaticClass()));
		C->ToolName = ToolName;
		Out = C;
	}
	else if (Type == TypeCallsNoTool)
	{
		Out = New(UTryllCallsNoToolCheck::StaticClass());
	}
	if (Out != nullptr)
	{
		Out->bEnabled = bEnabled;
		Out->Note = Note;
	}
	return Out;
}

TArray<FTryllLabCheckSpec> FTryllLabCheckSpec::FromLegacyStep(const FTryllDialogScriptStep& Step, bool bGenerated)
{
	TArray<FTryllLabCheckSpec> Out;
	for (const FString& S : Step.MustContain)
	{
		if (!S.IsEmpty())
		{
			FTryllLabCheckSpec C = Make(TypeMustContain);
			C.Text = S;
			Out.Add(C);
		}
	}
	for (const FString& S : Step.MustNotContain)
	{
		if (!S.IsEmpty())
		{
			FTryllLabCheckSpec C = Make(TypeMustNotContain);
			C.Text = S;
			Out.Add(C);
		}
	}
	for (const FString& S : Step.RegexForbid)
	{
		if (!S.IsEmpty())
		{
			FTryllLabCheckSpec C = Make(TypeRegexForbid);
			C.Pattern = S;
			Out.Add(C);
		}
	}
	if (Step.MaxWords > 0)
	{
		FTryllLabCheckSpec C = Make(TypeWordCount);
		C.MaxWords = Step.MaxWords;
		Out.Add(C);
	}
	for (const FTryllIntentCheck& I : Step.IntentChecks)
	{
		FTryllLabCheckSpec C = Make(I.bExpectNotFound ? TypeIntentNotFound : TypeIntentIs);
		C.Node = I.Node;
		if (!I.bExpectNotFound)
		{
			C.Intent = I.Intent;
		}
		Out.Add(C);
	}
	for (const FTryllAttributeCheck& A : Step.AttributeChecks)
	{
		FTryllLabCheckSpec C = Make(A.bScoreRange ? TypeAttributeInRange : TypeAttributeEquals);
		C.Node = A.Node;
		C.Name = A.Name;
		if (A.bScoreRange)
		{
			C.MinScore = A.ScoreMin;
			C.MaxScore = A.ScoreMax;
		}
		else
		{
			C.Value = A.Value;
		}
		Out.Add(C);
	}
	if (bGenerated)
	{
		TSet<FString> Seen;
		for (const FTryllCannedToolResult& T : Step.ToolResults)
		{
			if (!T.ToolName.IsEmpty() && !Seen.Contains(T.ToolName))
			{
				Seen.Add(T.ToolName);
				FTryllLabCheckSpec C = Make(TypeCallsTool);
				C.ToolName = T.ToolName;
				Out.Add(C);
			}
		}
	}
	return Out;
}

TArray<FTryllLabCheckSpec> FTryllLabCheckSpec::FromLegacyGlobal(const TArray<FString>& GlobalRegexForbid)
{
	TArray<FTryllLabCheckSpec> Out;
	for (const FString& S : GlobalRegexForbid)
	{
		if (!S.IsEmpty())
		{
			FTryllLabCheckSpec C = Make(TypeRegexForbid);
			C.Pattern = S;
			Out.Add(C);
		}
	}
	return Out;
}

FTryllLabScriptSnapshot FTryllLabScriptSnapshot::FromAsset(const UTryllDialogScriptAsset* Asset)
{
	FTryllLabScriptSnapshot Snapshot;
	if (Asset == nullptr)
	{
		return Snapshot;
	}
	Snapshot.Name          = Asset->GetName();
	Snapshot.Description   = Asset->Description;
	Snapshot.SeedTurnCount = Asset->SeedTurnCount;
	// Projected to values, so editing the asset mid-run cannot reach the snapshot. An element
	// with no type picked yet is dropped rather than carried as an empty check.
	for (const UTryllDialogCheck* Check : Asset->GlobalChecks)
	{
		if (Check != nullptr)
		{
			Snapshot.GlobalChecks.Add(FTryllLabCheckSpec::FromCheck(Check));
		}
	}
	for (const FTryllDialogScriptStep& Step : Asset->Steps)
	{
		FTryllLabStepSnapshot Out;
		Out.User        = Step.User;
		Out.Assistant   = Step.Assistant;
		Out.ToolResults = Step.ToolResults;
		for (const UTryllDialogCheck* Check : Step.Checks)
		{
			if (Check != nullptr)
			{
				Out.Checks.Add(FTryllLabCheckSpec::FromCheck(Check));
			}
		}
		Snapshot.Steps.Add(MoveTemp(Out));
	}
	return Snapshot;
}
