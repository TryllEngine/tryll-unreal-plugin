// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllManagedServerProcess.h"
#include "TryllClientModule.h"
#include "TryllLicence.h"
#include "TryllRuntimeSettings.h"

#include "CoreGlobals.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Sockets.h"
#include "SocketSubsystem.h"

namespace
{
// Process exit code the server returns when it cannot bind its listen port because the
// address is already in use (EADDRINUSE). Must match Tryll::ServerExitPortInUse in
// tryll/common/include/tryll/Protocol.h. Lets the caller distinguish a bind collision
// (worth retrying on a fresh port) from any other early exit.
constexpr int32 ServerExitPortInUse = 3;

const TCHAR* TelemetryLevelToken(ETryllTelemetryLevel Level)
{
	switch (Level)
	{
		case ETryllTelemetryLevel::Turns: return TEXT("turns");
		case ETryllTelemetryLevel::Full:  return TEXT("full");
		default:                          return TEXT("basic");
	}
}

/**
 * Poll Port until it accepts a TCP connection or TimeoutSeconds elapses, checking Proc for
 * early exit on every iteration. Returns true once the port is reachable while the process
 * is still running. On false, bOutExited/OutReturnCode report whether the process had
 * already exited (and with what code) as opposed to a genuine timeout.
 */
bool WaitForTcpReady(
	FProcHandle& Proc, int32 Port, double TimeoutSeconds, bool& bOutExited, int32& OutReturnCode)
{
	bOutExited    = false;
	OutReturnCode = 0;

	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	const double Deadline = FPlatformTime::Seconds() + TimeoutSeconds;
	while (FPlatformTime::Seconds() < Deadline)
	{
		if (!FPlatformProcess::IsProcRunning(Proc))
		{
			bOutExited = true;
			FPlatformProcess::GetProcReturnCode(Proc, &OutReturnCode);
			return false;
		}

		if (SocketSubsystem)
		{
			if (FSocket* Socket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("TryllReadinessProbe"), false))
			{
				bool bIsValidIp = false;
				TSharedRef<FInternetAddr> Addr = SocketSubsystem->CreateInternetAddr();
				Addr->SetIp(TEXT("127.0.0.1"), bIsValidIp);
				Addr->SetPort(Port);
				Socket->SetNonBlocking(true);
				Socket->Connect(*Addr);
				const bool bConnected =
					Socket->Wait(ESocketWaitConditions::WaitForWrite, FTimespan::FromMilliseconds(200))
					&& Socket->GetConnectionState() == SCS_Connected;
				Socket->Close();
				SocketSubsystem->DestroySocket(Socket);

				if (bConnected)
				{
					// A process that raced to exit right as the connect succeeded must not
					// be reported as ready — re-check before returning true.
					if (!FPlatformProcess::IsProcRunning(Proc))
					{
						bOutExited = true;
						FPlatformProcess::GetProcReturnCode(Proc, &OutReturnCode);
						return false;
					}
					return true;
				}
			}
		}

		FPlatformProcess::Sleep(0.1f);
	}
	return false; // timed out while still running; bOutExited stays false
}
} // namespace

const TCHAR* const FTryllManagedServerProcess::ServerModeEnvVar = TEXT("TRYLL_SERVER_MODE");
const TCHAR* const FTryllManagedServerProcess::TryllDevMode     = TEXT("tryll-dev");
const TCHAR* const FTryllManagedServerProcess::ClientDevMode    = TEXT("client-dev");
FString FTryllManagedServerProcess::DeveloperModeOverride;

FString FTryllManagedServerProcess::NormalizeLaunchMode(const FString& Value)
{
	const FString Trimmed = Value.TrimStartAndEnd();
	if (Trimmed.Equals(TryllDevMode, ESearchCase::IgnoreCase))
	{
		return TryllDevMode;
	}
	if (Trimmed.Equals(ClientDevMode, ESearchCase::IgnoreCase))
	{
		return ClientDevMode;
	}
	return FString();
}

FString FTryllManagedServerProcess::ResolveLaunchMode()
{
#if !WITH_EDITOR
	return TEXT("client-prod");
#else
	const FString EnvRaw = FPlatformMisc::GetEnvironmentVariable(ServerModeEnvVar);
	const FString EnvMode = NormalizeLaunchMode(EnvRaw);
	if (!EnvMode.IsEmpty())
	{
		return EnvMode;
	}
	if (!EnvRaw.TrimStartAndEnd().IsEmpty())
	{
		static FString WarnedValue;
		if (WarnedValue != EnvRaw)
		{
			WarnedValue = EnvRaw;
			UE_LOG(LogTryll, Warning, TEXT("[Tryll] %s='%s' is not '%s' or '%s'; ignoring it."),
				ServerModeEnvVar, *EnvRaw, TryllDevMode, ClientDevMode);
		}
	}
	const FString Developer = NormalizeLaunchMode(DeveloperModeOverride);
	return Developer.IsEmpty() ? FString(ClientDevMode) : Developer;
#endif
}

