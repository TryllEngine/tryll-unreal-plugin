// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/nodes.fbs  +  tools/codegen/templates/unreal_bp_factory.h.j2
// See: docs/dev/codegen.md
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Generated/Nodes/TryllNodeParamsBase.h"
#include "Generated/Nodes/TryllGenerateParams.h"
#include "Generated/Nodes/TryllRegexGuardrailParams.h"
#include "Generated/Nodes/TryllCannedResponseParams.h"
#include "Generated/Nodes/TryllToolCallParams.h"
#include "Generated/Nodes/TryllRetrieveParams.h"
#include "Generated/Nodes/TryllInstructionParams.h"
#include "Generated/Nodes/TryllClassifyIntentParams.h"
#include "Generated/Nodes/TryllClassifyIntentLLMParams.h"
#include "Generated/Nodes/TryllIntentToInstructionParams.h"
#include "Generated/Nodes/TryllGenerateAndSpeakParams.h"
#include "Generated/Nodes/TryllSpeakParams.h"
#include "Generated/Nodes/TryllPauseParams.h"
#include "Generated/Nodes/TryllTransformParams.h"
#include "Generated/Nodes/TryllBranchParams.h"
#include "Generated/Nodes/TryllWriteToMemoryParams.h"
#include "Generated/Nodes/TryllSummarizeParams.h"
#include "Generated/Nodes/TryllClassifyAttributesLLMParams.h"
#include "CodegenFingerprint.gen.h"

#include <string_view>
#include "TryllNodeParamsFactory.generated.h"

namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllNodeParamsFactory = void;
}

/**
 * Blueprint-callable factory for constructing concrete node params objects.
 *
 * Allows Blueprints to create typed params for ChangeParams() and graph authoring
 * without any per-project C++ glue.
 *
 * Usage (Blueprint):
 *   MakeGenerateParams → set fields → Agent.ChangeParams(NodeName, Params)
 *   CloneParams(Baseline) → set one field → Agent.ChangeParams(NodeName, Cloned)
 */
UCLASS(meta=(ScriptName="TryllNodeParamsFactory"))
class TRYLLCLIENT_API UTryllNodeParamsFactory : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    /** Construct an authored GenerateParams with schema-default field values. */
    UFUNCTION(BlueprintCallable, Category="Tryll|Params",
              meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
    static UTryllGenerateParams* MakeGenerateParams(
        UObject* WorldContextObject);

    /** Construct an authored RegexGuardrailParams with schema-default field values. */
    UFUNCTION(BlueprintCallable, Category="Tryll|Params",
              meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
    static UTryllRegexGuardrailParams* MakeRegexGuardrailParams(
        UObject* WorldContextObject);

    /** Construct an authored CannedResponseParams with schema-default field values. */
    UFUNCTION(BlueprintCallable, Category="Tryll|Params",
              meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
    static UTryllCannedResponseParams* MakeCannedResponseParams(
        UObject* WorldContextObject);

    /** Construct an authored ToolCallParams with schema-default field values. */
    UFUNCTION(BlueprintCallable, Category="Tryll|Params",
              meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
    static UTryllToolCallParams* MakeToolCallParams(
        UObject* WorldContextObject);

    /** Construct an authored RetrieveParams with schema-default field values. */
    UFUNCTION(BlueprintCallable, Category="Tryll|Params",
              meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
    static UTryllRetrieveParams* MakeRetrieveParams(
        UObject* WorldContextObject);

    /** Construct an authored InstructionParams with schema-default field values. */
    UFUNCTION(BlueprintCallable, Category="Tryll|Params",
              meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
    static UTryllInstructionParams* MakeInstructionParams(
        UObject* WorldContextObject);

    /** Construct an authored ClassifyIntentParams with schema-default field values. */
    UFUNCTION(BlueprintCallable, Category="Tryll|Params",
              meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
    static UTryllClassifyIntentParams* MakeClassifyIntentParams(
        UObject* WorldContextObject);

    /** Construct an authored ClassifyIntentLLMParams with schema-default field values. */
    UFUNCTION(BlueprintCallable, Category="Tryll|Params",
              meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
    static UTryllClassifyIntentLLMParams* MakeClassifyIntentLLMParams(
        UObject* WorldContextObject);

    /** Construct an authored IntentToInstructionParams with schema-default field values. */
    UFUNCTION(BlueprintCallable, Category="Tryll|Params",
              meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
    static UTryllIntentToInstructionParams* MakeIntentToInstructionParams(
        UObject* WorldContextObject);

    /** Construct an authored GenerateAndSpeakParams with schema-default field values. */
    UFUNCTION(BlueprintCallable, Category="Tryll|Params",
              meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
    static UTryllGenerateAndSpeakParams* MakeGenerateAndSpeakParams(
        UObject* WorldContextObject);

    /** Construct an authored SpeakParams with schema-default field values. */
    UFUNCTION(BlueprintCallable, Category="Tryll|Params",
              meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
    static UTryllSpeakParams* MakeSpeakParams(
        UObject* WorldContextObject);

    /** Construct an authored PauseParams with schema-default field values. */
    UFUNCTION(BlueprintCallable, Category="Tryll|Params",
              meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
    static UTryllPauseParams* MakePauseParams(
        UObject* WorldContextObject);

    /** Construct an authored TransformParams with schema-default field values. */
    UFUNCTION(BlueprintCallable, Category="Tryll|Params",
              meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
    static UTryllTransformParams* MakeTransformParams(
        UObject* WorldContextObject);

    /** Construct an authored BranchParams with schema-default field values. */
    UFUNCTION(BlueprintCallable, Category="Tryll|Params",
              meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
    static UTryllBranchParams* MakeBranchParams(
        UObject* WorldContextObject);

    /** Construct an authored WriteToMemoryParams with schema-default field values. */
    UFUNCTION(BlueprintCallable, Category="Tryll|Params",
              meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
    static UTryllWriteToMemoryParams* MakeWriteToMemoryParams(
        UObject* WorldContextObject);

    /** Construct an authored SummarizeParams with schema-default field values. */
    UFUNCTION(BlueprintCallable, Category="Tryll|Params",
              meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
    static UTryllSummarizeParams* MakeSummarizeParams(
        UObject* WorldContextObject);

    /** Construct an authored ClassifyAttributesLLMParams with schema-default field values. */
    UFUNCTION(BlueprintCallable, Category="Tryll|Params",
              meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
    static UTryllClassifyAttributesLLMParams* MakeClassifyAttributesLLMParams(
        UObject* WorldContextObject);

    /** Deep-clone any UTryllNodeParamsBase subclass.
     *  Typical mutation idiom:
     *    Clone = CloneParams(Agent->GetNodeParamsBaseline(NodeName))
     *    // set fields on Clone
     *    Agent->ChangeParams(NodeName, Clone)
     */
    UFUNCTION(BlueprintCallable, Category="Tryll|Params",
              meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
    static UTryllNodeParamsBase* CloneParams(
        UTryllNodeParamsBase* Source,
        UObject* WorldContextObject);
};
