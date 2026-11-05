// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllPIEModelGuard.h"
#include "TryllEditorPrompts.h"
#include "TryllModelDownloadGuard.h"
#include "TryllEditorSubsystem.h"
#include "TryllRuntimeSettings.h"
#include "TryllClientModule.h"

#include "Editor.h"
#include "Features/IModularFeatures.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Misc/MessageDialog.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "TryllPIEModelGuard"

bool FTryllPIEModelGuard::bVerifiedReady           = false;
bool FTryllPIEModelGuard::bCheckInProgress         = false;
bool FTryllPIEModelGuard::bSubscribedToSubsystem   = false;
bool FTryllPIEModelGuard::bAcquiredConnection      = false;
FString FTryllPIEModelGuard::LastErrorMessage;
FTSTicker::FDelegateHandle FTryllPIEModelGuard::TickerHandle;
FTSTicker::FDelegateHandle FTryllPIEModelGuard::WatchdogTickerHandle;
FDelegateHandle FTryllPIEModelGuard::PostPIEStartedHandle;
FDelegateHandle FTryllPIEModelGuard::CancelPIEHandle;
FDelegateHandle FTryllPIEModelGuard::EndPIEHandle;
UTryllEditorSubsystem* FTryllPIEModelGuard::EditorSubsystem = nullptr;
TSharedPtr<FTryllModelDownloadGuard> FTryllPIEModelGuard::ModelGuard;
TWeakPtr<SNotificationItem> FTryllPIEModelGuard::ProgressNotification;
FTryllPIEModelGuard FTryllPIEModelGuard::Instance;

void FTryllPIEModelGuard::Register()
{
	IModularFeatures::Get().RegisterModularFeature(IPIEAuthorizer::GetModularFeatureName(), &Instance);
}

void FTryllPIEModelGuard::Unregister()
{
	IModularFeatures::Get().UnregisterModularFeature(IPIEAuthorizer::GetModularFeatureName(), &Instance);
	Cleanup();
	ReleaseHeldConnection();
}

void FTryllPIEModelGuard::ShowOrUpdateProgressNotification(const FText& Text)
{
	if (TSharedPtr<SNotificationItem> Item = ProgressNotification.Pin())
	{
		Item->SetText(Text);
		return;
	}

	FNotificationInfo Info(Text);
	Info.bFireAndForget = false;
	Info.FadeInDuration = 0.1f;
	Info.FadeOutDuration = 0.5f;
	Info.ExpireDuration = 0.0f;
	Info.bUseThrobber = true;
	Info.bUseSuccessFailIcons = true;

	const TSharedPtr<SNotificationItem> Item = FSlateNotificationManager::Get().AddNotification(Info);
	if (Item.IsValid())
	{
		Item->SetCompletionState(SNotificationItem::CS_Pending);
		ProgressNotification = Item;
	}
}

void FTryllPIEModelGuard::DismissProgressNotification(int32 CompletionState, const FText& FinalText)
{
	TSharedPtr<SNotificationItem> Item = ProgressNotification.Pin();
	ProgressNotification.Reset();
	if (!Item.IsValid())
	{
		return;
	}
	if (!FinalText.IsEmpty())
	{
		Item->SetText(FinalText);
	}
	Item->SetCompletionState(static_cast<SNotificationItem::ECompletionState>(CompletionState));
	Item->ExpireAndFadeout();
}

bool FTryllPIEModelGuard::RequestPIEPermissionImpl(FString& OutReason) const
{
	// Project Settings → Tryll Client → Editor: when the guard is disabled, let Play through
	// immediately with no pre-flight model check.
	if (!GetDefault<UTryllRuntimeSettings>()->bEnablePlayModeModelGuard)
		return true;

	if (bVerifiedReady)
		return true;

	if (!bCheckInProgress)
	{
		bCheckInProgress = true;
		ShowOrUpdateProgressNotification(LOCTEXT("CheckingToast",
			"Tryll: verifying required models — Play will resume when ready…"));
		// Deferred to the next tick rather than called inline: confirmed via debugger that
		// when the connection/session/models are already warm, this whole chain — down to
		// GEditor->RequestPlaySession() — can complete synchronously within this call stack,
		// reentrantly, while the engine's PlayLevel.cpp is still unwinding the *outer*
		// RequestPlaySession call. That starts a second, real PIE session which the outer
		// call's subsequent cancel then tears down, producing the observed open/close
		// flicker loop. A one-shot ticker breaks the call stack so RunGuardThenResume()
		// always starts fresh, outside any PIE-start frame.
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic([](float) -> bool
		{
			RunGuardThenResume();
			return false;
		}), 0.0f);
	}

	OutReason = TEXT("Tryll: verifying required models are downloaded…");
	return false;
}

