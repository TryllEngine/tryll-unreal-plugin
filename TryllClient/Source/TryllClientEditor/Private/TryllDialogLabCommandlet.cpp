// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllDialogLabCommandlet.h"

#include "TryllDialogLabBatch.h"
#include "TryllDialogLabBatchSummary.h"
#include "TryllEditorSubsystem.h"

#include "Containers/Ticker.h"
#include "Editor.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Parse.h"

DEFINE_LOG_CATEGORY_STATIC(LogTryllDialogLabCommandlet, Log, All);

UTryllDialogLabCommandlet::UTryllDialogLabCommandlet()
{
	IsClient        = false;
	IsEditor        = true;
	IsServer        = false;
	LogToConsole    = true;
	ShowErrorCount  = false;
}

int32 UTryllDialogLabCommandlet::Main(const FString& Params)
{
	FString ConfigPath;
	if (!FParse::Value(*Params, TEXT("tryllLabConfig="), ConfigPath) || ConfigPath.IsEmpty())
	{
		UE_LOG(LogTryllDialogLabCommandlet, Error, TEXT("Pass the run config with -tryllLabConfig=<path>."));
		return FTryllDialogLabBatchSummary::ExitRunFailed;
	}

	UTryllEditorSubsystem* Subsystem = GEditor != nullptr ? GEditor->GetEditorSubsystem<UTryllEditorSubsystem>() : nullptr;
	TSharedRef<FTryllDialogLabBatch> Batch = FTryllDialogLabBatch::FromFile(ConfigPath, Subsystem,
		[](const FString& Line) { UE_LOG(LogTryllDialogLabCommandlet, Display, TEXT("[TryllDialogLab] %s"), *Line); });
	Batch->Start();

	// A commandlet has no engine loop: pump the core ticker (the batch, the runner's watchdog and
	// the connection's timers) and the subsystem (the socket) until the run is done. The safety
	// limit sits above the turn watchdog: a batch job must never hang.
	const double Limit    = Batch->SafetyTimeoutSeconds();
	const double Deadline = Limit > 0.0 ? FPlatformTime::Seconds() + Limit : TNumericLimits<double>::Max();
	const float  DeltaSec = 0.05f;
	while (!Batch->IsDone())
	{
		if (FPlatformTime::Seconds() > Deadline)
		{
			UE_LOG(LogTryllDialogLabCommandlet, Error,
				TEXT("[TryllDialogLab] Dialog Lab: ERROR, the run passed its %.0f s safety limit (exit 2)"), Limit);
			return FTryllDialogLabBatchSummary::ExitRunFailed;
		}
		FTSTicker::GetCoreTicker().Tick(DeltaSec);
		if (Subsystem != nullptr)
		{
			Subsystem->Tick(DeltaSec);
		}
		GLog->FlushThreadedLogs();
		FPlatformProcess::Sleep(DeltaSec);
	}
	return Batch->GetExitCode();
}
