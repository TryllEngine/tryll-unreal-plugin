// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllDialogScriptAsset.h"

class UTryllDialogCheck;

/**
 * A typed check as a plain value: its type id plus the union of every type's fields. The Lab's
 * snapshot, runner, and import hold these rather than the asset's instanced UObjects, so a copy
 * is a real copy (an instanced pointer copied into a plain struct is shared and invisible to GC)
 * and an imported script needs no UObject at all. Type ids match Unity's TryllDialogCheck.TypeId.
 */
struct TRYLLCLIENTEDITOR_API FTryllLabCheckSpec
{
	static const TCHAR* TypeMustContain;
	static const TCHAR* TypeMustNotContain;
	static const TCHAR* TypeRegexForbid;
	static const TCHAR* TypeRegexMatch;
	static const TCHAR* TypeWordCount;
	static const TCHAR* TypeIntentIs;
	static const TCHAR* TypeIntentNotFound;
	static const TCHAR* TypeAttributeEquals;
	static const TCHAR* TypeAttributeInRange;
	static const TCHAR* TypeCallsTool;
	static const TCHAR* TypeCallsNoTool;

	FString Type;
	bool    bEnabled = true;
	FString Note;

	FString Text;              // must_contain, must_not_contain
	bool    bCaseSensitive = false;
	FString Pattern;           // regex_forbid, regex_match
	int32   MinWords = 0;      // word_count (0 = unchecked)
	int32   MaxWords = 0;
	FString Node;              // intent_*, attribute_*
	FString Intent;            // intent_is
	FString Name;              // attribute_*
	FString Value;             // attribute_equals
	float   MinScore = 0.f;    // attribute_in_range
	float   MaxScore = 1.f;
	FString ToolName;          // calls_tool

	static bool IsKnownType(const FString& Type);

	/** Projection of an asset check; Type is empty for a null slot (no type picked yet). */
	static FTryllLabCheckSpec FromCheck(const UTryllDialogCheck* Check);

	/** A new instanced check owned by Outer, or null for an unknown Type. */
	UTryllDialogCheck* ToCheck(UObject* Outer) const;

	static FTryllLabCheckSpec Make(const TCHAR* InType)
	{
		FTryllLabCheckSpec Spec;
		Spec.Type = InType;
		return Spec;
	}

	/**
	 * The pre-typed-check step fields as typed checks, in a fixed order (contains, not
	 * contains, forbid, word count, intents, attributes, then tools). Shared by asset load and
	 * the v3 import so both produce the same checks. On a generated step a canned tool result
	 * used to assert "must call that tool", so it becomes calls_tool; seeded steps never ran
	 * tools and get none. Identical to Unity's TryllDialogCheckMigration.
	 */
	static TArray<FTryllLabCheckSpec> FromLegacyStep(const FTryllDialogScriptStep& Step, bool bGenerated);
	static TArray<FTryllLabCheckSpec> FromLegacyGlobal(const TArray<FString>& GlobalRegexForbid);
};

/** One step of a script snapshot. */
struct FTryllLabStepSnapshot
{
	FString User;
	FString Assistant;
	TArray<FTryllLabCheckSpec>     Checks;
	/** Canned results: fixture data, keyed by tool name. */
	TArray<FTryllCannedToolResult> ToolResults;
};

/** Pure script projection — no asset reference, so a live edit cannot reach a running run. */
struct TRYLLCLIENTEDITOR_API FTryllLabScriptSnapshot
{
	FString Name;
	FString Description;
	int32   SeedTurnCount = 0;

	/** Checks applied to every *generated* turn, on top of each step's own Checks. */
	TArray<FTryllLabCheckSpec> GlobalChecks;

	TArray<FTryllLabStepSnapshot> Steps;

	int32 GeneratedStepCount() const { return FMath::Max(0, Steps.Num() - SeedTurnCount); }

	static FTryllLabScriptSnapshot FromAsset(const UTryllDialogScriptAsset* Asset);
};
