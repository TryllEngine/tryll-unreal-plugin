// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllEditorBridge.h"

static ITryllEditorBridge* GTryllEditorBridge = nullptr;

void FTryllEditorBridge::Register(ITryllEditorBridge* Bridge)
{
	GTryllEditorBridge = Bridge;
}

void FTryllEditorBridge::Unregister(ITryllEditorBridge* Bridge)
{
	if (GTryllEditorBridge == Bridge)
	{
		GTryllEditorBridge = nullptr;
	}
}

ITryllEditorBridge* FTryllEditorBridge::Get()
{
	return GTryllEditorBridge;
}
