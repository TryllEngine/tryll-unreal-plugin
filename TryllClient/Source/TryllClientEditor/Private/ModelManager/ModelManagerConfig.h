// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllEditorUiContract.h"

/** User-per-project Model Manager column widths (Phase 9). */
struct FTryllModelManagerConfig
{
	static constexpr const TCHAR* IniSection = TEXT("TryllModelManager");

	float FilterWidth = FTryllEditorUiContract::ModelFilterDefaultWidth;
	float PropsWidth  = FTryllEditorUiContract::ModelPropsDefaultWidth;

	static FTryllModelManagerConfig Load();
	void Save() const;
};
