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
#include "TryllSpeakParams.generated.h"

namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllSpeakParams = void;
}

/**
 * Standalone TTS-only node.  Voices its `input`-resolved slot (any upstream node's output — Generate, CannedResponse, etc. — or `user_message` itself), segments it into sentences, and synthesizes each on a dedicated TTS strand, emitting PCM frames via OnTtsAudio callbacks.  Unlike GenerateAndSpeak it runs no LLM inference, so it can voice canned/scripted lines or any upstream-produced answer without paying for a generation pass. The voice belongs to the agent (CreateAgentRequest.voice); agent creation fails without one (3025).
 */
UCLASS(EditInlineNew, BlueprintType, meta=(DisplayName="Speak"))
class TRYLLCLIENT_API UTryllSpeakParams : public UTryllNodeParamsBase
{
    GENERATED_BODY()
public:
    /** Text to speak: the player's message or an earlier node's output (for example a Canned Response). Empty = the player's message. */
    /** Set to true to override the inherited Input value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input", meta=(InlineEditConditionToggle))
    bool bOverrideInput = false;
    /** Upstream inter-node slot this field consumes. Empty = context default. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input",
              meta=(EditCondition="bOverrideInput", TryllStructural))
    FTryllSlotInput Input;

    /** Minimum length of a spoken chunk before a sentence end cuts it (CJK characters count double). Lower = speech starts sooner, more TTS calls. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior",
              meta=(ClampMin="1.0", UIMin="1.0"))
    int32 MinSentenceChars = 12;

    /** Lets the first chunk end early, at a comma, dash or space once it is this long, so speech starts sooner. 0 = off; try 48. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior",
              meta=(ClampMin="0.0", UIMin="0.0"))
    int32 FirstChunkMaxChars = 0;

    /** Node to run after the text is spoken. Empty = end the turn. */
    /** Graph exit "default" — target node name; empty = END. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exits",
              meta=(GetOptions="GetExitTargetOptions", TryllExit="default", TryllStructural))
    FString DefaultExit;


    virtual ETryllNodeType GetNodeType() const override { return ETryllNodeType::Speak; }
    virtual FString GetDocsUrl() const override { return TEXT("https://docs.tryllengine.com/v0.10/reference/nodes/speak.html"); }
    virtual bool IsTtsConsumer() const override { return true; }
    virtual flatbuffers::Offset<void> Pack(flatbuffers::FlatBufferBuilder& Fbb,
                                           Tryll::NodeParams::NodeParams& OutType) const override;
};
