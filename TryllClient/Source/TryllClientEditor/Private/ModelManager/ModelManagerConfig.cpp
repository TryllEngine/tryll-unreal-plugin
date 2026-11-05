// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "ModelManager/ModelManagerConfig.h"

#include "Misc/ConfigCacheIni.h"

FTryllModelManagerConfig FTryllModelManagerConfig::Load()
{
	FTryllModelManagerConfig Config;
	GConfig->GetFloat(IniSection, TEXT("FilterWidth"), Config.FilterWidth, GEditorPerProjectIni);
	GConfig->GetFloat(IniSection, TEXT("PropsWidth"), Config.PropsWidth, GEditorPerProjectIni);
	Config.FilterWidth = FMath::Max(Config.FilterWidth, FTryllEditorUiContract::ModelFilterMinWidth);
	Config.PropsWidth  = FMath::Max(Config.PropsWidth, FTryllEditorUiContract::ModelPropsMinWidth);
	return Config;
}

void FTryllModelManagerConfig::Save() const
{
	GConfig->SetFloat(IniSection, TEXT("FilterWidth"), FilterWidth, GEditorPerProjectIni);
	GConfig->SetFloat(IniSection, TEXT("PropsWidth"), PropsWidth, GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}