#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 8)
TValueOrError<bool, FText> FTryllPIEModelGuard::IsPIEAuthorizedInternal(bool /*bIsSimulateInEditor*/) const
{
	// Silent query — must not start a check. The engine only calls RequestPIEPermissionInternal
	// when this returns true, so always allow here and gate in the request path.
	return MakeValue(true);
}

TValueOrError<bool, FText> FTryllPIEModelGuard::RequestPIEPermissionInternal(bool /*bIsSimulateInEditor*/) const
{
	FString Reason;
	if (RequestPIEPermissionImpl(Reason))
		return MakeValue(true);
	return MakeError(FText::FromString(Reason));
}
#else
bool FTryllPIEModelGuard::RequestPIEPermission(bool /*bIsSimulateInEditor*/, FString& OutReason) const
{
	return RequestPIEPermissionImpl(OutReason);
}
#endif

void FTryllPIEModelGuard::RunGuardThenResume()
{
	if (!GEditor)
	{
		bCheckInProgress = false;
		DismissProgressNotification(SNotificationItem::CS_Fail,
			LOCTEXT("NoEditorToast", "Tryll — cannot enter Play: no editor"));
		FTryllEditorPrompts::Open(EAppMsgType::Ok, LOCTEXT("NoEditor", "Tryll — cannot enter Play: no editor."));
		return;
	}

	EditorSubsystem = GEditor->GetEditorSubsystem<UTryllEditorSubsystem>();
	if (!EditorSubsystem)
	{
		bCheckInProgress = false;
		DismissProgressNotification(SNotificationItem::CS_Fail,
			LOCTEXT("NoSubsystemToast", "Tryll — cannot enter Play: subsystem unavailable"));
		FTryllEditorPrompts::Open(EAppMsgType::Ok, LOCTEXT("NoSubsystem",
			"Tryll — cannot enter Play: shared editor subsystem unavailable."));
		return;
	}

	ShowOrUpdateProgressNotification(LOCTEXT("ConnectingToast",
		"Tryll: connecting to server — Play will resume when ready…"));

	if (!bSubscribedToSubsystem)
	{
		bSubscribedToSubsystem = true;
		EditorSubsystem->OnConnectionChangedNative.AddStatic(&FTryllPIEModelGuard::HandleConnectionChanged);
		EditorSubsystem->OnCreateSessionCompleteNative.AddStatic(&FTryllPIEModelGuard::HandleCreateSessionComplete);
		EditorSubsystem->OnErrorNative.AddStatic(&FTryllPIEModelGuard::HandleError);
	}

	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float) -> bool
	{
		if (ModelGuard.IsValid()) ModelGuard->Tick();
		return true;
	}), 0.05f);

	// Arm the connect/session watchdog before acquiring. If the server wedges before a
	// session exists — never connects, or CreateSession gets no response — this fires and
	// turns what would be an indefinite Play veto into a clear error. It is disarmed the
	// moment a session outcome arrives (HandleCreateSessionComplete), so it is never live
	// during the model-download stage, which can legitimately run for minutes.
	WatchdogTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateStatic([](float) -> bool
		{
			WatchdogTickerHandle.Reset();
			if (bCheckInProgress)
			{
				const FString Err = LastErrorMessage.IsEmpty()
					? FString(TEXT("timed out establishing a connection/session with the Tryll server"))
					: LastErrorMessage;
				UE_LOG(LogTryll, Warning,
					TEXT("FTryllPIEModelGuard: watchdog fired (60s, no session yet) — aborting Play (%s)"), *Err);
				Cleanup();
				ReleaseHeldConnection();
				FTryllEditorPrompts::Open(EAppMsgType::Ok, FText::Format(
					LOCTEXT("WatchdogTimeout", "Tryll — cannot enter Play: {0}"),
					FText::FromString(Err)));
			}
			return false; // one-shot
		}),
		/*WatchdogSeconds=*/60.0f);

	UE_LOG(LogTryll, Log, TEXT("FTryllPIEModelGuard: begin verify — acquiring shared connection"));
	bAcquiredConnection = true;
	EditorSubsystem->AcquireConnection();

	// Join-late: another consumer (the chat panel, the model manager panel) may have
	// already brought the shared connection up before Play was pressed.
	if (EditorSubsystem->IsConnected())
	{
		HandleConnectionChanged(true);
	}
}

