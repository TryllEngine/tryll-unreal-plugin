// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonValue.h"
#include "TryllVariableDecl.h"

/**
 * A headless Dialog Lab run's config file (-tryllLabConfig), format 1. A public surface: CI jobs
 * script against it, so keys are never renamed, unknown keys are an error rather than ignored,
 * and a breaking change bumps Format. Unity's TryllDialogLabBatchConfig reads the same file with
 * the same messages.
 */
struct TRYLLCLIENTEDITOR_API FTryllDialogLabBatchConfig
{
	static constexpr int32 Format = 1;

	FString Workflow;
	/** Display name for the agent and the run's only variant; empty = the workflow asset's name. */
	FString AgentName;
	/** Raw JSON values by variable name, in file order; typed against the workflow later. */
	TArray<TPair<FString, TSharedPtr<FJsonValue>>> Variables;
	bool    bMaintainDialogueHistory = true;
	FString Model;

	/** A Dialog Script or Dialog Suite asset. */
	FString Dialogues;
	int32   Seeds = 0;
	int32   BaseSeed = 1;
	FString OutputDir = TEXT("Build/DialogLab");
	double  TurnTimeoutSeconds = 120.0;
	/** Accepted for Unity compatibility; Unreal always uses the project's Tryll settings. */
	FString Settings;

	TOptional<double> TurnsPassed;
	TOptional<double> ChecksPassed;
	TOptional<double> CleanDialogues;
	bool bUseLowerBound = false;

	static bool TryParse(const FString& Text, FTryllDialogLabBatchConfig& OutConfig, FString& OutError);

	/**
	 * The config's variables as component-style overrides, typed against the workflow's
	 * declarations. Unlike a component's stale override (a warning), a name the workflow does
	 * not declare or a value of the wrong type is an error: CI should not run a different NPC
	 * than the config describes.
	 */
	bool TryBuildOverrides(const TArray<FTryllVariableDecl>& Declared,
		TArray<FTryllVariableOverride>& OutOverrides, FString& OutError) const;

	/** use_lower_bound below the interval minimum: the config error, and the summary's guard. */
	/** What is "config" before the run, "run" for the summary's guard. */
	static FString LowerBoundSeedsError(int32 Seeds, const TCHAR* What = TEXT("config"));
};
