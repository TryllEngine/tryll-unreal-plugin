// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllDiagnosticsDemand.h"

// ── FTryllDiagnosticsDemand::FLease ─────────────────────────────────────────

FTryllDiagnosticsDemand::FLease::FLease(TWeakPtr<FTryllDiagnosticsDemand> InOwner)
	: Owner(MoveTemp(InOwner))
{
}

FTryllDiagnosticsDemand::FLease::~FLease()
{
	Release();
}

void FTryllDiagnosticsDemand::FLease::Release()
{
	if (bReleased)
	{
		return;
	}
	bReleased = true;
	if (TSharedPtr<FTryllDiagnosticsDemand> Pin = Owner.Pin())
	{
		Pin->OnLeaseReleased();
	}
}

// ── FTryllDiagnosticsDemand ───────────────────────────────────────────────────

TSharedRef<FTryllDiagnosticsDemand::FLease> FTryllDiagnosticsDemand::Acquire()
{
	FScopeLock Lock(&Gate);
	++RefCount;
	return MakeShared<FLease>(AsShared());
}

bool FTryllDiagnosticsDemand::IsActive() const
{
	FScopeLock Lock(&Gate);
	return RefCount > 0;
}

void FTryllDiagnosticsDemand::OnLeaseReleased()
{
	FScopeLock Lock(&Gate);
	if (RefCount > 0)
	{
		--RefCount;
	}
}
