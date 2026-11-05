// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

struct FTryllLabCheckSpec;
struct FTryllLabStepSnapshot;
struct FTryllLabScriptSnapshot;
class FJsonObject;
enum class ETryllDiagnosticsAbsenceReason : uint8;

enum class ETryllLabCellStatus : uint8
{
	Ok,
	CheckFail,
	Error,
	Skipped,
};

/**
 * Where an outcome's check lives: on the step, in the script's GlobalChecks, or the Lab's
 * built-in "no unexpected tool call" gate.
 */
enum class ETryllLabCheckScope : uint8
{
	Step,
	Global,
	Gate,
};

/**
 * One evaluated check on one turn. Identity is Scope + Index (the check's position in its
 * list), so two checks of the same type and argument are evaluated and reported separately.
 * Kind says *how* it passed or failed (e.g. intent_unavailable for an Intent is check whose
 * classifier never ran); CheckType says which check type it was.
 */
struct FTryllLabCheckOutcome
{
	ETryllLabCheckScope Scope = ETryllLabCheckScope::Step;
	/** Position in the step's or the script's list; 0 for the gate; -1 when an imported
	 *  failure could not be tied to a check. */
	int32   Index = 0;
	FString CheckType;
	FString Kind;
	FString Arg;
	bool    bPassed = false;
	/** Optional: e.g. the word count when Kind == word_count. */
	FString Actual;
	/** Human chip text only. */
	FString Label;
};

/** One authored row; matched to outcomes by Scope + Index. */
struct FTryllLabCheck
{
	FString Kind;
	FString Arg;
	FString Label;
	ETryllLabCheckScope Scope = ETryllLabCheckScope::Step;
	int32 Index = 0;
};

/** Deterministic Dialog Lab checks — no model involved. A port of Unity's TryllDialogLabChecks. */
struct TRYLLCLIENTEDITOR_API FTryllDialogLabChecks
{
	static const TCHAR* KindMustContain;
	static const TCHAR* KindMustNotContain;
	static const TCHAR* KindRegexForbid;
	static const TCHAR* KindRegexMatch;
	static const TCHAR* KindWordCount;
	/** Pre-typed-check kind; only the v3 import still reads it. */
	static const TCHAR* KindMaxWords;
	static const TCHAR* KindInvalidRegex;
	static const TCHAR* KindIntentIs;
	static const TCHAR* KindIntentNotFound;
	static const TCHAR* KindIntentUnavailable;
	static const TCHAR* KindAttributeIs;
	static const TCHAR* KindAttributeScore;
	static const TCHAR* KindAttributeUnavailable;
	static const TCHAR* KindToolCall;
	/** Gate arg when the step has canned results: "must not call other tools". */
	static const TCHAR* ToolCallOtherArg;
	/** CheckType of the built-in gate. */
	static const TCHAR* GateUnexpectedToolCall;

	static FString AttributeArg(const FString& Node, const FString& Name, const FString& Value);
	static FString AttributeScoreArg(const FString& Node, const FString& Name, float Min, float Max);
	static FString IntentArg(const FString& Node, const FString& Intent);
	static bool TryParseIntentArg(const FString& Arg, FString& OutNode, FString& OutIntent);
	static FString WordCountArg(int32 Min, int32 Max);

	static FString LabelFor(const FString& Kind, const FString& Arg, const FString& Actual = FString());

	/** The kind and argument a check reports under; feeds LabelFor, the export, and the v3 import. */
	static void Identity(const FTryllLabCheckSpec& Check, FString& OutKind, FString& OutArg);

	/** The label an authored row shows: the author's Note when set. */
	static FString AuthoredLabel(const FTryllLabCheckSpec& Check);

	/**
	 * Whether a check runs on a turn of this kind. Tool checks need the graph to have run;
	 * global checks skip seeded turns; a disabled check never runs; a check with nothing to
	 * compare (a blank text or pattern, a word count with no bound) checks nothing — a blank
	 * forbidden pattern would otherwise match, and fail, every answer.
	 */
	static bool AppliesTo(const FTryllLabCheckSpec& Check, ETryllLabCheckScope Scope, bool bSeeded);

	/**
	 * The rows the inspector lists for one turn, in evaluation order: step checks, global
	 * checks, then the gate. Must describe exactly the set EvaluateStep evaluates.
	 */
	static TArray<FTryllLabCheck> AuthoredChecks(
		const FTryllLabScriptSnapshot& Script, const FTryllLabStepSnapshot& Step, bool bSeeded);

	/**
	 * Evaluate one turn: the step's checks, the script's global checks (generated turns only),
	 * and the "no unexpected tool call" gate (generated turns only). OutOutcomes gets every
	 * outcome, passes included. Seeded turns pass Absence = Seeded. bUseLiveTools lets the gate
	 * accept a call a live handler answered; MissingResumedTools were empty-resumed and still
	 * fail it.
	 */
	static ETryllLabCellStatus EvaluateStep(
		const FTryllLabScriptSnapshot&  Script,
		const FTryllLabStepSnapshot&    Step,
		const FString&                  AnswerText,
		const TSharedPtr<FJsonObject>&  DebugInfo,
		ETryllDiagnosticsAbsenceReason  Absence,
		TArray<FTryllLabCheckOutcome>&  OutOutcomes,
		bool                            bUseLiveTools = false,
		const TArray<FString>&          MissingResumedTools = TArray<FString>());

	/**
	 * One check against plain text, for the types that need nothing else (contain, regex, word
	 * count). False for the rest, which need a turn's diagnostics.
	 */
	static bool EvaluateTextOnly(const FTryllLabCheckSpec& Check, const FString& Text, FTryllLabCheckOutcome& OutOutcome);

	/** Does this outcome belong to that authored row? By position, never by label. */
	static bool Matches(const FTryllLabCheckOutcome& Outcome, const FTryllLabCheck& Authored);

	static int32 CountWords(const FString& Text);

	/**
	 * Whether ICU accepts this pattern. FRegexPattern discards the compile status, so an
	 * invalid pattern is indistinguishable from a valid one that never matches — which would
	 * silently pass a forbid check that Unity fails as invalid_regex. Probed by compiling the
	 * pattern with an empty alternative: any valid pattern then matches everything, and an
	 * invalid one still fails to compile and matches nothing.
	 */
	static bool IsPatternValid(const FString& Pattern);
};
