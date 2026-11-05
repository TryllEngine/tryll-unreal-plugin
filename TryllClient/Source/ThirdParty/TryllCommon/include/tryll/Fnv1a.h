// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace Tryll {

/// 64-bit FNV-1a of @p data as 16 lowercase hex digits. A content key, not a
/// security hash.
[[nodiscard]] inline std::string Fnv1a64Hex(std::string_view data)
{
	std::uint64_t hash = 14695981039346656037ull;
	for (unsigned char c : data)
	{
		hash ^= static_cast<std::uint64_t>(c);
		hash *= 1099511628211ull;
	}
	constexpr char kDigits[] = "0123456789abcdef";
	std::string out(16, '0');
	for (int i = 15; i >= 0; --i, hash >>= 4)
		out[static_cast<std::size_t>(i)] = kDigits[hash & 0xF];
	return out;
}

} // namespace Tryll
