// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Generated/TryllErrorCode.h"
#include "TryllError.generated.h"

// ETryllErrorCode is generated into Generated/TryllErrorCode.h from ErrorCodes.h.

/** Error value returned by Tryll API calls and delivered to Blueprint delegates. */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllError
{
	GENERATED_BODY()

	/** Numeric code matching ETryllErrorCode / server ErrorCode range. */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll",
		meta = (ToolTip = "Numeric code matching ETryllErrorCode or a server ErrorCode range. 0 = Ok."))
	int32 Code = 0;

	/** Human-readable error description. Safe for display in UI and logs. */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll",
		meta = (ToolTip = "Human-readable error description from the server, or a client-originated message."))
	FString Message;

	FTryllError() = default;

	/** True when Code == 0 (no error). Use before reading Message in success-or-failure flows. */
	[[nodiscard]] bool IsOk() const { return Code == 0; }

	/** Construct from a typed ETryllErrorCode value. */
	explicit FTryllError(ETryllErrorCode InCode, FString InMessage)
		: Code(static_cast<int32>(InCode))
		, Message(MoveTemp(InMessage))
	{
	}

	/** Construct from a raw server-supplied numeric code (e.g. an unknown future value). */
	explicit FTryllError(uint32 InCode, FString InMessage)
		: Code(static_cast<int32>(InCode))
		, Message(MoveTemp(InMessage))
	{
	}
};
