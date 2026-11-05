// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllDialogLabExperiment.h"

/**
 * Round-trips a variant through the one-line JSON form the editor config stores (spec 6.2).
 *
 * Extracted from the panel purely so the round-trip is testable: a variant that fails to restore
 * loses authored work silently, and that is not something to find out by hand after an editor
 * restart.
 */
struct TRYLLCLIENTEDITOR_API FTryllDialogLabVariantCodec
{
	static FString Encode(const FTryllDialogLabVariant& Variant);

	/** False when the line is not decodable JSON; OutVariant is then left untouched. */
	static bool Decode(const FString& Line, FTryllDialogLabVariant& OutVariant);
};
