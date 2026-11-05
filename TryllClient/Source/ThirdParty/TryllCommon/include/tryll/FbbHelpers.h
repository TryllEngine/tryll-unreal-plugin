// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

// FlatBuffers builder / schema includes.
#include <flatbuffers/flatbuffers.h>
#include "messages_generated.h"

#include <string_view>

namespace Tryll {

// ── String helpers ────────────────────────────────────────────────────────────

/// Create a FlatBuffers string from \p sv, or return an empty offset (0 / null)
/// when \p sv is empty.  Avoids allocating an empty flatbuffers::String table
/// for optional fields that the schema allows to be absent.
///
/// Usage:
/// @code
///   auto name = MakeStringOpt(fbb, req->game_name);
/// @endcode
[[nodiscard]] inline flatbuffers::Offset<flatbuffers::String>
MakeStringOpt(flatbuffers::FlatBufferBuilder& fbb, std::string_view sv)
{
	return sv.empty()
		? flatbuffers::Offset<flatbuffers::String>{}
		: fbb.CreateString(sv);
}

} // namespace Tryll

// ── Request-id extraction ─────────────────────────────────────────────────────
// ExtractRequestId is generated from the schema; it already wraps itself in
// namespace Tryll, so it is included after the block above is closed.
#include "generated/RequestId.gen.h"
