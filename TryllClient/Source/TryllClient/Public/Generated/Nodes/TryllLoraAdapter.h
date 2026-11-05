// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/nodes.fbs  +  tools/codegen/templates/unreal_node.h.j2
// See: docs/dev/codegen.md
#pragma once



#include "CoreMinimal.h"
#include "TryllStoragePath.h"
#include "CodegenFingerprint.gen.h"

#include <string_view>
#include "TryllLoraAdapter.generated.h"

namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllLoraAdapter = void;
}

/**
 * One LoRA adapter applied to an LLM node's context. The GGUF adapter must have been trained for the node's base model; a file made for another base fails agent creation. Loaded once per model and shared by every context that names the same file.
 */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllLoraAdapter
{
    GENERATED_BODY()

    /** LoRA adapter file (.gguf), relative to the storage root. Must be trained for the same base model as the node's model. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(TryllStorageKind="plain"))
    FTryllStoragePath Path;

    /** Adapter strength. 1 = as trained, 0 = off. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(ClampMin="0.0", ClampMax="4.0", UIMin="0.0", UIMax="4.0"))
    float Scale = 1.0f;

};
