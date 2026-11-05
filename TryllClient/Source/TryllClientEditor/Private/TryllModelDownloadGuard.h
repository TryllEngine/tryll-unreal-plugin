// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

class UTryllEditorSubsystem;
struct FTryllError;
struct FTryllModelInfo;

/**
 * feat-011 shared helper: compares the project's Production/Experimental model manifest
 * (all kinds — language/embedding/stt/tts/vad) against live ListModels status and downloads
 * anything Absent — blocking behind a cancelable FScopedSlowTask. Reuses the existing
 * ListModels/DownloadModel wire messages via the one shared UTryllEditorSubsystem connection
 * (PIE, the chat panel, and the model manager panel all route through it) rather than opening
 * a second connection of its own, which would race the shared subsystem's own in-flight
 * requests against this guard's.
 */
class FTryllModelDownloadGuard : public TSharedFromThis<FTryllModelDownloadGuard>
{
public:
	DECLARE_DELEGATE_TwoParams(FOnComplete, bool /*bSuccess*/, FString /*Error*/);

	// Out-of-line so TUniquePtr<FScopedSlowTask> (forward-declared below) can be destroyed
	// where FScopedSlowTask's full definition is visible (TryllModelDownloadGuard.cpp),
	// regardless of which translation unit destroys a TSharedPtr<FTryllModelDownloadGuard>.
	FTryllModelDownloadGuard();
	~FTryllModelDownloadGuard();

	/** Kicks off the ensure via the shared editor subsystem's model API. OnComplete fires exactly once. */
	void Start(UTryllEditorSubsystem* InSubsystem, FOnComplete InOnComplete);

	/** True between Start() and OnComplete firing — the owner should hold agent creation until then. */
	bool IsActive() const { return bActive; }

	/** Call once per Tick() while IsActive() to update/poll the slow-task dialog. */
	void Tick();

private:
	void Finish(bool bSuccess, const FString& Error);
	void HandleDownloadProgress(const FString& ModelName, uint64 BytesDownloaded, uint64 TotalBytes, float Percent);
	void HandleDownloadComplete(const FString& ModelName, bool bSuccess, const FString& ErrorMessage);
	void HandleConnectionChanged(bool bConnected);
	void Unsubscribe();

	UTryllEditorSubsystem* Subsystem = nullptr;
	FOnComplete OnComplete;
	bool bActive = false;

	TArray<FString> PendingDownloads;
	int32 TotalToDownload = 0;
	TMap<FString, FString> FailedDownloads; // model name -> server error (may be empty)
	TMap<FString, float> LastFractionByName; // 0..1, last DownloadProgress reported per model

	TUniquePtr<struct FScopedSlowTask> SlowTask;
};
