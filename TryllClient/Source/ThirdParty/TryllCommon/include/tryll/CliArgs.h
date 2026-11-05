// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include <string_view>

namespace Tryll {

/// Result of looking up a value-taking CLI flag such as `--llama-backend`.
///
/// Distinguishes "flag absent" from "flag present with no usable value" so a
/// trailing `--llama-backend` cannot silently keep the JSON default.
struct CliFlagValue
{
	bool present = false;
	bool missingValue = false;
	bool valueLooksLikeFlag = false;
	std::string_view value;
};

[[nodiscard]] inline CliFlagValue FindCliFlagValue(
	int argc, char* const argv[], std::string_view flag) noexcept
{
	for (int i = 1; i < argc; ++i)
	{
		if (std::string_view{argv[i]} != flag)
			continue;
		if (i + 1 >= argc)
			return {true, true, false, {}};
		const std::string_view v{argv[i + 1]};
		if (v.starts_with('-'))
			return {true, true, true, v};
		return {true, false, false, v};
	}
	return {};
}

} // namespace Tryll
