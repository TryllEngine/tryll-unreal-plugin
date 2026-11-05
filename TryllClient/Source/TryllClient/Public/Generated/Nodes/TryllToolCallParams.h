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
#include "Generated/Nodes/TryllToolDefinition.h"
#include "Generated/Nodes/TryllSamplingOverrides.h"
#include "Generated/Nodes/TryllContextOverrides.h"
#include "Generated/Nodes/TryllModelAdaptation.h"
#include "CodegenFingerprint.gen.h"

#include <string_view>
#include "TryllToolCallParams.generated.h"

namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllToolCallParams = void;
}

/**
 * LLM-driven tool-call node. Constructs a tool-call prompt, runs sampling, parses the result.
 */
UCLASS(EditInlineNew, BlueprintType, meta=(DisplayName="ToolCall"))
class TRYLLCLIENT_API UTryllToolCallParams : public UTryllNodeParamsBase
{
    GENERATED_BODY()
public:
    /** Text the model reads to decide on a tool call: the player's message or an earlier node's output. Empty = the player's message. */
    /** Set to true to override the inherited Input value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input", meta=(InlineEditConditionToggle))
    bool bOverrideInput = false;
    /** Upstream inter-node slot this field consumes. Empty = context default. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input",
              meta=(EditCondition="bOverrideInput", TryllStructural))
    FTryllSlotInput Input;

    /** Language model this node runs. Empty = the workflow's Default Model Name. */
    /** Set to true to override the inherited ModelName value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Model", meta=(InlineEditConditionToggle))
    bool bOverrideModelName = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Model",
              meta=(TryllModelType="language", EditCondition="bOverrideModelName", TryllStructural))
    FTryllModelName ModelName;

    /** Context window for this node, in tokens; must fit the model's trained maximum. 0 = 2048. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Model",
              meta=(ClampMin="0.0", UIMin="0.0", TryllStructural))
    int32 ContextSize = 0;

    /** System message the model reads before the dialog. Empty = none. */
    /** Set to true to override the inherited SystemPrompt value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prompt", meta=(InlineEditConditionToggle))
    bool bOverrideSystemPrompt = false;
    /** Ticked: SystemPromptTextId is resolved by the text provider and that text is sent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prompt", meta=(TryllLocalizedToggle))
    bool bSystemPromptLocalized = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prompt",
              meta=(MultiLine="true", EditCondition="bOverrideSystemPrompt", TryllLocalizable))
    FString SystemPrompt;
    /** Text ID in the workflow text table. The provider's text is what is sent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prompt", meta=(EditCondition="bSystemPromptLocalized", EditConditionHides, TryllTextId))
    FString SystemPromptTextId;

    /** Tools the model may call, each with a name, description and parameters. The game carries out the call; the server only detects it. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prompt",
              meta=(TryllStructural))
    TArray<FTryllToolDefinition> Tools;

    /** DetectOnly: call a tool or say nothing. CallOrAnswer: call a tool or answer in text. RequireCall: must call a tool. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior")
    ETryllToolCallMode Mode = ETryllToolCallMode::CallOrAnswer;

    /** Allow several tool calls in one turn, if the model's chat template supports it. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior")
    bool bParallelToolCalls = false;

    /** After a call: whether to tell the game (Notify), wait for it (Pause, AwaitResult), and record an acknowledgement in history (…Acknowledge). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior")
    ETryllToolCallDisposition Disposition = ETryllToolCallDisposition::Acknowledge;

    /** Tool result recorded in history by the …Acknowledge dispositions. Empty = "ok". */
    /** Set to true to override the inherited AcknowledgeText value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior", meta=(InlineEditConditionToggle))
    bool bOverrideAcknowledgeText = false;
    /** Ticked: AcknowledgeTextTextId is resolved by the text provider and that text is sent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior", meta=(TryllLocalizedToggle))
    bool bAcknowledgeTextLocalized = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior",
              meta=(EditCondition="bOverrideAcknowledgeText", TryllLocalizable))
    FString AcknowledgeText;
    /** Text ID in the workflow text table. The provider's text is what is sent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior", meta=(EditCondition="bAcknowledgeTextLocalized", EditConditionHides, TryllTextId))
    FString AcknowledgeTextTextId;

    /** How long a pausing Disposition waits for the game, in ms. 0 = the server default (wait forever in released builds). On timeout the turn ends. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior",
              meta=(ClampMin="0.0", UIMin="0.0", TryllStructural))
    int32 PauseTimeoutMs = 0;

    /** Overrides for the model's sampling settings (temperature, top-p, max tokens…). Unset values keep the model's own. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Advanced")
    FTryllSamplingOverrides Sampling;

    /** Advanced memory and batching settings for this node's model (KV cache type, GPU offload, batch sizes). Unset = inherit. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Advanced",
              meta=(TryllStructural))
    FTryllContextOverrides Context;

    /** LoRA adapters applied to this node's model. Empty = the base model. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Advanced",
              meta=(TryllStructural))
    FTryllModelAdaptation Adaptation;

    /** Node to run when the model calls a tool. Empty = end the turn. */
    /** Graph exit "tool_called" — target node name; empty = END. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exits",
              meta=(GetOptions="GetExitTargetOptions", TryllExit="tool_called", TryllStructural))
    FString ToolCalledExit;

    /** Node to run when no tool is called (under CallOrAnswer, after the text answer). Empty = end the turn. */
    /** Graph exit "no_tool_called" — target node name; empty = END. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exits",
              meta=(GetOptions="GetExitTargetOptions", TryllExit="no_tool_called", TryllStructural))
    FString NoToolCalledExit;

    /** Not used. Kept so older workflows still load. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hidden",
              meta=(TryllStructural))
    FString ToolCallFormat;


    virtual ETryllNodeType GetNodeType() const override { return ETryllNodeType::ToolCall; }
    virtual FString GetDocsUrl() const override { return TEXT("https://docs.tryllengine.com/v0.10/reference/nodes/tool-call.html"); }
    virtual flatbuffers::Offset<void> Pack(flatbuffers::FlatBufferBuilder& Fbb,
                                           Tryll::NodeParams::NodeParams& OutType) const override;
};
