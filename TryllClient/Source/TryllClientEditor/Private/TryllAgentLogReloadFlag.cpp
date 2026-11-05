// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllAgentLogReloadFlag.h"

#include "HAL/PlatformProcess.h"
#include "Misc/ConfigCacheIni.h"

bool FTryllAgentLogReloadFlag::ShouldHonor(
	const bool bDiscard,
	const uint32 StoredProcessId,
	const uint32 CurrentProcessId)
{
	return bDiscard && StoredProcessId != 0 && StoredProcessId == CurrentProcessId;
}

void FTryllAgentLogReloadFlag::Write(const bool bDiscard)
{
	const uint32 Pid = FPlatformProcess::GetCurrentProcessId();
	GConfig->SetBool(Section, DiscardKey, bDiscard, GEditorPerProjectIni);
	GConfig->SetInt(Section, ProcessIdKey, static_cast<int32>(Pid), GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}

bool FTryllAgentLogReloadFlag::ConsumeIfCurrentProcess()
{
	bool bDiscard = false;
	int32 StoredPid = 0;
	GConfig->GetBool(Section, DiscardKey, bDiscard, GEditorPerProjectIni);
	GConfig->GetInt(Section, ProcessIdKey, StoredPid, GEditorPerProjectIni);
	if (!ShouldHonor(bDiscard, static_cast<uint32>(StoredPid), FPlatformProcess::GetCurrentProcessId()))
	{
		if (bDiscard)
		{
			GConfig->SetBool(Section, DiscardKey, false, GEditorPerProjectIni);
			GConfig->Flush(false, GEditorPerProjectIni);
		}
		return false;
	}

	GConfig->SetBool(Section, DiscardKey, false, GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
	return true;
}
