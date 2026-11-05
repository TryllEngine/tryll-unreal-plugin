// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#ifndef TRYLLCLIENT_API
#define TRYLLCLIENT_API
#endif

/** One stripped Agent Log field. Path is dotted from the message body. */
struct FTryllOmittedField
{
	std::string Path;
	std::int64_t Count = 0;
	std::int64_t Bytes = 0;
};

/**
 * JSON writer for the generated Agent Log payload visitor.
 * NaN and infinities are JSON strings. Invalid UTF-8 becomes U+FFFD.
 */
class TRYLLCLIENT_API FTryllJsonWriter
{
public:
	explicit FTryllJsonWriter(bool bIndent = false);

	void BeginObject();
	void EndObject();
	void BeginArray();
	void EndArray();
	void Key(std::string_view Name);
	void Null();
	void Bool(bool bValue);
	void Int64(std::int64_t Value);
	void UInt64(std::uint64_t Value);
	void Float(float Value);
	void Double(double Value);
	void String(std::string_view Value);
	void Raw(std::string_view Json);

	[[nodiscard]] static bool TryParseJson(std::string_view Text, std::string& OutReason);
	[[nodiscard]] const std::string& Utf8() const { return Text; }

private:
	struct FFrame
	{
		bool bArray = false;
		int Count = 0;
	};

	void Begin(char Open);
	void End(char Close);
	void Lead(bool bValue);
	void RawNumber(std::string_view Number);
	void WriteString(std::string_view Value);
	[[nodiscard]] int Depth() const { return static_cast<int>(Frames.size()); }

	std::string Text;
	bool bIndent = false;
	std::vector<FFrame> Frames;
};

/** (log_label) renderer. An empty reference drops the separators beside it. */
[[nodiscard]] std::string TryllFormatLogLabel(
	std::string_view Template,
	const std::vector<std::pair<std::string, std::string>>& Refs);
