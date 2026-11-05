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
#include "TryllSlotName.h"
#include "Generated/Nodes/TryllLocalizableText.h"
#include "CodegenFingerprint.gen.h"

#include <string_view>
#include "TryllCannedResponseParams.generated.h"

namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllCannedResponseParams = void;
}

/**
 * Canned-response node backed by session string storage. Resolution priority (factory): named string_storage → lines → server-wide default (default-canned-responses.txt from server-config).
 */
UCLASS(EditInlineNew, BlueprintType, meta=(DisplayName="CannedResponse"))
class TRYLLCLIENT_API UTryllCannedResponseParams : public UTryllNodeParamsBase
{
    GENERATED_BODY()
public:
    /** File of responses, one per line. Used instead of Lines. Both empty = the server's default responses. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storage",
              meta=(TryllStorageKind="plain"))
    FTryllStoragePath StringStorage;

    /** Responses typed here; used when String Storage is empty. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storage",
              meta=(TryllStructural))
    TArray<FTryllLocalizableText> Lines;

    /** Which response comes next: Random, First (always the first), or RoundRobin (each in turn). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior")
    ETryllCannedResponseSelectionStrategy SelectionStrategy = ETryllCannedResponseSelectionStrategy::RoundRobin;

    /** Name later nodes use to read the chosen response (in their Input, or as the slot.<name> tag). Empty = this node's name. */
    /** Set to true to override the inherited OutputName value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Output", meta=(InlineEditConditionToggle))
    bool bOverrideOutputName = false;
    /** Effective output slot name for this node. Empty = the node's own name. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Output",
              meta=(EditCondition="bOverrideOutputName", TryllStructural))
    FTryllSlotName OutputName;

    /** How the response reaches the game: Whole (in one piece), Streamed, or None (kept for later nodes only). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Output")
    ETryllSendAnswer Send = ETryllSendAnswer::Whole;

    /** Whether later turns remember this response as an assistant message. None = forgotten after this turn. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Output")
    ETryllHistoryRole HistoryRole = ETryllHistoryRole::Assistant;

    /** Node to run after the response. Empty = end the turn. */
    /** Graph exit "default" — target node name; empty = END. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exits",
              meta=(GetOptions="GetExitTargetOptions", TryllExit="default", TryllStructural))
    FString DefaultExit;


    virtual ETryllNodeType GetNodeType() const override { return ETryllNodeType::CannedResponse; }
    virtual FString GetDocsUrl() const override { return TEXT("https://docs.tryllengine.com/v0.10/reference/nodes/canned-response.html"); }
    virtual flatbuffers::Offset<void> Pack(flatbuffers::FlatBufferBuilder& Fbb,
                                           Tryll::NodeParams::NodeParams& OutType) const override;

    UPROPERTY()
    TArray<FString> InlineStrings_DEPRECATED;
};
