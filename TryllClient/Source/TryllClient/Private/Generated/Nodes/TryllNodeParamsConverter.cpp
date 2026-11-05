// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/nodes.fbs  +  tools/codegen/templates/unreal_converter.cpp.j2
// See: docs/dev/codegen.md
#include "Generated/Nodes/TryllNodeParamsBase.h"
#include "nodes_generated.h"
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
using _TryllFpCheck_TryllNodeParamsConverter = void;
}

namespace TryllNodeParamsConverter
{
    flatbuffers::Offset<void> PackNodeParams(
        flatbuffers::FlatBufferBuilder& Fbb,
        const UTryllNodeParamsBase* Params,
        ::Tryll::NodeParams::NodeParams& OutType)
    {
        if (!Params)
        {
            OutType = ::Tryll::NodeParams::NodeParams_NONE;
            return 0;
        }
        return Params->Pack(Fbb, OutType);
    }
}
