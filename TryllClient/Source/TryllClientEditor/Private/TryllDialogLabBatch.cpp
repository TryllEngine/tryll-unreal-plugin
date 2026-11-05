// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllDialogLabBatch.h"

#include "TryllDialogLabBatchSummary.h"
#include "TryllDialogLabDialogues.h"
#include "TryllDialogLabExport.h"
#include "TryllDialogLabExperiment.h"
#include "TryllDialogLabRunValidation.h"
#include "TryllDialogLabRunner.h"
#include "TryllDialogScriptAsset.h"
#include "TryllEditorSubsystem.h"
#include "TryllRuntimeSettings.h"
#include "TryllWorkflowAsset.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogTryllDialogLab, Log, All);

namespace
{
	/** How long the server may take to launch, connect and configure a session. */
	constexpr double BatchConnectTimeoutSeconds = 180.0;
}

TSharedRef<FTryllDialogLabBatch> FTryllDialogLabBatch::FromFile(
	const FString& ConfigPath, UTryllEditorSubsystem* InSubsystem, FLog InLog)
{
	TSharedRef<FTryllDialogLabBatch> Batch = MakeShareable(new FTryllDialogLabBatch());
	Batch->Subsystem = InSubsystem;
	Batch->LogFn = MoveTemp(InLog);

	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *ConfigPath))
	{
		Batch->Fail(FString::Printf(TEXT("Cannot read config '%s'."), *ConfigPath));
		return Batch;
	}
	FString Error;
	if (!FTryllDialogLabBatchConfig::TryParse(Text, Batch->Config, Error))
	{
		Batch->Fail(FString::Printf(TEXT("%s: %s"), *ConfigPath, *Error));
		return Batch;
	}
	Batch->bHaveConfig = true;

	const UTryllWorkflowAsset* Workflow = LoadObject<UTryllWorkflowAsset>(nullptr, *ObjectPathOf(Batch->Config.Workflow));
	if (Workflow == nullptr)
	{
		Batch->Fail(FString::Printf(TEXT("agent.workflow: no Tryll Workflow asset at '%s'."), *Batch->Config.Workflow));
		return Batch;
	}
	const UObject* Source = LoadObject<UObject>(nullptr, *ObjectPathOf(Batch->Config.Dialogues));
	if (Source == nullptr)
	{
		Batch->Fail(FString::Printf(TEXT("dialogues: no asset at '%s'."), *Batch->Config.Dialogues));
		return Batch;
	}
	if (!Batch->Prepare(Workflow, Source, Error))
	{
		Batch->Fail(Error);
	}
	return Batch;
}

TSharedRef<FTryllDialogLabBatch> FTryllDialogLabBatch::FromLoaded(const FTryllDialogLabBatchConfig& InConfig,
	const UTryllWorkflowAsset* Workflow, const UObject* DialogSource, UTryllEditorSubsystem* InSubsystem, FLog InLog)
{
	TSharedRef<FTryllDialogLabBatch> Batch = MakeShareable(new FTryllDialogLabBatch());
	Batch->Config = InConfig;
	Batch->bHaveConfig = true;
	Batch->Subsystem = InSubsystem;
	Batch->LogFn = MoveTemp(InLog);
	FString Error;
	if (Workflow == nullptr)
	{
		Batch->Fail(TEXT("agent.workflow: no Tryll Workflow asset."));
	}
	else if (!Batch->Prepare(Workflow, DialogSource, Error))
	{
		Batch->Fail(Error);
	}
	return Batch;
}

FTryllDialogLabBatch::~FTryllDialogLabBatch()
{
	if (TickHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
	}
}

bool FTryllDialogLabBatch::Prepare(const UTryllWorkflowAsset* Workflow, const UObject* DialogSource, FString& OutError)
{
	if (Config.AgentName.IsEmpty())
	{
		Config.AgentName = Workflow->GetName();
	}
	TArray<const UTryllDialogScriptAsset*> Scripts;
	FString SourceError;
	if (!FTryllDialogLabDialogues::TryResolve(DialogSource, Scripts, SourceError))
	{
		OutError = TEXT("dialogues: ") + SourceError;
		return false;
	}

	TArray<FTryllVariableOverride> Overrides;
	if (!Config.TryBuildOverrides(Workflow->Variables, Overrides, OutError)
		|| !FTryllDialogLabRunValidation::TryValidateScripts(Scripts, OutError))
	{
		return false;
	}

	FTryllDialogLabVariant Variant;
	Variant.Label         = Config.AgentName;
	Variant.ModelOverride = Config.Model;
	FTryllLabRunAgent RunAgent;
	RunAgent.Name  = Config.AgentName;
	RunAgent.Graph = &Workflow->Graph;
	RunAgent.bMaintainDialogueHistory = Config.bMaintainDialogueHistory;
	if (!FTryllDialogLabRunValidation::TryValidateVariant(Variant, RunAgent, Scripts, OutError))
	{
		return false;
	}

	FTryllLabAgentSpec Spec;
	Spec.Workflow          = Workflow;
	Spec.Name              = Config.AgentName;
	Spec.VariableOverrides = Overrides;
	Spec.bMaintainDialogueHistory = Config.bMaintainDialogueHistory;
	Spec.Model             = Config.Model;
	Snapshot = FTryllDialogLabRunSnapshot::CaptureFromSpec(
		GetDefault<UTryllRuntimeSettings>(), Spec, Scripts, Config.Seeds, Config.BaseSeed);
	return true;
}

