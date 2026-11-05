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
#include "TryllModelName.h"
#include "TryllSlotInput.h"
#include "TryllSlotName.h"
#include "Generated/Nodes/TryllClassifyAttributeDef.h"
#include "Generated/Nodes/TryllContextOverrides.h"
#include "Generated/Nodes/TryllModelAdaptation.h"
#include "CodegenFingerprint.gen.h"

#include <string_view>
#include "TryllClassifyAttributesLLMParams.generated.h"

namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllClassifyAttributesLLMParams = void;
}

/**
 * Several specialist classifiers scored in one cached batched call. Each attribute writes slot.<output_name>_<name>. triggered_exit is taken when any attribute's own rule fires.
 */
UCLASS(EditInlineNew, BlueprintType, meta=(DisplayName="ClassifyAttributesLLM"))
class TRYLLCLIENT_API UTryllClassifyAttributesLLMParams : public UTryllNodeParamsBase
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

    /** How many earlier dialog turns the classifier sees. 0 = the current message only. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input",
              meta=(ClampMin="0.0", ClampMax="64.0", UIMin="0.0", UIMax="64.0"))
    int32 HistoryTurns = 0;

    /** Language model that scores the attributes. Empty = the workflow's Default Model Name. */
    /** Set to true to override the inherited ModelName value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Model", meta=(InlineEditConditionToggle))
    bool bOverrideModelName = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Model",
              meta=(TryllModelType="language", EditCondition="bOverrideModelName", TryllStructural))
    FTryllModelName ModelName;

    /** Context window for this node, in tokens. 0 = sized automatically from the attribute prompts. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Model",
              meta=(ClampMin="0.0", UIMin="0.0", TryllStructural))
    int32 ContextSize = 0;

    /** Questions to classify, 1–16, all scored in one pass. Each writes its answer to <Output Name>_<attribute Name>. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prompt")
    TArray<FTryllClassifyAttributeDef> Attributes;

    /** Sharpens (below 1) or flattens (above 1) every attribute's probabilities before its thresholds. 0 = unchanged. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior",
              meta=(ClampMin="0.0", ClampMax="8.0", UIMin="0.0", UIMax="8.0"))
    float Temperature = 0.0f;

    /** Exit to take if scoring fails: Default (fail open) or Triggered (fail closed). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior")
    ETryllAttributeFailureRoute OnFailure = ETryllAttributeFailureRoute::Default;

    /** Prefix of this node's results: each attribute writes <Output Name>_<attribute Name>. Empty = this node's name. */
    /** Set to true to override the inherited OutputName value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Output", meta=(InlineEditConditionToggle))
    bool bOverrideOutputName = false;
    /** Effective output slot name for this node. Empty = the node's own name. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Output",
              meta=(EditCondition="bOverrideOutputName", TryllStructural))
    FTryllSlotName OutputName;

    /** When to send the game an attributes_classified event: Disabled, OnFound (Triggered exit only), or Always. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Output")
    ETryllNotifyClient NotifyClient = ETryllNotifyClient::Disabled;

    /** Advanced memory and batching settings for this node's model (KV cache type, GPU offload, batch sizes). Unset = inherit. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Advanced",
              meta=(TryllStructural))
    FTryllContextOverrides Context;

    /** LoRA adapters applied to this node's model. Empty = the base model. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Advanced",
              meta=(TryllStructural))
    FTryllModelAdaptation Adaptation;

    /** Node to run when any attribute fires (or scoring fails and On Failure is Triggered). Empty = end the turn. */
    /** Graph exit "triggered" — target node name; empty = END. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exits",
              meta=(GetOptions="GetExitTargetOptions", TryllExit="triggered", TryllStructural))
    FString TriggeredExit;

    /** Node to run when no attribute fires. Empty = end the turn. */
    /** Graph exit "default" — target node name; empty = END. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exits",
              meta=(GetOptions="GetExitTargetOptions", TryllExit="default", TryllStructural))
    FString DefaultExit;


    virtual ETryllNodeType GetNodeType() const override { return ETryllNodeType::ClassifyAttributesLLM; }
    virtual FString GetDocsUrl() const override { return TEXT("https://docs.tryllengine.com/v0.10/reference/nodes/classify-attributes-llm.html"); }
    virtual flatbuffers::Offset<void> Pack(flatbuffers::FlatBufferBuilder& Fbb,
                                           Tryll::NodeParams::NodeParams& OutType) const override;
};
