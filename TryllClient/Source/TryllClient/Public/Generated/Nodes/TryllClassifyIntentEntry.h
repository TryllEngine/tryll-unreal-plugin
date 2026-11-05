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
#include "TryllClassifyIntentEntry.generated.h"

namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllClassifyIntentEntry = void;
}

/**
 * One intent label for ClassifyIntentLLM.  Order in the parent `intents` vector is letter order: entry 0 is A, entry 1 is B, …
 */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllClassifyIntentEntry
{
    GENERATED_BODY()

    /** Label written to the output when this intent wins. The model never sees it. Must be unique among the intents. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Id;

    /** Description of the intent the model reads, listed as "A. <prompt>", "B. <prompt>", … in Intents order. */
    /** Ticked: PromptTextId is resolved by the text provider and that text is sent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(TryllLocalizedToggle))
    bool bPromptLocalized = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(MultiLine="true", EditCondition="!bPromptLocalized", EditConditionHides, TryllLocalizable))
    FString Prompt;

    /** Text ID in the workflow text table. The provider's text is what is sent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="bPromptLocalized", EditConditionHides, TryllTextId))
    FString PromptTextId;

};
