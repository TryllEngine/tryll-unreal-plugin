// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Win32Undef.h"

#include <flatbuffers/string.h>

#include <string>

namespace Tryll::Client {

/// Shared UTF-8 → FString helpers. Must live at `Tryll::Client` scope, not in
/// an anonymous namespace: UBT unity-builds concatenate Private/*.cpp of one
/// module into a single TU, and two `namespace { Utf8ToFString(...) }` collide
/// (C2084 / C2995). Adaptive unity hides that locally.
[[nodiscard]] inline FString Utf8ToFString(const char* Str, std::size_t Len)
{
	const std::string Tmp(Str, Len);
	return FString(UTF8_TO_TCHAR(Tmp.c_str()));
}

[[nodiscard]] inline FString Utf8ToFString(const flatbuffers::String* Str)
{
	if (!Str)
	{
		return {};
	}
	return Utf8ToFString(Str->c_str(), Str->size());
}

} // namespace Tryll::Client