void FTryllManagedServerProcess::SetDeveloperModeOverride(const FString& Mode)
{
	DeveloperModeOverride = NormalizeLaunchMode(Mode);
}

const TCHAR* const FTryllManagedServerProcess::ServerNeedsLicenceMessage =
	TEXT("Accept the Tryll Runtime Licence in the Tryll Welcome window first.");

bool FTryllManagedServerProcess::ShouldBlockLaunch(
	const FString& Mode, bool bLicenceAccepted, bool bNonInteractive)
{
	if (bLicenceAccepted || bNonInteractive)
	{
		return false;
	}
	return Mode != TryllDevMode && Mode != TEXT("client-prod");
}

bool FTryllManagedServerProcess::IsNonInteractiveRun()
{
	return IsRunningCommandlet() || FApp::IsUnattended() || GIsAutomationTesting;
}

bool FTryllManagedServerProcess::IsLaunchBlockedByLicence()
{
#if WITH_EDITOR
	return ShouldBlockLaunch(ResolveLaunchMode(), FTryllLicence::IsAccepted(), IsNonInteractiveRun());
#else
	return false;
#endif
}

FTryllManagedServerProcess::~FTryllManagedServerProcess()
{
	ReleaseHandle();
}

bool FTryllManagedServerProcess::IsRunning()
{
	return ServerProcess.IsValid() && FPlatformProcess::IsProcRunning(ServerProcess);
}

