// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

class UTryllDialogScriptAsset;
struct FTryllLabScriptSnapshot;
struct FTryllLabCheckSpec;
struct FTryllGraphDescription;

/** Pre-run validation for dialog scripts — shared by the Lab panel and the tests. */
struct TRYLLCLIENTEDITOR_API FTryllDialogScriptValidation
{
	static bool TryValidate(const UTryllDialogScriptAsset* Script, FString& OutError);

	static bool TryValidate(const FTryllLabScriptSnapshot& Script, FString& OutError);

	/**
	 * One check's authoring errors; Where prefixes the message. A disabled check is not
	 * evaluated, so it is not validated either. A malformed regex is deliberately not an error:
	 * it fails its turn as invalid_regex, which is visible in the results.
	 */
	static bool TryValidateCheck(
		const FTryllLabCheckSpec& Check, const FString& Where, bool bSeeded, FString& OutError);

	/**
	 * Non-blocking warnings for intent checks against a variant's effective graph.
	 */
	static TArray<FString> CollectIntentCheckWarnings(
		const FTryllLabScriptSnapshot& Script,
		const FTryllGraphDescription* Graph);

	/**
	 * Non-blocking warnings for seeded assistant lines that break a global check. Global checks
	 * run on generated turns only, so such a line is never counted; it is flagged before Run
	 * because the history may be what the check guards against. Only checks that need nothing
	 * but the text are evaluated, and an invalid regex is left to fail its generated turns.
	 */
	static TArray<FString> CollectSeededGlobalCheckWarnings(const FTryllLabScriptSnapshot& Script);

	static FString SeededGlobalCheckWarning(int32 StepIndex, int32 CheckIndex, const FString& Label);

	/**
	 * The half of validation that needs the agent as well as the script. A stateless agent
	 * (MaintainDialogueHistory off) has its dialogue cleared by the server after every turn,
	 * so scripted history injected by AppendInteractions survives exactly one generated turn
	 * and is then gone. Running it anyway would silently apply the seed to the first generated
	 * step only — a result that looks fine and means nothing — so it is refused instead.
	 */
	static bool TryValidateForAgent(
		int32 SeedTurnCount,
		bool bMaintainDialogueHistory,
		const FString& AgentName,
		FString& OutError);
};
