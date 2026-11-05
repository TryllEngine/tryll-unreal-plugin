// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

/// Internal value passed to a pending-handler lambda when an ErrorResponse
/// arrives for its request_id, or when the connection is torn down by
/// FailAllPending.
///
/// When Code == 0 and Message is empty, no error has occurred and the lambda
/// is being invoked with a regular response frame.
///
/// Lifetime: stack-only.  Constructed by HandleIncoming before invoking the
/// lambda; never stored.
struct FTryllErrorContext
{
	uint32  Code    = 0;
	FString Message;

	/// True when no error has been reported — the lambda should process the
	/// message normally.
	[[nodiscard]] bool IsEmpty() const { return Code == 0 && Message.IsEmpty(); }
};