bool FTryllManagedServerProcess::Spawn(
	const FString& ExePath, int32 Port, int32 IdleShutdownTimeoutSeconds,
	double ReadinessTimeoutSeconds, const FString& LlamaCppBackend,
	FString& OutError, bool& bOutPortConflict)
{
	OutError.Reset();
	bOutPortConflict = false;

	if (!FPlatformProcess::CreatePipe(StdoutRead, StdoutWrite))
	{
		StdoutRead = StdoutWrite = nullptr;
	}

	const FString Backend = LlamaCppBackend.IsEmpty() ? TEXT("vulkan") : LlamaCppBackend;

	const FString Mode = ResolveLaunchMode();
	const UTryllRuntimeSettings* Settings = GetDefault<UTryllRuntimeSettings>();

	FString Args = FString::Printf(
		TEXT("--port %d --idle-shutdown-timeout %d --llama-backend %s --allow-absolute-save-paths --mode %s"),
		Port, IdleShutdownTimeoutSeconds, *Backend, *Mode);
	if (Mode == TryllDevMode)
	{
		Args += TEXT(" --telemetry-level off");
	}
	else if (Mode == ClientDevMode)
	{
		const ETryllTelemetryLevel Level = Settings
			? Settings->TelemetryLevel
			: ETryllTelemetryLevel::Basic;
		Args += FString::Printf(TEXT(" --telemetry-level %s"), TelemetryLevelToken(Level));
	}
	if (Mode == TEXT("client-prod"))
	{
		const FString LogDir = FPaths::ConvertRelativePathToFull(
			FPaths::ProjectSavedDir() / TEXT("Tryll") / TEXT("logs"));
		FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*LogDir);
		Args += FString::Printf(TEXT(" --log-dir \"%s\""), *LogDir);
	}
	if (Settings)
	{
		const FString ProjectName = Settings->ResolveProjectName();
		if (!ProjectName.IsEmpty())
		{
			Args += FString::Printf(TEXT(" --project-name \"%s\""), *ProjectName);
		}

		const FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
		if (!ProjectDir.IsEmpty())
		{
			Args += FString::Printf(TEXT(" --project-dir \"%s\""), *ProjectDir);
		}

		if (Mode == TEXT("client-dev"))
		{
			if (UTryllRuntimeSettings* Mutable = GetMutableDefault<UTryllRuntimeSettings>())
			{
				const FString ProjectId = Mutable->EnsureProjectId();
				if (!ProjectId.IsEmpty())
				{
					Args += FString::Printf(TEXT(" --project-id %s"), *ProjectId);
				}
			}
		}

		const FString StorageRoot = DebugStorageRoot.IsEmpty()
			? Settings->ResolveStorageDataFolderForServer()
			: DebugStorageRoot;
		if (!StorageRoot.IsEmpty())
		{
			Args += FString::Printf(TEXT(" --storage-root \"%s\""), *StorageRoot);
		}

		const FString SaveRoot = DebugSaveRoot.IsEmpty()
			? Settings->ResolveSaveDataFolderForLaunch()
			: DebugSaveRoot;
		if (!SaveRoot.IsEmpty())
		{
			Args += FString::Printf(TEXT(" --save-root \"%s\""), *SaveRoot);
		}

		FString UserCatalog = bDebugNoUserCatalogs ? FString() : Settings->ResolveUserModelsCatalogForLaunch();
		if (!UserCatalog.IsEmpty() && !FPaths::FileExists(UserCatalog))
		{
			UE_LOG(LogTryll, Warning,
				TEXT("Custom models catalog '%s' was not found. The server will start without it."),
				*Settings->UserModelsCatalogPath);
			UserCatalog.Reset();
		}
		if (!UserCatalog.IsEmpty())
		{
			Args += FString::Printf(TEXT(" --user-models-path \"%s\""), *UserCatalog);
		}
	}
	{
		const FString UserModelsDir = FPaths::ConvertRelativePathToFull(
			FPaths::ProjectDir() / TEXT("Tryll") / TEXT("models.d"));
		if (!bDebugNoUserCatalogs && FPaths::DirectoryExists(UserModelsDir))
		{
			Args += FString::Printf(TEXT(" --user-models-dir \"%s\""), *UserModelsDir);
		}
	}
	if (!DebugExtraCliArgs.IsEmpty())
	{
		Args += TEXT(" ");
		Args += DebugExtraCliArgs;
	}

	// Windows CreateProcess rejects a file-symlink as the working directory
	// (ERROR_DIRECTORY). Resolve through the reparse point so a checkout that
	// materialized Release as SYMLINK rather than SYMLINKD still launches.
	FString LaunchExe = ExePath;
	FString WorkDir   = FPaths::GetPath(ExePath);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	const FString DiskExe = PlatformFile.GetFilenameOnDisk(*ExePath);
	if (!DiskExe.IsEmpty() && PlatformFile.FileExists(*DiskExe))
	{
		LaunchExe = DiskExe;
		WorkDir   = FPaths::GetPath(DiskExe);
	}
	else if (!PlatformFile.DirectoryExists(*WorkDir))
	{
		WorkDir.Reset();
	}

	constexpr int32 MaxWindowsLaunchCodeUnits = 30000;
	const int32 LaunchCodeUnits = LaunchExe.Len() + 1 + Args.Len();
	if (LaunchCodeUnits > MaxWindowsLaunchCodeUnits)
	{
		OutError = FString::Printf(
			TEXT("Server launch command is %d UTF-16 code units, above the %d limit."),
			LaunchCodeUnits, MaxWindowsLaunchCodeUnits);
		if (StdoutRead || StdoutWrite)
		{
			FPlatformProcess::ClosePipe(StdoutRead, StdoutWrite);
			StdoutRead = StdoutWrite = nullptr;
		}
		return false;
	}

	ServerProcess = FPlatformProcess::CreateProc(
		*LaunchExe, *Args, true, true, true,
		nullptr, 0, WorkDir.IsEmpty() ? nullptr : *WorkDir,
		StdoutWrite, nullptr);

	if (!ServerProcess.IsValid())
	{
		OutError = FString::Printf(TEXT("failed to spawn '%s'."), *ExePath);
		if (StdoutRead || StdoutWrite)
		{
			FPlatformProcess::ClosePipe(StdoutRead, StdoutWrite);
			StdoutRead = StdoutWrite = nullptr;
		}
		return false;
	}

	UE_LOG(LogTryll, Log, TEXT("Spawned tryll server: %s (port %d), waiting for it to become ready..."),
		*ExePath, Port);

	bool  bExited    = false;
	int32 ReturnCode = 0;
	const bool bReady = WaitForTcpReady(ServerProcess, Port, ReadinessTimeoutSeconds, bExited, ReturnCode);

	// A genuine launch failure — the process exited on its own before opening the port
	// (e.g. a port collision, exit code 3) — is the only fatal outcome.
	if (!bReady && bExited)
	{
		bOutPortConflict = (ReturnCode == ServerExitPortInUse);
		OutError = FString::Printf(
			TEXT("server process exited with code %d before port %d opened."), ReturnCode, Port);
		ReleaseHandle();
		return false;
	}

	// Otherwise proceed to connect. Either the port is already accepting, or the readiness
	// wait elapsed while the process is still alive — a cold start still warming up (DLL
	// paging + first-run antivirus scan of the freshly built exe). We deliberately do NOT
	// kill it: the connection worker retries until it is ready, and killing here would throw
	// away the warmup and force a cold re-spawn (the "first launch hangs, relaunch works" bug).
	bObservedReady = bReady;
	if (bReady)
	{
		UE_LOG(LogTryll, Log, TEXT("Tryll server ready on port %d."), Port);
	}
	else
	{
		UE_LOG(LogTryll, Warning,
			TEXT("Tryll server not yet accepting on port %d after %.0fs; proceeding — the client "
			     "will keep retrying while the server warms up."), Port, ReadinessTimeoutSeconds);
	}
	if (StdoutRead)
	{
		DrainTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateRaw(this, &FTryllManagedServerProcess::DrainOutput), 0.1f);
	}
	return true;
}

