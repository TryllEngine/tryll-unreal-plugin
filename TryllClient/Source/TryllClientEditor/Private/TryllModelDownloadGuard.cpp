// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllModelDownloadGuard.h"

#include "TryllEditorSubsystem.h"
#include "TryllError.h"
#include "TryllModelInfo.h"
#include "TryllModelManifest.h"

#include "Misc/ScopedSlowTask.h"

#define LOCTEXT_NAMESPACE "TryllModelDownloadGuard"

FTryllModelDownloadGuard::FTryllModelDownloadGuard() = default;

FTryllModelDownloadGuard::~FTryllModelDownloadGuard()
{
	Unsubscribe();
}

void FTryllModelDownloadGuard::Unsubscribe()
{
	if (Subsystem)
	{
		Subsystem->OnDownloadProgressNative.RemoveAll(this);
		Subsystem->OnDownloadCompleteNative.RemoveAll(this);
		Subsystem->OnConnectionChangedNative.RemoveAll(this);
	}
}

void FTryllModelDownloadGuard::Start(UTryllEditorSubsystem* InSubsystem, FOnComplete InOnComplete)
{
	Subsystem = InSubsystem;
	OnComplete = InOnComplete;
	bActive = true;
	PendingDownloads.Reset();
	FailedDownloads.Reset();
	LastFractionByName.Reset();
	TotalToDownload = 0;

	if (!Subsystem)
	{
		Finish(false, TEXT("No editor subsystem."));
		return;
	}

	Subsystem->OnDownloadProgressNative.AddRaw(this, &FTryllModelDownloadGuard::HandleDownloadProgress);
	Subsystem->OnDownloadCompleteNative.AddRaw(this, &FTryllModelDownloadGuard::HandleDownloadComplete);
	Subsystem->OnConnectionChangedNative.AddRaw(this, &FTryllModelDownloadGuard::HandleConnectionChanged);

	TWeakPtr<FTryllModelDownloadGuard> WeakSelf = SharedThis(this);
	Subsystem->RequestListModels([WeakSelf](TArray<FTryllModelInfo> Models, FTryllError Error)
	{
		TSharedPtr<FTryllModelDownloadGuard> Self = WeakSelf.Pin();
		if (!Self.IsValid() || !Self->bActive) return;

		if (!Error.IsOk())
		{
			Self->Finish(false, FString::Printf(TEXT("ListModels failed: %s"), *Error.Message));
			return;
		}

		TArray<FString> Downloadable, NonDownloadable;
		const UTryllModelManifest* Manifest = UTryllModelManifest::GetActive();
		for (const FTryllModelInfo& M : Models)
		{
			if (Manifest->Get(M.Name) == ETryllModelRegistration::None) continue;
			if (M.Status != ETryllModelStatus::Absent) continue;
			if (M.HuggingFaceRepo.IsEmpty()) NonDownloadable.Add(M.Name);
			else                             Downloadable.Add(M.Name);
		}

		if (NonDownloadable.Num() > 0)
		{
			FString List;
			for (const FString& N : NonDownloadable) List += FString::Printf(TEXT("  • %s\n"), *N);
			Self->Finish(false, FString::Printf(
				TEXT("The following registered models are missing and have no download source ")
				TEXT("(no HuggingFace repo configured):\n\n%s\nProvide them via a local path or update the catalog."),
				*List));
			return;
		}

		if (Downloadable.Num() == 0)
		{
			Self->Finish(true, FString());
			return;
		}

		Self->TotalToDownload  = Downloadable.Num();
		Self->PendingDownloads = Downloadable;
		Self->SlowTask = MakeUnique<FScopedSlowTask>(
			static_cast<float>(Self->TotalToDownload),
			FText::Format(LOCTEXT("Downloading", "Downloading {0} required model(s)…"),
				FText::AsNumber(Self->TotalToDownload)));
		// bAllowInPIE=true: this guard also runs during a PIE begin/end sequence, so the
		// dialog would otherwise never appear (MakeDialog defaults to suppressing it in PIE).
		Self->SlowTask->MakeDialog(/*bShowCancelButton=*/true, /*bAllowInPIE=*/true);

		for (const FString& N : Downloadable)
		{
			Self->Subsystem->RequestDownloadModel(N);
		}
	});
}

void FTryllModelDownloadGuard::HandleDownloadProgress(
	const FString& ModelName,
	uint64         /*BytesDownloaded*/,
	uint64         /*TotalBytes*/,
	float          Percent)
{
	if (!bActive || !PendingDownloads.Contains(ModelName)) return;

	float& Last = LastFractionByName.FindOrAdd(ModelName, 0.f);
	const float NewFraction = Percent / 100.f;
	const float Delta = NewFraction - Last;
	if (Delta > 0.f && SlowTask.IsValid()) SlowTask->EnterProgressFrame(Delta);
	Last = NewFraction;
}

void FTryllModelDownloadGuard::HandleDownloadComplete(
	const FString& ModelName,
	bool           bSuccess,
	const FString& ErrorMessage)
{
	if (!bActive || !PendingDownloads.Contains(ModelName)) return;

	PendingDownloads.Remove(ModelName);
	if (!bSuccess) FailedDownloads.Add(ModelName, ErrorMessage);
	// Advance by whatever fraction DownloadProgress hasn't already reported, so the total
	// advanced across this model's life is exactly 1.0 regardless of how granular the
	// progress events were (or if none arrived at all).
	if (SlowTask.IsValid())
	{
		const float Remaining = 1.f - LastFractionByName.FindRef(ModelName);
		if (Remaining > 0.f) SlowTask->EnterProgressFrame(Remaining);
	}
	LastFractionByName.Remove(ModelName);

	if (PendingDownloads.Num() == 0)
	{
		SlowTask.Reset();
		if (FailedDownloads.Num() > 0)
		{
			FString List;
			for (const TPair<FString, FString>& Pair : FailedDownloads)
			{
				if (Pair.Value.IsEmpty())
					List += FString::Printf(TEXT("  • %s\n"), *Pair.Key);
				else
					List += FString::Printf(TEXT("  • %s — %s\n"), *Pair.Key, *Pair.Value);
			}
			Finish(false, FString::Printf(TEXT("Failed to download:\n\n%s"), *List));
		}
		else
		{
			Finish(true, FString());
		}
	}
}

void FTryllModelDownloadGuard::HandleConnectionChanged(bool bConnected)
{
	if (!bActive || bConnected) return;
	Finish(false, TEXT("Connection dropped"));
}

void FTryllModelDownloadGuard::Tick()
{
	if (!bActive || !SlowTask.IsValid()) return;
	if (SlowTask->ShouldCancel())
	{
		SlowTask.Reset();
		Finish(false, FString()); // user cancel — no error message, stay in edit mode quietly
	}
}

void FTryllModelDownloadGuard::Finish(bool bSuccess, const FString& Error)
{
	bActive = false;
	SlowTask.Reset();
	Unsubscribe();
	FOnComplete Cb = OnComplete;
	OnComplete.Unbind();
	Cb.ExecuteIfBound(bSuccess, Error);
}

#undef LOCTEXT_NAMESPACE
