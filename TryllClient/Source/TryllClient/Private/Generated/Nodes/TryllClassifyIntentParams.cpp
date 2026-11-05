// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/nodes.fbs  +  tools/codegen/templates/unreal_node.cpp.j2
// See: docs/dev/codegen.md
#include "Generated/Nodes/TryllClassifyIntentParams.h"
#include "Generated/Nodes/TryllNodeParamsBase.h"
#include "TryllRuntimeSettings.h"
#include <flatbuffers/flatbuffers.h>
#include "nodes_generated.h"

#include "CodegenFingerprint.gen.h"

#include <string_view>
namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllClassifyIntentParams = void;
}
flatbuffers::Offset<void> UTryllClassifyIntentParams::Pack(
    flatbuffers::FlatBufferBuilder& Fbb,
    ::Tryll::NodeParams::NodeParams& OutType) const
{
    using namespace ::Tryll::NodeParams;

    ClassifyIntentParamsT paramsT;

    paramsT.embedded_string_storage = std::string(TCHAR_TO_UTF8(*UTryllRuntimeSettings::NormalizeStorageRelativePath(EmbeddedStringStorage.Path)));
    paramsT.input = std::string(TCHAR_TO_UTF8(*Input));
    paramsT.metadata_field = std::string(TCHAR_TO_UTF8(*MetadataField));
    paramsT.threshold = Threshold;
    if (bOverrideFilter)
        paramsT.filter = std::string(TCHAR_TO_UTF8(*Filter));
    paramsT.notify_client = static_cast<Tryll::NotifyClient>(static_cast<uint8>(NotifyClient));
    paramsT.diagnostic_topk = DiagnosticTopk;
    paramsT.found_exit = std::string(TCHAR_TO_UTF8(*FoundExit));
    paramsT.not_found_exit = std::string(TCHAR_TO_UTF8(*NotFoundExit));

    OutType = NodeParams_ClassifyIntentParams;
    auto offset = ClassifyIntentParams::Pack(Fbb, &paramsT);
    return offset.Union();
}
