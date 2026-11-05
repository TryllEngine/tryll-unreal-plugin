// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/nodes.fbs  +  tools/codegen/templates/unreal_node.cpp.j2
// See: docs/dev/codegen.md
#include "Generated/Nodes/TryllRetrieveParams.h"
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
using _TryllFpCheck_TryllRetrieveParams = void;
}
flatbuffers::Offset<void> UTryllRetrieveParams::Pack(
    flatbuffers::FlatBufferBuilder& Fbb,
    ::Tryll::NodeParams::NodeParams& OutType) const
{
    using namespace ::Tryll::NodeParams;

    RetrieveParamsT paramsT;

    paramsT.embedded_string_storage = std::string(TCHAR_TO_UTF8(*UTryllRuntimeSettings::NormalizeStorageRelativePath(EmbeddedStringStorage.Path)));
    paramsT.input = std::string(TCHAR_TO_UTF8(*Input));
    paramsT.top_k = TopK;
    paramsT.threshold = Threshold;
    paramsT.retrieval_mode = static_cast<Tryll::RetrievalMode>(static_cast<uint8>(RetrievalMode));
    paramsT.rrf_k = RrfK;
    if (bOverrideSource)
        paramsT.source = std::string(TCHAR_TO_UTF8(*Source));
    if (bOverrideFilter)
        paramsT.filter = std::string(TCHAR_TO_UTF8(*Filter));
    paramsT.found_exit = std::string(TCHAR_TO_UTF8(*FoundExit));
    paramsT.not_found_exit = std::string(TCHAR_TO_UTF8(*NotFoundExit));
    paramsT.storage_name = std::string(TCHAR_TO_UTF8(*StorageName));
    paramsT.schema = std::string(TCHAR_TO_UTF8(*Schema));
    paramsT.source_kind = static_cast<Tryll::KnowledgeSourceKind>(static_cast<uint8>(SourceKind));

    OutType = NodeParams_RetrieveParams;
    auto offset = RetrieveParams::Pack(Fbb, &paramsT);
    return offset.Union();
}
