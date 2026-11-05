// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/nodes.fbs  +  tools/codegen/templates/unreal_node.h.j2
// See: docs/dev/codegen.md
#pragma once



#include "CoreMinimal.h"
#include "Generated/Nodes/TryllToolParamDefinition.h"
#include "CodegenFingerprint.gen.h"

#include <string_view>
#include "TryllToolDefinition.generated.h"

namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllToolDefinition = void;
}

/**
 * A callable tool definition used for prompt construction by ToolCallNode. The server does not execute tools; these are schema-only for SLM prompting.
 */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllToolDefinition
{
    GENERATED_BODY()

    /** Tool name the model uses to call it, and the game receives with the call. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Name;

    /** What the tool does; the model reads this to decide when to call it. */
    /** Ticked: DescriptionTextId is resolved by the text provider and that text is sent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(TryllLocalizedToggle))
    bool bDescriptionLocalized = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(MultiLine="true", EditCondition="!bDescriptionLocalized", EditConditionHides, TryllLocalizable))
    FString Description;

    /** Text ID in the workflow text table. The provider's text is what is sent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="bDescriptionLocalized", EditConditionHides, TryllTextId))
    FString DescriptionTextId;

    /** Values the model must supply when it calls this tool. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FTryllToolParamDefinition> Parameters;

};
