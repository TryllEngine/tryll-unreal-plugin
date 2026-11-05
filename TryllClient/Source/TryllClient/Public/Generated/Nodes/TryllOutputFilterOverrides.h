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
#include "TryllOutputFilterOverrides.generated.h"

namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllOutputFilterOverrides = void;
}

/**
 * Bounded streaming cleanup of roleplay artifacts in Generate / GenerateAndSpeak output (speaker prefixes and paired markup spans). All flags default off. Budgets are UTF-8 byte counts. Missing wire sub-table is treated by the server as an explicit all-default/off table; generated clients still materialize the sub-table (same invariant as SamplingOverrides). No on_change — nodes read live params at Execute.
 */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllOutputFilterOverrides
{
    GENERATED_BODY()

    /** Remove a leading speaker tag such as "Bob: " from the answer, once per turn. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bStripSpeakerPrefix = false;

    /** Longest speaker tag Strip Speaker Prefix looks for, in bytes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(ClampMin="1.0", ClampMax="64.0", UIMin="1.0", UIMax="64.0"))
    int32 MaxPrefixBytes = 16;

    /** Remove text between asterisks, such as *smiles*. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bStripAsteriskSpans = false;

    /** Remove text in parentheses, such as (laughs). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bStripParenSpans = false;

    /** Remove text in square brackets, such as [aside]. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bStripBracketSpans = false;

    /** Longest span the strip options can remove, in bytes. Longer spans are kept as they are. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(ClampMin="1.0", ClampMax="256.0", UIMin="1.0", UIMax="256.0"))
    int32 MaxSpanBytes = 64;

};
