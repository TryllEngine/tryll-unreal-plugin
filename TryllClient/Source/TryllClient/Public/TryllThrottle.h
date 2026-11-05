// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllThrottle.generated.h"

/**
 * Whether a human ever waits on an agent's turns. Purely a scheduling hint
 * for the inference budget controller — it never changes node semantics.
 * Set Background for world-logic/evaluator agents so their work is paced or
 * deferred first under GPU pressure.
 */
UENUM(BlueprintType)
enum class ETryllWorkload : uint8
{
	Interactive UMETA(DisplayName = "Interactive"),
	Background  UMETA(DisplayName = "Background"),
};
