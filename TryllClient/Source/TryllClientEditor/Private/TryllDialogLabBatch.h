// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "TryllDialogLabBatchConfig.h"
#include "TryllDialogLabRunData.h"

class FTryllDialogLabRunner;
class UTryllEditorSubsystem;
class UTryllWorkflowAsset;

/**
 * One headless Dialog Lab run (Unity's TryllDialogLabBatch): one workflow-asset agent against a
 * Dialog Script or Suite for N seeds, through the shared editor connection. It drives itself
 * from FTSTicker, so the commandlet pumps the ticker and the fixture spec just waits. When done
 * it has written <run_id>.jsonl and <run_id>.summary.json and holds the exit code: 0 thresholds
 * met, 1 a threshold missed, 2 the run itself failed.
 */
class TRYLLCLIENTEDITOR_API FTryllDialogLabBatch : public TSharedFromThis<FTryllDialogLabBatch>
{
public:
	using FLog = TFunction<void(const FString&)>;

	/** Reads and parses the config file and loads its assets; a failure finishes the run at once. */
	static TSharedRef<FTryllDialogLabBatch> FromFile(const FString& ConfigPath, UTryllEditorSubsystem* Subsystem, FLog Log);

	/** The run from loaded assets: specs pass in-memory ones. */
	static TSharedRef<FTryllDialogLabBatch> FromLoaded(const FTryllDialogLabBatchConfig& Config,
		const UTryllWorkflowAsset* Workflow, const UObject* DialogSource, UTryllEditorSubsystem* Subsystem, FLog Log);

	~FTryllDialogLabBatch();

	/** Acquires the connection and starts the run; a no-op once done. */
	void Start();

	bool IsDone() const { return bDone; }
	int32 GetExitCode() const { return ExitCode; }
	/** Turns x the turn timeout plus startup room; 0 = no limit (no turn timeout). */
	double SafetyTimeoutSeconds() const;

	/** Relative output folders resolve against the project folder. */
	static FString OutputDirectory(const FTryllDialogLabBatchConfig& Config);
	/** "/Game/Dir/Asset" -> "/Game/Dir/Asset.Asset"; full object paths pass through. */
	static FString ObjectPathOf(const FString& AssetPath);

private:
	FTryllDialogLabBatch() = default;

	bool Prepare(const UTryllWorkflowAsset* Workflow, const UObject* DialogSource, FString& OutError);
	void HandleConnectionChanged(bool bConnected);
	void HandleCreateSessionComplete(bool bSuccess);
	bool Tick(float DeltaSeconds);
	void StartRunner();
	void HandleRunFinished(const FTryllDialogLabRunResult& Result);
	/** A run that produced no results: the verdict, and a summary file when there is a folder for it. */
	void Fail(const FString& Reason);
	void Finish(int32 Code);
	void Log(const FString& Line) const;

	FTryllDialogLabBatchConfig Config;
	bool bHaveConfig = false;
	FTryllDialogLabRunSnapshot Snapshot;
	TWeakObjectPtr<UTryllEditorSubsystem> Subsystem;
	FLog LogFn;

	TSharedPtr<FTryllDialogLabRunner> Runner;
	FTSTicker::FDelegateHandle TickHandle;
	bool   bStarted = false;
	bool   bAcquired = false;
	bool   bSessionRequested = false;
	bool   bDone = false;
	int32  ExitCode = 2;
	double ConnectDeadline = 0.0;
};
