// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "Knowledge/TryllKnowledgeConfig.h"

#include "Misc/ConfigCacheIni.h"

FTryllKnowledgeConfig FTryllKnowledgeConfig::Load()
{
	FTryllKnowledgeConfig Config;
	GConfig->GetFloat(IniSection, TEXT("StoreWidth"), Config.StoreWidth, GEditorPerProjectIni);
	GConfig->GetFloat(IniSection, TEXT("InspectorWidth"), Config.InspectorWidth, GEditorPerProjectIni);
	Config.StoreWidth = FMath::Max(Config.StoreWidth, FTryllEditorUiContract::KnowledgeStoreMinWidth);
	Config.InspectorWidth = FMath::Max(Config.InspectorWidth, FTryllEditorUiContract::KnowledgeInspectorMinWidth);
	return Config;
}

void FTryllKnowledgeConfig::Save() const
{
	GConfig->SetFloat(IniSection, TEXT("StoreWidth"), StoreWidth, GEditorPerProjectIni);
	GConfig->SetFloat(IniSection, TEXT("InspectorWidth"), InspectorWidth, GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}
