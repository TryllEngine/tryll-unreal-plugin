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
#include "TryllClassifyIntentParams.generated.h"

namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllClassifyIntentParams = void;
}

/**
 * Intent classification node (embedding-based, top-1 nearest-neighbour). Searches a labelled embedded storage and attaches an IntentionComponent.
 */
UCLASS(EditInlineNew, BlueprintType, meta=(DisplayName="ClassifyIntent"))
class TRYLLCLIENT_API UTryllClassifyIntentParams : public UTryllNodeParamsBase
{
    GENERATED_BODY()
public:
    /** Text to classify: the player's message or an earlier node's output. Empty = the player's message. */
    /** Set to true to override the inherited Input value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input", meta=(InlineEditConditionToggle))
    bool bOverrideInput = false;
    /** Upstream inter-node slot this field consumes. Empty = context default. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input",
              meta=(EditCondition="bOverrideInput", TryllStructural))
    FTryllSlotInput Input;

    /** Embedded file of example phrases, each labelled with its intent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storage",
              meta=(TryllStorageKind="embedded", TryllStructural))
    FTryllStoragePath EmbeddedStringStorage;

    /** Metadata field in that file that holds each example's intent label. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storage")
    FString MetadataField;

    /** JSON filter on example metadata; only matching examples are compared. Empty = no filter. */
    /** Set to true to override the inherited Filter value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storage", meta=(InlineEditConditionToggle))
    bool bOverrideFilter = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storage",
              meta=(EditCondition="bOverrideFilter"))
    FString Filter;

    /** Max cosine distance of the closest example (0 = identical, 2 = opposite). Farther = Not Found. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior",
              meta=(ClampMin="0.0", ClampMax="2.0", UIMin="0.0", UIMax="2.0"))
    float Threshold = 0.25f;

    /** When to send the game an intent_classified event: Disabled, OnFound (Found exit only), or Always. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Output")
    ETryllNotifyClient NotifyClient = ETryllNotifyClient::Disabled;

    /** How many closest examples to report in diagnostics. Routing always uses the closest one. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Advanced",
              meta=(ClampMin="1.0", ClampMax="64.0", UIMin="1.0", UIMax="64.0"))
    int32 DiagnosticTopk = 5;

    /** Node to run when the closest example is within Threshold. Empty = end the turn. */
    /** Graph exit "found" — target node name; empty = END. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exits",
              meta=(GetOptions="GetExitTargetOptions", TryllExit="found", TryllStructural))
    FString FoundExit;

    /** Node to run when no example is close enough. Empty = end the turn. */
    /** Graph exit "not_found" — target node name; empty = END. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exits",
              meta=(GetOptions="GetExitTargetOptions", TryllExit="not_found", TryllStructural))
    FString NotFoundExit;


    virtual ETryllNodeType GetNodeType() const override { return ETryllNodeType::ClassifyIntent; }
    virtual FString GetDocsUrl() const override { return TEXT("https://docs.tryllengine.com/v0.10/reference/nodes/classify-intent.html"); }
    virtual flatbuffers::Offset<void> Pack(flatbuffers::FlatBufferBuilder& Fbb,
                                           Tryll::NodeParams::NodeParams& OutType) const override;
};
