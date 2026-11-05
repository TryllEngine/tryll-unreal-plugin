// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

/**
 * Ref-counted diagnostics capture lease shared by editor + PIE on one connection.
 * Active leases OR into agent create enable_diagnostics; they never mutate assets.
 */
class TRYLLCLIENT_API FTryllDiagnosticsDemand final : public TSharedFromThis<FTryllDiagnosticsDemand>
{
public:
	class FLease final : public TSharedFromThis<FLease>
	{
	public:
		explicit FLease(TWeakPtr<FTryllDiagnosticsDemand> InOwner);
		~FLease();

		FLease(const FLease&)            = delete;
		FLease& operator=(const FLease&) = delete;

	private:
		void Release();

		TWeakPtr<FTryllDiagnosticsDemand> Owner;
		bool                              bReleased = false;

		friend class FTryllDiagnosticsDemand;
	};

	TSharedRef<FLease> Acquire();
	bool               IsActive() const;

private:
	void OnLeaseReleased();

	mutable FCriticalSection Gate;
	int32                    RefCount = 0;
};
