// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllEditorUiContract.h"

struct FTryllKnowledgeConfig
{
	static constexpr const TCHAR* IniSection = TEXT("TryllKnowledge");

	float StoreWidth = FTryllEditorUiContract::KnowledgeStoreDefaultWidth;
	float InspectorWidth = FTryllEditorUiContract::KnowledgeInspectorDefaultWidth;

	static FTryllKnowledgeConfig Load();
	void Save() const;
};
