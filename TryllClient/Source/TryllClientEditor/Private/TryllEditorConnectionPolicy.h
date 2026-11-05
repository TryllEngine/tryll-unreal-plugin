// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

/**
 * Shared wait-versus-fail rule for editor consumers of UTryllEditorSubsystem.
 * Keep this a header-only predicate so automation can lock the contract without
 * constructing Slate panels.
 */
namespace TryllEditorConnectionPolicy
{
	/** Transient disconnect during startup is not terminal while retry budget remains. */
	[[nodiscard]] inline bool ShouldWaitForOwnerRetry(bool bStarting, bool bRetriesRemaining) noexcept
	{
		return bStarting && bRetriesRemaining;
	}

	/** A panel that already holds the shared connection must not bind or acquire again. */
	[[nodiscard]] inline bool ShouldBindAndAcquire(bool bAlreadyAcquired) noexcept
	{
		return !bAlreadyAcquired;
	}

	/** Retry a failed local list/refresh when the owner session is still ready. */
	[[nodiscard]] inline bool ShouldRetryLocalWork(bool bAlreadyAcquired, bool bSessionReady) noexcept
	{
		return bAlreadyAcquired && bSessionReady;
	}
} // namespace TryllEditorConnectionPolicy