void FTryllManagedServerProcess::ReleaseHandle()
{
	if (DrainTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(DrainTickerHandle);
		DrainTickerHandle.Reset();
	}
	if (ServerProcess.IsValid())
	{
		// Not TerminateProc: the server self-exits via its idle-shutdown timer.
		FPlatformProcess::CloseProc(ServerProcess);
		ServerProcess.Reset();
	}
	if (StdoutRead || StdoutWrite)
	{
		FPlatformProcess::ClosePipe(StdoutRead, StdoutWrite);
		StdoutRead  = nullptr;
		StdoutWrite = nullptr;
	}
	LogLineBuffer.Empty();
	bObservedReady = false;
}

void FTryllManagedServerProcess::TerminateAndRelease()
{
	if (ServerProcess.IsValid())
	{
		FPlatformProcess::TerminateProc(ServerProcess);
	}
	ReleaseHandle();
}

bool FTryllManagedServerProcess::DrainOutput(float /*DeltaTime*/)
{
	if (!StdoutRead)
	{
		return false;
	}

	const FString Chunk = FPlatformProcess::ReadPipe(StdoutRead);
	if (Chunk.IsEmpty())
	{
		return true;
	}

	LogLineBuffer += Chunk;

	int32 Newline = INDEX_NONE;
	while (LogLineBuffer.FindChar(TEXT('\n'), Newline))
	{
		FString Line = LogLineBuffer.Left(Newline);
		LogLineBuffer.RightChopInline(Newline + 1, EAllowShrinking::No);
		Line.TrimEndInline();
		if (Line.IsEmpty())
		{
			continue;
		}
		// Server warnings and errors are raised to Warning so they stand out in the
		// Output Log (yellow, counted). Not Error: many server errors are per-request
		// failures the caller already gets as an error response. Automation keeps Log,
		// since UE elevates any warning during a test to a test failure.
		if (!GIsAutomationTesting && ClassifyLine(Line) != ELineSeverity::Normal)
		{
			UE_LOG(LogTryll, Warning, TEXT("[Server] %s"), *Line);
		}
		else
		{
			UE_LOG(LogTryll, Log, TEXT("[Server] %s"), *Line);
		}
	}
	return true;
}

FTryllManagedServerProcess::ELineSeverity FTryllManagedServerProcess::ClassifyLine(const FString& Line)
{
	// The server's spdlog pattern is "[%H:%M:%S.%e] [%l] %v" (ServerLogging.cpp).
	int32 TimeEnd = INDEX_NONE;
	if (!Line.StartsWith(TEXT("[")) || !Line.FindChar(TEXT(']'), TimeEnd)
		|| Line.Mid(TimeEnd, 3) != TEXT("] ["))
	{
		return ELineSeverity::Normal;
	}
	const int32 LevelStart = TimeEnd + 3;
	const int32 LevelEnd = Line.Find(TEXT("]"), ESearchCase::CaseSensitive, ESearchDir::FromStart, LevelStart);
	if (LevelEnd == INDEX_NONE)
	{
		return ELineSeverity::Normal;
	}
	const FString Level = Line.Mid(LevelStart, LevelEnd - LevelStart);
	if (Level.Equals(TEXT("error"), ESearchCase::CaseSensitive)
		|| Level.Equals(TEXT("critical"), ESearchCase::CaseSensitive))
	{
		return ELineSeverity::Error;
	}
	if (Level.Equals(TEXT("warning"), ESearchCase::CaseSensitive))
	{
		return ELineSeverity::Warning;
	}
	return ELineSeverity::Normal;
}
