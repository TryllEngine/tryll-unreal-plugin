// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

class UTryllNodeParamsBase;

/** One declared graph exit on a node-params object, discovered via the `TryllExit` UPROPERTY metadata. */
struct FTryllNodeExit
{
	/** Wire exit name (e.g. "tool_called", "default"). Matches the FlatBuffers `<name>_exit` field. */
	FString ExitName;

	/** The FString UPROPERTY that holds this exit's target node name ("" = END). */
	FStrProperty* Prop = nullptr;
};

/**
 * Reflection over node-params exit fields.
 *
 * Exit fields are emitted by codegen (unreal_node.h.j2) with
 * `meta=(GetOptions="GetExitTargetOptions", TryllExit="<wire_name>")`. This helper
 * surfaces them generically so the graph editor never needs per-node-type code:
 * the set of output pins on a node is exactly its set of TryllExit-tagged properties.
 */
struct FTryllNodeExitReflection
{
	/** All exits declared on Params' class, in declaration order. Empty if Params is null. */
	static TArray<FTryllNodeExit> GetExits(const UTryllNodeParamsBase* Params);

	/** All exits declared on a params UClass, in declaration order. */
	static TArray<FTryllNodeExit> GetExitsForClass(const UClass* ParamClass);

	/** Read the current target node name for an exit ("" = END). */
	static FString ReadTarget(const UTryllNodeParamsBase* Params, const FTryllNodeExit& Exit);

	/** Write the target node name for an exit ("" = END). */
	static void WriteTarget(UTryllNodeParamsBase* Params, const FTryllNodeExit& Exit, const FString& Target);
};
