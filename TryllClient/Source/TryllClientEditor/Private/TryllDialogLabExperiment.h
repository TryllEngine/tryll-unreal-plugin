// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllDialogLabExperiment.generated.h"

class UTryllAgentComponent;
class UTryllDialogScriptAsset;

/**
 * A single parameter override addressed by node instance name + field path. FieldPath is
 * either a top-level field name ("SystemPrompt") or a dotted path into a nested overridable
 * group ("Sampling.Seed"). Value is stored in its round-trippable string form and applied to
 * a cloned params object at run time, never to the workflow asset.
 */
USTRUCT()
struct FTryllLabParamOverride
{
	GENERATED_BODY()

	UPROPERTY()
	FString NodeName;

	UPROPERTY()
	FString FieldPath;

	UPROPERTY()
	FString Value;
};

/**
 * One configuration to compare, run on its own fresh agent. Labels are always
 * author-supplied — never derived from the override diff, which reads badly once a variant
 * touches more than one field.
 */
USTRUCT()
struct FTryllDialogLabVariant
{
	GENERATED_BODY()

	UPROPERTY()
	FString Label = TEXT("Variant");

	/**
	 * sRGB (0.31, 0.63, 1.00) pre-converted to linear, matching FTryllEditorStyle::Srgb — written out
	 * because a UPROPERTY default cannot call it. Handing Slate the raw sRGB numbers would render
	 * this swatch far brighter than every other colour in the window.
	 */
	UPROPERTY()
	FLinearColor Swatch = FLinearColor(0.0781f, 0.3568f, 1.0f);

	/**
	 * Optional per-variant agent. Null = the panel's default agent. This is how structural
	 * differences are compared — a different model, context size, or graph wiring — since
	 * those params are structural and ChangeParams rejects them (3006 ParamNotMutable).
	 */
	UPROPERTY()
	TWeakObjectPtr<UTryllAgentComponent> AgentOverride;

	/** Soft path for AgentOverride. Kept when the object is unloaded so the next encode does not wipe it. */
	UPROPERTY()
	FString AgentOverrideId;

	/**
	 * Optional per-variant language model. Empty = the workflow's own DefaultModelName. On
	 * the wire the default model is a CreateAgentRequest field (a sibling of graph), so the
	 * Lab bakes variant values onto a throwaway graph clone before CreateAgent — nothing
	 * structural is mutated and the asset is never written.
	 *
	 * Applies only to nodes that leave their own ModelName empty; a node that pins a model
	 * keeps it, which the Variants tab warns about rather than silently ignoring.
	 */
	UPROPERTY()
	FString ModelOverride;

	/**
	 * When false (default), a tool call with no canned result for the current step is a
	 * CheckFail — the model must not call that tool here, even if the scene agent has a
	 * live RegisterTool handler. The turn still completes so later steps run. When true,
	 * an uncanned call may be answered by that handler instead.
	 */
	UPROPERTY()
	bool bUseLiveTools = false;

	UPROPERTY()
	TArray<FTryllLabParamOverride> Overrides;
};

/**
 * The Lab panel's run configuration: which script, which variants, how many repeats.
 * Deliberately panel-authored rather than an asset — most variant sets are thirty-second
 * experiments, and forcing an asset-creation dialog for that is exactly the friction that
 * stops the feature being used.
 */
USTRUCT()
struct FTryllDialogLabExperiment
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<UTryllDialogScriptAsset> Script;

	UPROPERTY()
	TArray<FTryllDialogLabVariant> Variants;

	/**
	 * Repeats per variant. MUST default > 1: a single sample per variant reproduces the exact
	 * one-sample anecdote the Lab exists to replace — comparing prompt A to prompt B on one
	 * seed each is a coin flip presented as a result.
	 */
	UPROPERTY()
	int32 Repeats = 4;

	/**
	 * Seed base for derivation. MUST NOT be 0 — in SamplingParams, seed == 0 means "random
	 * seed", which would silently defeat reproducibility for every sample.
	 */
	UPROPERTY()
	int32 BaseSeed = 1;

	FTryllDialogLabExperiment()
	{
		FTryllDialogLabVariant Baseline;
		Baseline.Label = TEXT("Baseline");
		Variants.Add(Baseline);
	}

	/** Deterministic per-(variant, repeat) seed, reproducible and displayable. */
	int32 DeriveSeed(int32 VariantIndex, int32 Repeat) const
	{
		return BaseSeed + VariantIndex * 1000 + Repeat;
	}

	int32 TotalSamples() const
	{
		return Variants.Num() * FMath::Max(1, Repeats);
	}
};
