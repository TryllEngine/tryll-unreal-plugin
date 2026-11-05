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
#include "CodegenFingerprint.gen.h"

#include <string_view>
#include "TryllPauseParams.generated.h"

namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllPauseParams = void;
}

/**
 * No-op node.  The executor pauses the turn after this node exits, giving the client a chance to inspect/mutate agent state (via ChangeAgentParam) before the graph continues. Resume with ResumeAgentRequest.
 */
UCLASS(EditInlineNew, BlueprintType, meta=(DisplayName="Pause"))
class TRYLLCLIENT_API UTryllPauseParams : public UTryllNodeParamsBase
{
    GENERATED_BODY()
public:
    /** How long to wait for the game to resume, in ms. 0 = forever. Set a limit unless the game always resumes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior",
              meta=(ClampMin="0.0", UIMin="0.0", TryllStructural))
    int32 PauseTimeoutMs = 0;

    /** Node to run when the game resumes without choosing one. Empty = end the turn. */
    /** Graph exit "default" — target node name; empty = END. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exits",
              meta=(GetOptions="GetExitTargetOptions", TryllExit="default", TryllStructural))
    FString DefaultExit;

    /** Node to run if the wait times out, such as a fallback line. Empty = end the turn as timed out. Unused when Pause Timeout Ms is 0. */
    /** Graph exit "timeout" — target node name; empty = END. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exits",
              meta=(GetOptions="GetExitTargetOptions", TryllExit="timeout", TryllStructural))
    FString TimeoutExit;


    virtual ETryllNodeType GetNodeType() const override { return ETryllNodeType::Pause; }
    virtual FString GetDocsUrl() const override { return TEXT("https://docs.tryllengine.com/v0.10/reference/nodes/pause.html"); }
    virtual flatbuffers::Offset<void> Pack(flatbuffers::FlatBufferBuilder& Fbb,
                                           Tryll::NodeParams::NodeParams& OutType) const override;
};
