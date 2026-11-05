// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/nodes.fbs  +  tools/codegen/templates/unreal_node.h.j2
// See: docs/dev/codegen.md
#pragma once



#include "CoreMinimal.h"
#include "CodegenFingerprint.gen.h"

#include <string_view>
#include "TryllSamplingOverrides.generated.h"

namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllSamplingOverrides = void;
}

/**
 * Sparse sampling override block. Null on any field means "inherit the selected model's default". No per-field (on_change) attributes: the outer sub-table field carries one callback that re-resolves the whole block and commits derived state.
 */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllSamplingOverrides
{
    GENERATED_BODY()

    /** Randomness of the output: 0 = always the most likely token, 2 = very random. Unset = the model's default. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(InlineEditConditionToggle))
    bool bOverrideTemperature = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(EditCondition="bOverrideTemperature", ClampMin="0.0", ClampMax="2.0", UIMin="0.0", UIMax="2.0"))
    float Temperature = 0.0;

    /** Nucleus sampling: pick only from the most likely tokens that together reach this probability (0–1). Unset = the model's default. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(InlineEditConditionToggle))
    bool bOverrideTopP = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(EditCondition="bOverrideTopP", ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
    float TopP = 0.0;

    /** Pick only from this many most likely tokens. Unset = the model's default. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(InlineEditConditionToggle))
    bool bOverrideTopK = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(EditCondition="bOverrideTopK", ClampMin="0.0", UIMin="0.0"))
    int32 TopK = 0;

    /** Drop tokens less likely than this fraction of the top token (0–1). Unset = the model's default. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(InlineEditConditionToggle))
    bool bOverrideMinP = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(EditCondition="bOverrideMinP", ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
    float MinP = 0.0;

    /** Longest answer this node may generate, in tokens. Must be smaller than Context Size. Unset = the model's default. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(InlineEditConditionToggle))
    bool bOverrideMaxTokens = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(EditCondition="bOverrideMaxTokens", ClampMin="1.0", UIMin="1.0", TryllStructural))
    int32 MaxTokens = 0;

    /** Random seed; the same seed and input give the same output. Unset = the model's default. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(InlineEditConditionToggle))
    bool bOverrideSeed = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(EditCondition="bOverrideSeed"))
    int32 Seed = 0;

    /** Penalty for repeating recent tokens; 1 = none, higher = less repetition. Unset = the model's default. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(InlineEditConditionToggle))
    bool bOverrideRepeatPenalty = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(EditCondition="bOverrideRepeatPenalty", ClampMin="0.0", UIMin="0.0"))
    float RepeatPenalty = 0.0;

    /** Penalty for any token already used, pushing toward new topics. Unset = the model's default. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(InlineEditConditionToggle))
    bool bOverridePresencePenalty = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(EditCondition="bOverridePresencePenalty"))
    float PresencePenalty = 0.0;

    /** Penalty that grows with how often a token was already used. Unset = the model's default. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(InlineEditConditionToggle))
    bool bOverrideFrequencyPenalty = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(EditCondition="bOverrideFrequencyPenalty"))
    float FrequencyPenalty = 0.0;

};
