// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllDialogLabConfig.generated.h"

/**
 * The Dialog Lab panel's persisted UI and authoring state.
 *
 * Unity's window survives a domain reload through [SerializeField]; Unreal has no domain reload,
 * so the same observable behaviour (spec 6.2) comes from editor config. The variant list is here
 * rather than in an asset because the experiment is deliberately window-authored (16.7) — but it
 * still has to survive an editor restart, which is what makes this class load-bearing rather than
 * a convenience.
 *
 * Object references are stored as paths, not pointers: an ini cannot hold a live UObject, and a
 * scene agent component may not exist yet when the config loads.
 */
UCLASS(config = EditorPerProjectUserSettings)
class UTryllDialogLabConfig : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(config)
	int32 Stage = 0;

	/** 0 = Statistics, 1 = Grid (the default). */
	UPROPERTY(config)
	int32 ViewMode = 1;

	UPROPERTY(config)
	bool bStatsDelta = false;

	UPROPERTY(config)
	bool bStatsIntervals = true;

	UPROPERTY(config)
	int32 Density = 1;

	UPROPERTY(config)
	float CellWidth = 276.0f;

	UPROPERTY(config)
	float InspectorWidth = 400.0f;

	UPROPERTY(config)
	bool bInspectorVisible = true;

	UPROPERTY(config)
	FString ScriptPath;

	UPROPERTY(config)
	FString DefaultAgentPath;

	UPROPERTY(config)
	int32 Repeats = 4;

	UPROPERTY(config)
	int32 BaseSeed = 1;

	/**
	 * One JSON object per variant. A nested USTRUCT array does not round-trip through an ini
	 * reliably, and a variant carries a colour, an agent path and a list of overrides — encoding
	 * each variant as one line keeps that intact without inventing a bespoke delimiter format
	 * that breaks the first time a prompt override contains a comma.
	 */
	UPROPERTY(config)
	TArray<FString> Variants;
};
