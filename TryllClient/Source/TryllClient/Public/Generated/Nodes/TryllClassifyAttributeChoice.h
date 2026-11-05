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
#include "TryllClassifyAttributeChoice.generated.h"

namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllClassifyAttributeChoice = void;
}

/**
 * One option of a Choice or Score attribute. Boolean does not use this table: its menu is fixed as A=True, B=False.
 */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllClassifyAttributeChoice
{
    GENERATED_BODY()

    /** Value written to the output when this option wins. The model never sees it. Must be unique within the attribute. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Id;

    /** Description of the option the model reads, listed as "A. <prompt>". */
    /** Ticked: PromptTextId is resolved by the text provider and that text is sent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(TryllLocalizedToggle))
    bool bPromptLocalized = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(MultiLine="true", EditCondition="!bPromptLocalized", EditConditionHides, TryllLocalizable))
    FString Prompt;

    /** Text ID in the workflow text table. The provider's text is what is sent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="bPromptLocalized", EditConditionHides, TryllTextId))
    FString PromptTextId;

    /** Choice only. When this option is the answer, the node takes its Triggered exit. Leave off for Score attributes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bTriggers = false;

};
