// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/nodes.fbs  +  tools/codegen/templates/unreal_node.h.j2
// See: docs/dev/codegen.md
#pragma once



#include "CoreMinimal.h"
#include "TryllGraphDescription.h"
#include "CodegenFingerprint.gen.h"

#include <string_view>
#include "TryllContextOverrides.generated.h"

namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllContextOverrides = void;
}

/**
 * Backend context construction. Null fields inherit ModelVariant then server config, except `n_outputs_max` which CreateContext defaults to 1. Structural at the field: these are baked into llama_context_params at context creation, so a change means a new context and a lost KV cache.
 */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllContextOverrides
{
    GENERATED_BODY()

    /** Number format of the context memory (KV cache). Smaller types save memory at some quality cost. Inherit = the model's setting, else q8_0. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ETryllKvCacheType KvCacheType = ETryllKvCacheType::Inherit;

    /** Keep the context memory on the GPU. Can only turn it off, not on for a model placed in RAM. Unset = server and model placement decide. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(InlineEditConditionToggle))
    bool bOverrideOffloadKqv = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(EditCondition="bOverrideOffloadKqv"))
    bool OffloadKqv = 0;

    /** Tokens processed per GPU step; sizes the working buffers. Unset = the model's setting. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(InlineEditConditionToggle))
    bool bOverrideNUbatch = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(EditCondition="bOverrideNUbatch", ClampMin="1.0", UIMin="1.0"))
    int32 NUbatch = 0;

    /** Leave unset. Advanced: how many positions return scores per step. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(InlineEditConditionToggle))
    bool bOverrideNOutputsMax = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(EditCondition="bOverrideNOutputsMax", ClampMin="1.0", UIMin="1.0"))
    int32 NOutputsMax = 0;

    /** Most tokens sent to the model in one call while reading the prompt. Unset = the model's setting. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(InlineEditConditionToggle))
    bool bOverrideNBatch = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(EditCondition="bOverrideNBatch", ClampMin="1.0", UIMin="1.0"))
    int32 NBatch = 0;

};
