// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/nodes.fbs  +  tools/codegen/templates/unreal_builder.cpp.j2
// See: docs/dev/codegen.md
#include "TryllGraphBuilder.Nodes.h"
#include "TryllGraphDescription.h"

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
namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllGraphBuilder_Nodes = void;
}

FTryllGraphBuilder& AddGenerateNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllGenerateParams* Params)
{
    Builder.AddNode(MoveTemp(Name), Params);
    return Builder;
}

FTryllGraphBuilder& AddRegexGuardrailNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllRegexGuardrailParams* Params)
{
    Builder.AddNode(MoveTemp(Name), Params);
    return Builder;
}

FTryllGraphBuilder& AddCannedResponseNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllCannedResponseParams* Params)
{
    Builder.AddNode(MoveTemp(Name), Params);
    return Builder;
}

FTryllGraphBuilder& AddToolCallNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllToolCallParams* Params)
{
    Builder.AddNode(MoveTemp(Name), Params);
    return Builder;
}

FTryllGraphBuilder& AddRetrieveNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllRetrieveParams* Params)
{
    Builder.AddNode(MoveTemp(Name), Params);
    return Builder;
}

FTryllGraphBuilder& AddInstructionNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllInstructionParams* Params)
{
    Builder.AddNode(MoveTemp(Name), Params);
    return Builder;
}

FTryllGraphBuilder& AddClassifyIntentNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllClassifyIntentParams* Params)
{
    Builder.AddNode(MoveTemp(Name), Params);
    return Builder;
}

FTryllGraphBuilder& AddClassifyIntentLLMNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllClassifyIntentLLMParams* Params)
{
    Builder.AddNode(MoveTemp(Name), Params);
    return Builder;
}

FTryllGraphBuilder& AddIntentToInstructionNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllIntentToInstructionParams* Params)
{
    Builder.AddNode(MoveTemp(Name), Params);
    return Builder;
}

FTryllGraphBuilder& AddGenerateAndSpeakNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllGenerateAndSpeakParams* Params)
{
    Builder.AddNode(MoveTemp(Name), Params);
    return Builder;
}

FTryllGraphBuilder& AddSpeakNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllSpeakParams* Params)
{
    Builder.AddNode(MoveTemp(Name), Params);
    return Builder;
}

FTryllGraphBuilder& AddPauseNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllPauseParams* Params)
{
    Builder.AddNode(MoveTemp(Name), Params);
    return Builder;
}

FTryllGraphBuilder& AddTransformNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllTransformParams* Params)
{
    Builder.AddNode(MoveTemp(Name), Params);
    return Builder;
}

FTryllGraphBuilder& AddBranchNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllBranchParams* Params)
{
    Builder.AddNode(MoveTemp(Name), Params);
    return Builder;
}

FTryllGraphBuilder& AddWriteToMemoryNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllWriteToMemoryParams* Params)
{
    Builder.AddNode(MoveTemp(Name), Params);
    return Builder;
}

FTryllGraphBuilder& AddSummarizeNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllSummarizeParams* Params)
{
    Builder.AddNode(MoveTemp(Name), Params);
    return Builder;
}

FTryllGraphBuilder& AddClassifyAttributesLLMNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllClassifyAttributesLLMParams* Params)
{
    Builder.AddNode(MoveTemp(Name), Params);
    return Builder;
}

