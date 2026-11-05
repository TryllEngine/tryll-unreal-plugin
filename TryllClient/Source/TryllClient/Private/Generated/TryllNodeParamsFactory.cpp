// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/nodes.fbs  +  tools/codegen/templates/unreal_bp_factory.cpp.j2
// See: docs/dev/codegen.md
#include "TryllNodeParamsFactory.h"

#include "CodegenFingerprint.gen.h"

#include <string_view>
namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllNodeParamsFactory = void;
}

UTryllGenerateParams* UTryllNodeParamsFactory::MakeGenerateParams(
    UObject* WorldContextObject)
{
    return NewObject<UTryllGenerateParams>(WorldContextObject);
}

UTryllRegexGuardrailParams* UTryllNodeParamsFactory::MakeRegexGuardrailParams(
    UObject* WorldContextObject)
{
    return NewObject<UTryllRegexGuardrailParams>(WorldContextObject);
}

UTryllCannedResponseParams* UTryllNodeParamsFactory::MakeCannedResponseParams(
    UObject* WorldContextObject)
{
    return NewObject<UTryllCannedResponseParams>(WorldContextObject);
}

UTryllToolCallParams* UTryllNodeParamsFactory::MakeToolCallParams(
    UObject* WorldContextObject)
{
    return NewObject<UTryllToolCallParams>(WorldContextObject);
}

UTryllRetrieveParams* UTryllNodeParamsFactory::MakeRetrieveParams(
    UObject* WorldContextObject)
{
    return NewObject<UTryllRetrieveParams>(WorldContextObject);
}

UTryllInstructionParams* UTryllNodeParamsFactory::MakeInstructionParams(
    UObject* WorldContextObject)
{
    return NewObject<UTryllInstructionParams>(WorldContextObject);
}

UTryllClassifyIntentParams* UTryllNodeParamsFactory::MakeClassifyIntentParams(
    UObject* WorldContextObject)
{
    return NewObject<UTryllClassifyIntentParams>(WorldContextObject);
}

UTryllClassifyIntentLLMParams* UTryllNodeParamsFactory::MakeClassifyIntentLLMParams(
    UObject* WorldContextObject)
{
    return NewObject<UTryllClassifyIntentLLMParams>(WorldContextObject);
}

UTryllIntentToInstructionParams* UTryllNodeParamsFactory::MakeIntentToInstructionParams(
    UObject* WorldContextObject)
{
    return NewObject<UTryllIntentToInstructionParams>(WorldContextObject);
}

UTryllGenerateAndSpeakParams* UTryllNodeParamsFactory::MakeGenerateAndSpeakParams(
    UObject* WorldContextObject)
{
    return NewObject<UTryllGenerateAndSpeakParams>(WorldContextObject);
}

UTryllSpeakParams* UTryllNodeParamsFactory::MakeSpeakParams(
    UObject* WorldContextObject)
{
    return NewObject<UTryllSpeakParams>(WorldContextObject);
}

UTryllPauseParams* UTryllNodeParamsFactory::MakePauseParams(
    UObject* WorldContextObject)
{
    return NewObject<UTryllPauseParams>(WorldContextObject);
}

UTryllTransformParams* UTryllNodeParamsFactory::MakeTransformParams(
    UObject* WorldContextObject)
{
    return NewObject<UTryllTransformParams>(WorldContextObject);
}

UTryllBranchParams* UTryllNodeParamsFactory::MakeBranchParams(
    UObject* WorldContextObject)
{
    return NewObject<UTryllBranchParams>(WorldContextObject);
}

UTryllWriteToMemoryParams* UTryllNodeParamsFactory::MakeWriteToMemoryParams(
    UObject* WorldContextObject)
{
    return NewObject<UTryllWriteToMemoryParams>(WorldContextObject);
}

UTryllSummarizeParams* UTryllNodeParamsFactory::MakeSummarizeParams(
    UObject* WorldContextObject)
{
    return NewObject<UTryllSummarizeParams>(WorldContextObject);
}

UTryllClassifyAttributesLLMParams* UTryllNodeParamsFactory::MakeClassifyAttributesLLMParams(
    UObject* WorldContextObject)
{
    return NewObject<UTryllClassifyAttributesLLMParams>(WorldContextObject);
}

UTryllNodeParamsBase* UTryllNodeParamsFactory::CloneParams(
    UTryllNodeParamsBase* Source,
    UObject* WorldContextObject)
{
    if (!Source) return nullptr;
    return DuplicateObject<UTryllNodeParamsBase>(Source, WorldContextObject);
}
