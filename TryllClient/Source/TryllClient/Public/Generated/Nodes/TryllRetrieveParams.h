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
#include "TryllRetrieveParams.generated.h"

namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllRetrieveParams = void;
}

/**
 * RAG retrieval node.  Embeds the HumanMessage, searches an embedded storage, and attaches a KnowledgeComponent to the current interaction.
 */
UCLASS(EditInlineNew, BlueprintType, meta=(DisplayName="Retrieve"))
class TRYLLCLIENT_API UTryllRetrieveParams : public UTryllNodeParamsBase
{
    GENERATED_BODY()
public:
    /** Text to search with: the player's message or an earlier node's output. Empty = the player's message. */
    /** Set to true to override the inherited Input value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input", meta=(InlineEditConditionToggle))
    bool bOverrideInput = false;
    /** Upstream inter-node slot this field consumes. Empty = context default. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input",
              meta=(EditCondition="bOverrideInput", TryllStructural))
    FTryllSlotInput Input;

    /** From File = a prebuilt knowledge file. From Name = a memory store the game fills during play (Write To Memory). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storage",
              meta=(TryllStructural))
    ETryllKnowledgeSourceKind SourceKind = ETryllKnowledgeSourceKind::FromFile;

    /** Embedded knowledge file to search. Used when Source Kind is From File; leave empty with From Name. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storage",
              meta=(TryllStorageKind="embedded", TryllStructural))
    FTryllStoragePath EmbeddedStringStorage;

    /** JSON filter on chunk metadata; only matching chunks are searched. Empty = no filter. */
    /** Set to true to override the inherited Filter value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storage", meta=(InlineEditConditionToggle))
    bool bOverrideFilter = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storage",
              meta=(EditCondition="bOverrideFilter"))
    FString Filter;

    /** Memory store to search, by name. Used when Source Kind is From Name. May include agent variables (var.*). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storage",
              meta=(TryllStructural))
    FString StorageName;

    /** Record schema of the store. Required with From Name; with From File, checked against the file. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storage",
              meta=(TryllStructural))
    FString Schema;

    /** Dense = match by meaning. Lexical = match by keywords (BM25). Hybrid = both, merged. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior")
    ETryllRetrievalMode RetrievalMode = ETryllRetrievalMode::Dense;

    /** How many matching chunks to return. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior",
              meta=(ClampMin="1.0", ClampMax="1000.0", UIMin="1.0", UIMax="1000.0"))
    int32 TopK = 2;

    /** Max cosine distance for a chunk to count (0 = identical, 2 = opposite). 0 = no limit. Ignored in Lexical mode. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior",
              meta=(ClampMin="0.0", ClampMax="2.0", UIMin="0.0", UIMax="2.0"))
    float Threshold = 0.5f;

    /** Hybrid only: how strongly the merge favours top-ranked hits. Higher = flatter. 60 is the standard value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior",
              meta=(ClampMin="1.0", ClampMax="1000.0", UIMin="1.0", UIMax="1000.0"))
    int32 RrfK = 60;

    /** Name templates use for these results: the knowledge_<source> tag. Empty = this node's name. */
    /** Set to true to override the inherited Source value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Output", meta=(InlineEditConditionToggle))
    bool bOverrideSource = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Output",
              meta=(EditCondition="bOverrideSource"))
    FString Source;

    /** Node to run when at least one chunk is found. Empty = end the turn. */
    /** Graph exit "found" — target node name; empty = END. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exits",
              meta=(GetOptions="GetExitTargetOptions", TryllExit="found", TryllStructural))
    FString FoundExit;

    /** Node to run when nothing is found. Empty = end the turn. */
    /** Graph exit "not_found" — target node name; empty = END. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exits",
              meta=(GetOptions="GetExitTargetOptions", TryllExit="not_found", TryllStructural))
    FString NotFoundExit;


    virtual ETryllNodeType GetNodeType() const override { return ETryllNodeType::Retrieve; }
    virtual FString GetDocsUrl() const override { return TEXT("https://docs.tryllengine.com/v0.10/reference/nodes/retrieve.html"); }
    virtual flatbuffers::Offset<void> Pack(flatbuffers::FlatBufferBuilder& Fbb,
                                           Tryll::NodeParams::NodeParams& OutType) const override;
};
