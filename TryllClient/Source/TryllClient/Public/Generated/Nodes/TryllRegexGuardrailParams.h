// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/nodes.fbs  +  tools/codegen/templates/unreal_node.h.j2
// See: docs/dev/codegen.md
#pragma once



#include "CoreMinimal.h"
#include "Generated/Nodes/TryllNodeParamsBase.h"
#include "TryllGraphDescription.h"
#include "TryllStoragePath.h"
#include "TryllSlotInput.h"
#include "CodegenFingerprint.gen.h"

#include <string_view>
#include "TryllRegexGuardrailParams.generated.h"

namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllRegexGuardrailParams = void;
}

/**
 * Regex guardrail node backed by session string storage. Branches on whether the node's input slot text matches a pattern in the storage.  Resolution priority (factory): named string_storage → inline_strings → server-wide default (default-guardrail-patterns.txt from server-config).
 */
UCLASS(EditInlineNew, BlueprintType, meta=(DisplayName="RegexGuardrail"))
class TRYLLCLIENT_API UTryllRegexGuardrailParams : public UTryllNodeParamsBase
{
    GENERATED_BODY()
public:
    /** Text checked against the patterns: the player's message or an earlier node's output. Empty = the player's message. */
    /** Set to true to override the inherited Input value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input", meta=(InlineEditConditionToggle))
    bool bOverrideInput = false;
    /** Upstream inter-node slot this field consumes. Empty = context default. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input",
              meta=(EditCondition="bOverrideInput", TryllStructural))
    FTryllSlotInput Input;

    /** File of regex patterns, one per line. Used instead of Inline Strings. Both empty = the server's default patterns. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storage",
              meta=(TryllStorageKind="plain"))
    FTryllStoragePath StringStorage;

    /** Regex patterns typed here; used when String Storage is empty. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storage",
              meta=(TryllStructural))
    TArray<FString> InlineStrings;

    /** Node to run when the Input matches a pattern. Empty = end the turn. */
    /** Graph exit "triggered" — target node name; empty = END. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exits",
              meta=(GetOptions="GetExitTargetOptions", TryllExit="triggered", TryllStructural))
    FString TriggeredExit;

    /** Node to run when no pattern matches. Empty = end the turn. */
    /** Graph exit "not_triggered" — target node name; empty = END. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exits",
              meta=(GetOptions="GetExitTargetOptions", TryllExit="not_triggered", TryllStructural))
    FString NotTriggeredExit;


    virtual ETryllNodeType GetNodeType() const override { return ETryllNodeType::RegexGuardrail; }
    virtual FString GetDocsUrl() const override { return TEXT("https://docs.tryllengine.com/v0.10/reference/nodes/regex-guardrail.html"); }
    virtual flatbuffers::Offset<void> Pack(flatbuffers::FlatBufferBuilder& Fbb,
                                           Tryll::NodeParams::NodeParams& OutType) const override;
};
