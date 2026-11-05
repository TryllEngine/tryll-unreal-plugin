// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

struct FTryllDialogLabVariant;
struct FTryllGraphDescription;
class UTryllAgentComponent;
class UTryllDialogScriptAsset;

/**
 * What a variant runs on, as the pre-run checks see it: the Lab panel builds one from a scene
 * UTryllAgentComponent, headless mode from a workflow asset.
 */
struct FTryllLabRunAgent
{
	FString Name;
	const FTryllGraphDescription* Graph = nullptr;
	bool bMaintainDialogueHistory = true;

	static TOptional<FTryllLabRunAgent> FromComponent(const UTryllAgentComponent* Agent);
};

/**
 * The checks a run must pass before it starts, shared by the Lab panel and the headless
 * commandlet so both refuse the same runs with the same messages (Unity's
 * TryllDialogLabRunValidation).
 */
struct FTryllDialogLabRunValidation
{
	/** Every dialogue on its own; messages name the dialogue when there are several. */
	static bool TryValidateScripts(const TArray<const UTryllDialogScriptAsset*>& Scripts, FString& OutError);

	/**
	 * One variant against its agent: a graph to run, overrides that still apply, a resolvable
	 * model, and no seeded history on a stateless agent. An unset agent is refused with the
	 * panel's wording.
	 */
	static bool TryValidateVariant(const FTryllDialogLabVariant& Variant, const TOptional<FTryllLabRunAgent>& Agent,
		const TArray<const UTryllDialogScriptAsset*>& Scripts, FString& OutError);
};
