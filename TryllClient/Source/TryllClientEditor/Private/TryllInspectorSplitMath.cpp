// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllInspectorSplitMath.h"

float FTryllInspectorSplitMath::ClampFixedPane(
	const float OuterWidth,
	const float StoredWidth,
	const float FixedMin,
	const float FlexMin,
	const float HandleWidth)
{
	if (OuterWidth <= 0.0f)
	{
		return StoredWidth;
	}
	const float MaxWidth = FMath::Max(FixedMin, OuterWidth - FlexMin - HandleWidth);
	return FMath::Clamp(StoredWidth, FixedMin, MaxWidth);
}

bool FTryllInspectorSplitMath::ShouldPersistWidth(const float Width, const float FixedMin)
{
	return Width >= FixedMin;
}
