// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

/**
 * Reflection over node-params structural fields.
 *
 * Structural fields are emitted by codegen (unreal_node.h.j2) with
 * `meta=(…, TryllStructural)` whenever the schema marks the field `(structural)`.
 * Future Dialog Lab / override UIs should use this helper so they never offer a
 * runtime change the server will reject with ParamNotMutable (3006).
 */
struct FTryllNodeStructuralReflection
{
	/** True when the property carries codegen `TryllStructural` metadata. */
	static bool IsStructural(const FProperty* Prop);

	/** All structural properties on a params UClass, in declaration order. */
	static TArray<FProperty*> GetStructuralProperties(const UClass* ParamClass);
};
