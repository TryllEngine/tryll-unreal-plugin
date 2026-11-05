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
#include "TryllToolParamDefinition.generated.h"

namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllToolParamDefinition = void;
}

/**
 * One named parameter in a tool's schema (used in SLM prompt construction).
 */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllToolParamDefinition
{
    GENERATED_BODY()

    /** Parameter name the model fills in when it calls the tool. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Name;

    /** JSON Schema type of the value, such as "string", "number" or "boolean". */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Type;

    /** What the parameter means; the model reads this to fill it in. */
    /** Ticked: DescriptionTextId is resolved by the text provider and that text is sent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(TryllLocalizedToggle))
    bool bDescriptionLocalized = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(MultiLine="true", EditCondition="!bDescriptionLocalized", EditConditionHides, TryllLocalizable))
    FString Description;

    /** Text ID in the workflow text table. The provider's text is what is sent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="bDescriptionLocalized", EditConditionHides, TryllTextId))
    FString DescriptionTextId;

};
