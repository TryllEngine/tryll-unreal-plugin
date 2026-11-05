// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllVariableDecl.h"
#include "TryllError.h"

class UTryllAgentComponent;
class FTryllAgentVariables;

/**
 * Pure helpers for Material-Instance style variable overrides on
 * UTryllAgentComponent. Used by CreateAgent merge, the Details customization,
 * and automation specs — no Slate / property-handle dependency.
 */
namespace TryllVariableOverrideUtils
{
	/** Declarations that drive override UI: WorkflowAsset->Variables, or empty when no asset. */
	TRYLLCLIENT_API TArray<FTryllVariableDecl> ResolveDeclared(const UTryllAgentComponent* Component);

	/** Index of the first override whose Name equals Name, or INDEX_NONE. */
	TRYLLCLIENT_API int32 FindFirstOverrideIndex(const TArray<FTryllVariableOverride>& Overrides, const FString& Name);

	/**
	 * Ensure a sparse override entry exists for Decl. When newly created or its
	 * nested type disagrees with the declaration, seed name/type/value from Decl.
	 * Does not change bEnabled.
	 */
	TRYLLCLIENT_API FTryllVariableOverride* EnsureEntry(
		TArray<FTryllVariableOverride>& Overrides, const FTryllVariableDecl& Decl);

	/** Overwrite an override's nested value from the workflow declaration default. */
	TRYLLCLIENT_API void SeedFromDeclaration(FTryllVariableOverride& Override, const FTryllVariableDecl& Decl);

	/**
	 * Indices of overrides that should not appear as declaration-driven rows:
	 * empty names, names absent from the declaration set, or duplicate names
	 * after the first occurrence. Order is ascending index.
	 */
	TRYLLCLIENT_API TArray<int32> CollectLegacyIndices(
		const TArray<FTryllVariableOverride>& Overrides,
		const TArray<FTryllVariableDecl>& Declared);

	/** Remove legacy/stale entries (highest index first). Returns count removed. */
	TRYLLCLIENT_API int32 RemoveLegacyEntries(
		TArray<FTryllVariableOverride>& Overrides,
		const TArray<FTryllVariableDecl>& Declared);

	/**
	 * Merge enabled matching overrides onto declarations (CreateAgent path).
	 * Type-mismatched and stale enabled overrides are skipped with LogTryll warnings.
	 */
	TRYLLCLIENT_API TArray<FTryllVariableDecl> MergeVariableOverrides(
		const TArray<FTryllVariableDecl>& Declared,
		const TArray<FTryllVariableOverride>& Overrides);

	/**
	 * Push the effective Inspector value onto the live agent mirror.
	 * When bEnabled is true, assign OverrideValue; when false, assign the
	 * workflow declaration default (not Reset(), which restores CreateAgent-time
	 * merged initials that may themselves have been overrides).
	 */
	TRYLLCLIENT_API FTryllError ApplyEffectiveValue(
		FTryllAgentVariables& Vars,
		const FTryllVariableDecl& Declared,
		const FTryllVariableDecl* OverrideValue,
		bool bEnabled);

	TRYLLCLIENT_API FTryllError AssignFromDecl(
		FTryllAgentVariables& Vars,
		const FString& Name,
		ETryllVariableType Type,
		const FTryllVariableDecl& Source);

	TRYLLCLIENT_API FTryllVariableDecl CloneDecl(const FTryllVariableDecl& Source);
}