void FTryllPIEModelGuard::HandleConnectionChanged(bool bConnected)
{
	// These native delegates are shared by every consumer (this guard, the chat panel, the
	// model manager panel) and stay subscribed for the life of the editor process — so this
	// fires for ANY consumer's connect/session activity, not just ones this guard caused.
	// bCheckInProgress is only true while a Play attempt is actually driving a cycle this
	// guard started (set in RequestPIEPermission, cleared in Cleanup()); gating on it —
	// rather than bVerifiedReady — correctly ignores e.g. the model manager panel opening
	// and creating its own session while this guard is between Play attempts (confirmed via
	// debugger: opening Model Manager after a failed Play check re-triggered a second,
	// independent download the user had no way to decline).
	if (!bCheckInProgress) return;

	if (!bConnected)
	{
		// ConnectFailed is not terminal while the shared subsystem still has retry
		// budget — accept-late cold starts (fault_injection.pre_listen_delay_ms)
		// emit several of these before the server listens. The connect-retry loop
		// (and the 60s watchdog below) cover exhausted-budget / wedged-server cases.
		if (EditorSubsystem && EditorSubsystem->HasConnectRetriesRemaining())
		{
			UE_LOG(LogTryll, Log,
				TEXT("FTryllPIEModelGuard: connect failed (%s); waiting for retry…"),
				LastErrorMessage.IsEmpty() ? TEXT("Disconnected") : *LastErrorMessage);
			ShowOrUpdateProgressNotification(LOCTEXT("WaitingServerToast",
				"Tryll: waiting for server — Play will resume when ready…"));
			return;
		}

		const FString Err = LastErrorMessage;
		UE_LOG(LogTryll, Warning,
			TEXT("FTryllPIEModelGuard: connect failed with no retries left — aborting Play (%s)"),
			Err.IsEmpty() ? TEXT("Disconnected") : *Err);
		Cleanup();
		ReleaseHeldConnection();
		FTryllEditorPrompts::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("CannotEnterPlayConn", "Tryll — cannot enter Play: {0}"),
			FText::FromString(Err.IsEmpty() ? TEXT("Disconnected") : Err)));
		return;
	}
	if (!EditorSubsystem) return;

	UE_LOG(LogTryll, Log, TEXT("FTryllPIEModelGuard: connection ready — creating session"));
	ShowOrUpdateProgressNotification(LOCTEXT("CreatingSessionToast",
		"Tryll: creating session — Play will resume when ready…"));

	// ListModels is engine-agnostic since v13, so the guard no longer has to
	// declare engines it does not use just to see every model type.
	EditorSubsystem->CreateSession(FString());
	// If the session is already configured (join-late), CreateSession() above broadcasts
	// synchronously and HandleCreateSessionComplete() below already subscribed — no
	// separate check needed here.
}

void FTryllPIEModelGuard::HandleCreateSessionComplete(bool bSuccess)
{
	if (!bCheckInProgress) return;

	// A session outcome has arrived: the connect/session stage is over, so the watchdog
	// has done its job. Disarm it now — the model-download stage that may follow has its
	// own cancelable progress UI and can legitimately run far longer than the watchdog.
	DisarmWatchdog();

	if (!bSuccess)
	{
		const FString Err = LastErrorMessage;
		UE_LOG(LogTryll, Warning, TEXT("FTryllPIEModelGuard: CreateSession failed — aborting Play (%s)"), *Err);
		Cleanup();
		ReleaseHeldConnection();
		FTryllEditorPrompts::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("CannotEnterPlayConn", "Tryll — cannot enter Play: {0}"), FText::FromString(Err)));
		return;
	}
	if (!EditorSubsystem) return;
	if (ModelGuard.IsValid() && ModelGuard->IsActive()) return;   // already checking; ignore redundant broadcast

	UE_LOG(LogTryll, Log, TEXT("FTryllPIEModelGuard: session created — checking required models"));
	ShowOrUpdateProgressNotification(LOCTEXT("CheckingModelsToast",
		"Tryll: checking required models — Play will resume when ready…"));

	ModelGuard = MakeShared<FTryllModelDownloadGuard>();
	ModelGuard->Start(EditorSubsystem, FTryllModelDownloadGuard::FOnComplete::CreateStatic(&FTryllPIEModelGuard::OnModelsReady));
}

