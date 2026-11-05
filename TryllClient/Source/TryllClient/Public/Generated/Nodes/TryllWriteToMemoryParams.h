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
#include "TryllSlotInput.h"
#include "CodegenFingerprint.gen.h"

#include <string_view>
#include "TryllWriteToMemoryParams.generated.h"

namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllWriteToMemoryParams = void;
}

/**
 * Upsert the current input slot into a named mutable store. An empty `input` reads the player's message. Only when that resolved text is empty does the node write nothing and take written_exit; only real write failures take failed_exit.
 */
UCLASS(EditInlineNew, BlueprintType, meta=(DisplayName="WriteToMemory"))
class TRYLLCLIENT_API UTryllWriteToMemoryParams : public UTryllNodeParamsBase
{
    GENERATED_BODY()
public:
    /** Text to store: the player's message or an earlier node's output. Empty = the player's message. */
    /** Set to true to override the inherited Input value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input", meta=(InlineEditConditionToggle))
    bool bOverrideInput = false;
    /** Upstream inter-node slot this field consumes. Empty = context default. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input",
              meta=(EditCondition="bOverrideInput", TryllStructural))
    FTryllSlotInput Input;

    /** Memory store to write to, by name; it lasts for the session. May include agent variables (var.*). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storage",
              meta=(TryllStructural))
    FString StorageName;

    /** Record schema of the store. Nodes that read the same store must use the same schema. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storage",
              meta=(TryllStructural))
    FString Schema;

    /** Record id; writing an existing id replaces that record. May include agent variables (var.*). Empty = add a new record each time. */
    /** Set to true to override the inherited Id value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storage", meta=(InlineEditConditionToggle))
    bool bOverrideId = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storage",
              meta=(EditCondition="bOverrideId"))
    FString Id;

    /** Node to run after the write, or when the text to store is empty (nothing is written). Empty = end the turn. */
    /** Graph exit "written" — target node name; empty = END. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exits",
              meta=(GetOptions="GetExitTargetOptions", TryllExit="written", TryllStructural))
    FString WrittenExit;

    /** Node to run when the store rejects the write (for example, the store is full). Empty = end the turn. */
    /** Graph exit "failed" — target node name; empty = END. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exits",
              meta=(GetOptions="GetExitTargetOptions", TryllExit="failed", TryllStructural))
    FString FailedExit;


    virtual ETryllNodeType GetNodeType() const override { return ETryllNodeType::WriteToMemory; }
    virtual FString GetDocsUrl() const override { return TEXT("https://docs.tryllengine.com/v0.10/reference/nodes/write-to-memory.html"); }
    virtual flatbuffers::Offset<void> Pack(flatbuffers::FlatBufferBuilder& Fbb,
                                           Tryll::NodeParams::NodeParams& OutType) const override;
};
