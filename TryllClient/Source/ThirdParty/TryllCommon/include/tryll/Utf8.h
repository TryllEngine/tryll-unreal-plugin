// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include <cstddef>
#include <string_view>

namespace Tryll {

/// Longest prefix of @p s that forms complete, well-formed UTF-8 code points.
/// Stops before an incomplete trailing sequence or an invalid leading/continuation byte.
[[nodiscard]] inline std::size_t Utf8CompletePrefixLength(std::string_view s) noexcept
{
	std::size_t i = 0;
	const std::size_t n = s.size();
	while (i < n)
	{
		const unsigned char c = static_cast<unsigned char>(s[i]);
		std::size_t need = 1;
		if (c < 0x80)
		{
			++i;
			continue;
		}
		else if ((c & 0xE0) == 0xC0)
		{
			if (c < 0xC2)
				break;
			need = 2;
		}
		else if ((c & 0xF0) == 0xE0)
		{
			need = 3;
		}
		else if ((c & 0xF8) == 0xF0)
		{
			if (c > 0xF4)
				break;
			need = 4;
		}
		else
		{
			break;
		}

		if (i + need > n)
			break;

		bool ok = true;
		for (std::size_t j = 1; j < need; ++j)
		{
			const unsigned char cc = static_cast<unsigned char>(s[i + j]);
			if ((cc & 0xC0) != 0x80)
			{
				ok = false;
				break;
			}
		}
		if (!ok)
			break;

		if (need == 3)
		{
			const unsigned char c1 = static_cast<unsigned char>(s[i + 1]);
			if ((c == 0xE0 && c1 < 0xA0) || (c == 0xED && c1 >= 0xA0))
				break;
		}
		else if (need == 4)
		{
			const unsigned char c1 = static_cast<unsigned char>(s[i + 1]);
			if ((c == 0xF0 && c1 < 0x90) || (c == 0xF4 && c1 > 0x8F))
				break;
		}

		i += need;
	}
	return i;
}

} // namespace Tryll