void FTryllPIEModelGuard::OnModelsReady(bool bSuccess, FString Error)
{
	UE_LOG(LogTryll, Log, TEXT("FTryllPIEModelGuard: model check complete — success=%d%s"),
		bSuccess ? 1 : 0,
		Error.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(" error='%s'"), *Error));

	if (bSuccess)
	{
		DismissProgressNotification(SNotificationItem::CS_Success,
			LOCTEXT("ReadyToast", "Tryll ready — starting Play"));
	}
	else if (!Error.IsEmpty())
	{
		DismissProgressNotification(SNotificationItem::CS_Fail,
			LOCTEXT("FailedToast", "Tryll — cannot enter Play"));
	}
	else
	{
		// User cancelled the download dialog — fade quietly.
		DismissProgressNotification(SNotificationItem::CS_None);
	}

	Cleanup();
	if (bSuccess)
	{
		bVerifiedReady = true;

		// Hold the connection acquisition past this point rather than releasing it here —
		// PIE's own UTryllSubsystem is about to acquire its own ref via Initialize(), and
		// releasing first would force it to reconnect from scratch. Release once that
		// handoff has definitely happened (PostPIEStarted) or definitely won't (CancelPIE /
		// EndPIE), whichever comes first — see the declarations for why these three cover it.
		PostPIEStartedHandle = FEditorDelegates::PostPIEStarted.AddStatic(&FTryllPIEModelGuard::OnPIEStartedOrEnded);
		EndPIEHandle         = FEditorDelegates::EndPIE.AddStatic(&FTryllPIEModelGuard::OnPIEStartedOrEnded);
		CancelPIEHandle      = FEditorDelegates::CancelPIE.AddStatic(&FTryllPIEModelGuard::OnPIECancelled);

		// Denied attempts don't auto-retry on their own; explicitly ask again now that
		// RequestPIEPermission will allow it through.
		UE_LOG(LogTryll, Log,
			TEXT("FTryllPIEModelGuard: models ready — resuming Play (bVerifiedReady=true, RequestPlaySession)"));
		GEditor->RequestPlaySession(FRequestPlaySessionParams());
	}
	else
	{
		ReleaseHeldConnection();
		if (!Error.IsEmpty())
		{
			FTryllEditorPrompts::Open(EAppMsgType::Ok, FText::Format(
				LOCTEXT("CannotEnterPlay", "Tryll — cannot enter Play:\n\n{0}"),
				FText::FromString(Error)));
		}
		// Cancelled by the user (Error empty): stay in edit mode quietly.
	}
}

void FTryllPIEModelGuard::OnPIEStartedOrEnded(bool /*bIsSimulating*/)
{
	ReleaseHeldConnection();
}

void FTryllPIEModelGuard::OnPIECancelled()
{
	ReleaseHeldConnection();
}

void FTryllPIEModelGuard::HandleError(const FString& ErrorMessage)
{
	// Cached for whichever paired bool-only completion handler fires next — see LastErrorMessage's
	// declaration for why this is always fresh when read.
	LastErrorMessage = ErrorMessage;
}

void FTryllPIEModelGuard::Cleanup()
{
	bCheckInProgress = false;
	DisarmWatchdog();
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}
	ModelGuard.Reset();
	// Failure paths reach Cleanup without an explicit dismiss — expire as fail so the
	// toast doesn't linger as a perpetual throbber under the modal error dialog.
	DismissProgressNotification(SNotificationItem::CS_Fail);
}

void FTryllPIEModelGuard::DisarmWatchdog()
{
	if (WatchdogTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(WatchdogTickerHandle);
		WatchdogTickerHandle.Reset();
	}
}

void FTryllPIEModelGuard::ReleaseHeldConnection()
{
	if (bAcquiredConnection)
	{
		UE_LOG(LogTryll, Log, TEXT("FTryllPIEModelGuard: releasing held connection"));
	}

	if (PostPIEStartedHandle.IsValid())
	{
		FEditorDelegates::PostPIEStarted.Remove(PostPIEStartedHandle);
		PostPIEStartedHandle.Reset();
	}
	if (EndPIEHandle.IsValid())
	{
		FEditorDelegates::EndPIE.Remove(EndPIEHandle);
		EndPIEHandle.Reset();
	}
	if (CancelPIEHandle.IsValid())
	{
		FEditorDelegates::CancelPIE.Remove(CancelPIEHandle);
		CancelPIEHandle.Reset();
	}

	// Mirrors the panels' fix: release our own acquisition rather than forcing a disconnect
	// that would tear down the connection for other consumers (e.g. the chat panel).
	if (EditorSubsystem && bAcquiredConnection)
	{
		EditorSubsystem->ReleaseConnection();
	}
	bAcquiredConnection = false;
}

#undef LOCTEXT_NAMESPACE