double FTryllDialogLabBatch::SafetyTimeoutSeconds() const
{
	const int32 Turns = Snapshot.TotalSteps() * Snapshot.Repeats;
	return Config.TurnTimeoutSeconds > 0.0 ? Turns * Config.TurnTimeoutSeconds + 300.0 : 0.0;
}

void FTryllDialogLabBatch::Start()
{
	if (bDone || bStarted)
	{
		return;
	}
	bStarted = true;
	UTryllEditorSubsystem* Sub = Subsystem.Get();
	if (Sub == nullptr)
	{
		Fail(TEXT("Shared editor subsystem unavailable"));
		return;
	}
	Log(FString::Printf(TEXT("Running %d generated turns: %d dialogue(s) x %d seed(s), agent '%s'."),
		Snapshot.PlannedGeneratedTurns(), Snapshot.Dialogues.Num(), Snapshot.Repeats, *Config.AgentName));

	Sub->OnConnectionChangedNative.AddSP(this, &FTryllDialogLabBatch::HandleConnectionChanged);
	Sub->OnCreateSessionCompleteNative.AddSP(this, &FTryllDialogLabBatch::HandleCreateSessionComplete);
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateSP(this, &FTryllDialogLabBatch::Tick), 0.1f);
	ConnectDeadline = FPlatformTime::Seconds() + BatchConnectTimeoutSeconds;

	bAcquired = true;
	Sub->AcquireConnection();
	// Join-late: another consumer may already have brought the shared connection up.
	if (!bDone && Sub->IsConnected())
	{
		HandleConnectionChanged(true);
	}
}

void FTryllDialogLabBatch::HandleConnectionChanged(const bool bConnected)
{
	UTryllEditorSubsystem* Sub = Subsystem.Get();
	if (bDone || Runner.IsValid() || Sub == nullptr)
	{
		return;
	}
	if (!bConnected)
	{
		// The owner retries a cold start on its own; only an exhausted budget is a failure.
		if (!Sub->HasConnectRetriesRemaining())
		{
			Fail(TEXT("Disconnected"));
		}
		return;
	}
	if (Sub->IsSessionReady())
	{
		StartRunner();
		return;
	}
	if (!bSessionRequested)
	{
		bSessionRequested = true;
		Log(TEXT("Configuring session…"));
		Sub->CreateSessionFromSettings();
	}
}

void FTryllDialogLabBatch::HandleCreateSessionComplete(const bool bSuccess)
{
	if (bDone || Runner.IsValid())
	{
		return;
	}
	if (!bSuccess)
	{
		Fail(TEXT("CreateSession failed"));
		return;
	}
	StartRunner();
}

bool FTryllDialogLabBatch::Tick(float)
{
	if (bDone)
	{
		TickHandle.Reset();
		return false;
	}
	UTryllEditorSubsystem* Sub = Subsystem.Get();
	if (!Runner.IsValid() && Sub != nullptr)
	{
		if (Sub->IsSessionReady())
		{
			StartRunner();
		}
		else if (Sub->GetOwnerPhase() == ETryllOwnerPhase::Error && !Sub->HasConnectRetriesRemaining())
		{
			const FString Detail = Sub->GetOwnerDetail();
			Fail(Detail.IsEmpty() ? FString(TEXT("Server connection failed")) : Detail);
		}
		else if (FPlatformTime::Seconds() > ConnectDeadline)
		{
			Fail(FString::Printf(TEXT("The server was not ready within %.0f s."), BatchConnectTimeoutSeconds));
		}
	}
	return !bDone;
}

