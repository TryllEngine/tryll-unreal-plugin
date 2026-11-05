// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/nodes.fbs  +  tools/codegen/templates/unreal_node.h.j2
// See: docs/dev/codegen.md
#pragma once



#include "CoreMinimal.h"
#include "Generated/Nodes/TryllLoraAdapter.h"
#include "CodegenFingerprint.gen.h"

#include <string_view>
#include "TryllModelAdaptation.generated.h"

namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllModelAdaptation = void;
}

/**
 * Changes to how the model behaves for one LLM node, applied to the node's llama context after it is created. Kept apart from ContextOverrides, which only sizes memory and batching. Control vectors (steering) will join here.
 */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllModelAdaptation
{
    GENERATED_BODY()

    /** LoRA adapters applied together, up to 8, each file once. Order does not matter. Empty = none. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FTryllLoraAdapter> Loras;

};
