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
#include "TryllLocalizableText.generated.h"

namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllLocalizableText = void;
}

/**
 * One string a node can store inline or as a Text ID.
 */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllLocalizableText
{
    GENERATED_BODY()

    /** The text. Tick Localized to store a Text ID from the workflow text table. */
    /** Ticked: TextTextId is resolved by the text provider and that text is sent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(TryllLocalizedToggle))
    bool bTextLocalized = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(MultiLine="true", EditCondition="!bTextLocalized", EditConditionHides, TryllLocalizable))
    FString Text;

    /** Text ID in the workflow text table. The provider's text is what is sent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="bTextLocalized", EditConditionHides, TryllTextId))
    FString TextTextId;

};
