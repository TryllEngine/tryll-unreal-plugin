// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllError.h"
#include "Win32Undef.h"

#include <flatbuffers/flatbuffers.h>

#include <string>

namespace Tryll::Client::Snapshot {

/// String and error helpers shared by the snapshot codecs (TryllDialogSnapshot.cpp,
/// TryllMkbSnapshot.cpp). One copy here instead of one per file in an anonymous
/// namespace: UBT unity builds merge Private/*.cpp of a module into one TU, where the
/// two copies redefine each other (C2084, td-042). Each .cpp pulls these in with
/// using-declarations inside its anonymous namespace, which may repeat in one TU.
[[nodiscard]] inline std::string ToUtf8(const FString& Value)
{
	return TCHAR_TO_UTF8(*Value);
}

[[nodiscard]] inline FString FromUtf8(const flatbuffers::String* Value)
{
	if (Value == nullptr)
	{
		return FString();
	}
	return UTF8_TO_TCHAR(Value->c_str());
}

/// Empty FString -> null offset, so optional string fields stay absent on the wire.
[[nodiscard]] inline flatbuffers::Offset<flatbuffers::String> MakeFStringOpt(
	flatbuffers::FlatBufferBuilder& Fbb, const FString& Value)
{
	if (Value.IsEmpty())
	{
		return {};
	}
	const std::string Utf8 = ToUtf8(Value);
	return Fbb.CreateString(Utf8);
}

inline void Fail(FTryllError& OutError, ETryllErrorCode Code, const TCHAR* Message)
{
	OutError = FTryllError(Code, Message);
}

} // namespace Tryll::Client::Snapshot
