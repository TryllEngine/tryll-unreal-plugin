// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/*.fbs + tools/codegen/agent_log_emit.py
// See: docs/dev/codegen.md

#pragma once

#include "TryllJsonWriter.h"
#include "messages_generated.h"

#include <cstdint>
#include <string>
#include <vector>

#include "CodegenFingerprint.gen.h"

#include <string_view>
namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllAgentLogJson_gen = void;
}

#ifndef TRYLLCLIENT_API
#define TRYLLCLIENT_API
#endif

namespace Tryll::AgentLog
{

TRYLLCLIENT_API void WriteMessage(FTryllJsonWriter& Writer, const std::uint8_t* Data, std::size_t Size,
    const std::vector<FTryllOmittedField>& Omitted);
TRYLLCLIENT_API void WriteLocal(FTryllJsonWriter& Writer, const std::uint8_t* Data, std::size_t Size,
    const std::vector<FTryllOmittedField>& Omitted);
TRYLLCLIENT_API [[nodiscard]] std::vector<std::uint8_t> StripMessage(const std::uint8_t* Data, std::size_t Size,
    std::vector<FTryllOmittedField>& Omitted);
TRYLLCLIENT_API [[nodiscard]] std::string Label(::Tryll::MessageBody Body, const std::uint8_t* Data, std::size_t Size);
TRYLLCLIENT_API void ReadScope(const std::uint8_t* Data, std::size_t Size, ::Tryll::MessageBody& Body,
    std::uint64_t& RequestId, std::uint64_t& AgentId, std::uint64_t& VoiceInputId);

struct FTryllLogField
{
    const char* Name;
    const char* Kind;
    const char* Doc;
    const char* DefaultText;
    const char* const* EnumNames;
    int EnumCount;
    const char* Log;
    const char* Hint;
    const char* TypeName;
    const char* ElementKind;
    const char* ElementType;
};

struct FTryllLogTable
{
    const char* Name;
    const char* Label;
    const char* Coalesce;
    const char* CoalesceField;
    bool HasNodeName;
    bool HasIsDelta;
    const FTryllLogField* Fields;
    int FieldCount;
};

TRYLLCLIENT_API [[nodiscard]] const FTryllLogTable* FindTable(const char* Name);

} // namespace Tryll::AgentLog

