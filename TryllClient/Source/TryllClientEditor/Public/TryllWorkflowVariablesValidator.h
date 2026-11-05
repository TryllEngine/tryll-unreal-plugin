// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllGraphDescription.h"
#include "TryllVariableDecl.h"
#include "TryllWorkflowValidator.h"

/**
 * Editor-only validation pass for per-agent Variables references: every
 * {{var.<name>}} in a node template and every {"var": "<name>"} operand in a
 * Retrieve/ClassifyIntent Filter or Branch Condition JSON must name a variable
 * declared on the workflow. Also validates {"slot": "<name>"} operands (Branch's
 * Condition) against upstream slot reachability, and Branch's cross-field shape
 * rules (test/values/condition/input/variable coherence — mirrors the server's
 * ValidateBranchParamShape). Kept separate from FTryllWorkflowValidator
 * (TryllClient / Runtime module) because it depends on editor-only scanners
 * (TryllVarRefs, TryllSlotGraphAnalysis) and Variables, which the runtime
 * validator has no scope for — do NOT widen FTryllWorkflowValidator::Validate's
 * public signature to add them.
 */
namespace TryllWorkflowVariablesValidator
{
	// Exported (not just visible within TryllClientEditor) so TryllClientTests'
	// automation spec can link against it across the module DLL boundary.
	[[nodiscard]] TRYLLCLIENTEDITOR_API TArray<FTryllWorkflowIssue> Validate(
		const FTryllGraphDescription& Graph, const TArray<FTryllVariableDecl>& DeclaredVariables);
}
