// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

class UObject;
class UTryllDialogScriptAsset;

/**
 * What the Lab's Dialog Script field holds: a single UTryllDialogScriptAsset (a run of one
 * dialogue) or a UTryllDialogSuiteAsset (several, in order).
 */
struct FTryllDialogLabDialogues
{
	static bool IsAcceptedSource(const UObject* Source);

	/**
	 * The scripts a source runs, in order. A suite's empty slots are skipped; a suite with no
	 * scripts, or any other asset type, is an error.
	 */
	static bool TryResolve(const UObject* Source,
		TArray<const UTryllDialogScriptAsset*>& OutScripts, FString& OutError);

	/** Validation messages name the dialogue only when there is more than one. */
	static FString Prefix(int32 Index, const FString& Name, int32 Count);
};
