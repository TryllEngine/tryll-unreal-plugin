// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

/** Pure splitter clamp / collapse / persist policy. Host widgets apply DefaultHandleWidth. */
struct TRYLLCLIENTEDITOR_API FTryllInspectorSplitMath
{
	static constexpr float CollapseWidth = 0.0f;
	static constexpr float DefaultHandleWidth = 5.0f;

	static float ClampFixedPane(
		float OuterWidth, float StoredWidth, float FixedMin, float FlexMin, float HandleWidth);
	static bool ShouldPersistWidth(float Width, float FixedMin);
};