void FTryllDialogLabBatch::StartRunner()
{
	if (bDone || Runner.IsValid())
	{
		return;
	}
	Runner = MakeShared<FTryllDialogLabRunner>();
	Runner->TurnTimeoutSeconds = Config.TurnTimeoutSeconds;
	TWeakPtr<FTryllDialogLabBatch> SelfWeak = AsShared();
	Runner->Start(Subsystem.Get(), Snapshot,
		FTryllDialogLabRunner::FOnProgress::CreateLambda([SelfWeak](int32 Done, int32 Total)
		{
			if (TSharedPtr<FTryllDialogLabBatch> Self = SelfWeak.Pin())
			{
				if (Done == Total || Done % 10 == 0)
				{
					Self->Log(FString::Printf(TEXT("%d/%d turns"), Done, Total));
				}
			}
		}),
		FTryllDialogLabRunner::FOnFinished::CreateSP(this, &FTryllDialogLabBatch::HandleRunFinished));
}

void FTryllDialogLabBatch::HandleRunFinished(const FTryllDialogLabRunResult& Result)
{
	if (bDone)
	{
		return; // stopped by Finish after a failure that already reported
	}
	const FTryllDialogLabBatchSummary Summary = FTryllDialogLabBatchSummary::Build(&Result, Config);
	const FString Dir = OutputDirectory(Config);
	const FString Base = FPaths::Combine(Dir, Snapshot.RunId);
	IFileManager::Get().MakeDirectory(*Dir, /*Tree*/ true);
	if (!FFileHelper::SaveStringToFile(FTryllDialogLabExport::ExportToJsonl(Result), *(Base + TEXT(".jsonl")),
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)
		|| !FFileHelper::SaveStringToFile(Summary.Json, *(Base + TEXT(".summary.json")),
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		bHaveConfig = false; // nowhere to write the failure summary either
		Fail(FString::Printf(TEXT("Cannot write results to '%s'."), *Dir));
		return;
	}
	Log(FString::Printf(TEXT("Results: %s.jsonl / .summary.json"), *Base));
	for (const FString& Line : Summary.Lines)
	{
		Log(Line);
	}
	Finish(Summary.ExitCode);
}

void FTryllDialogLabBatch::Fail(const FString& Reason)
{
	const FTryllDialogLabBatchSummary Summary = FTryllDialogLabBatchSummary::Build(
		nullptr, bHaveConfig ? Config : FTryllDialogLabBatchConfig(), Reason);
	if (bHaveConfig)
	{
		const FString Dir = OutputDirectory(Config);
		IFileManager::Get().MakeDirectory(*Dir, /*Tree*/ true);
		FFileHelper::SaveStringToFile(Summary.Json, *FPaths::Combine(Dir, TEXT("failed.summary.json")),
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}
	for (const FString& Line : Summary.Lines)
	{
		Log(Line);
	}
	Finish(Summary.ExitCode);
}

void FTryllDialogLabBatch::Finish(const int32 Code)
{
	if (bDone)
	{
		return;
	}
	bDone = true;
	ExitCode = Code;
	if (Runner.IsValid() && Runner->IsRunning())
	{
		Runner->Stop();
	}
	if (UTryllEditorSubsystem* Sub = Subsystem.Get())
	{
		Sub->OnConnectionChangedNative.RemoveAll(this);
		Sub->OnCreateSessionCompleteNative.RemoveAll(this);
		if (bAcquired)
		{
			// Release, never kill: the managed server self-exits after its idle grace.
			Sub->ReleaseConnection();
			bAcquired = false;
		}
	}
}

FString FTryllDialogLabBatch::OutputDirectory(const FTryllDialogLabBatchConfig& InConfig)
{
	const FString Dir = InConfig.OutputDir.IsEmpty() ? FString(TEXT("Build/DialogLab")) : InConfig.OutputDir;
	// ProjectDir() is itself relative to the binaries folder when the project path is; make it
	// full first so the result (and the "Results:" line) is a real absolute path.
	return FPaths::IsRelative(Dir)
		? FPaths::ConvertRelativePathToFull(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()), Dir)
		: Dir;
}

FString FTryllDialogLabBatch::ObjectPathOf(const FString& AssetPath)
{
	if (AssetPath.Contains(TEXT(".")))
	{
		return AssetPath;
	}
	return AssetPath + TEXT(".") + FPaths::GetBaseFilename(AssetPath);
}

void FTryllDialogLabBatch::Log(const FString& Line) const
{
	if (LogFn)
	{
		LogFn(Line);
	}
	else
	{
		UE_LOG(LogTryllDialogLab, Display, TEXT("%s"), *Line);
	}
}
