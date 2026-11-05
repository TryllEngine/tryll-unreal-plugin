// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllGraphDescription.h"

/**
 * Static helper that validates a FTryllGraphDescription for common authoring errors.
 *
 * Usage
 * -----
 * Call Validate() from:
 *  - A "Validate Workflow" toolbar button in a custom asset editor.
 *  - Blueprint / C++ tooling scripts.
 *  - The future visual-graph-editor canvas overlay.
 *
 * Checks performed
 * ----------------
 * 1. StartNode is non-empty and present in Nodes.
 * 2. No duplicate node names.
 * 3. Each exit field (on the typed params) references a node that exists in the
 *    graph, or is empty (END).
 * 4. Orphan nodes (no incoming exit and not the start node) — reported as warnings.
 * 5. Node names and slot overrides (output_name) must match
 *    [A-Za-z_][A-Za-z0-9_]* and not be the reserved "user_message"; a slot name
 *    produced by more than one node is a warning.
 * 6. `input`-style slot selectors (FTryllSlotInput): a reference to a slot with no
 *    producer reachable on any path is an error; a producer reachable on only some
 *    paths (branch-dependent) is a warning. (Mustache `{{slot.<name>}}` template refs
 *    are not scanned here — that scanner is editor-only; see the Unity/Unreal editor
 *    tooling in TryllClientEditor / the Unity Editor assembly.)
 * 7. Storage file-picker fields: absolute paths and ".." traversal are errors;
 *    kind mismatch and virtual names (no recognized extension) are warnings.
 *    Voice clips are not on the graph since protocol v27: they live on the
 *    agent's UTryllVoiceProfile (FTryllVoiceProfileData::Validate).
 */
struct TRYLLCLIENT_API FTryllWorkflowIssue
{
	enum class ESeverity : uint8 { Warning, Error };

	ESeverity Severity;
	FString   Message;
};

struct TRYLLCLIENT_API FTryllWorkflowValidator
{
	/**
	 * Validate @p Graph and return a (possibly empty) list of issues.
	 * Thread-safety: reads Graph fields only; safe to call from any thread.
	 */
	static TArray<FTryllWorkflowIssue> Validate(const FTryllGraphDescription& Graph);

	/**
	 * Convenience wrapper: validate and log results via UE_LOG (LogTryllClient).
	 * Returns true if no errors were found (warnings are tolerated).
	 */
	static bool ValidateAndLog(const FTryllGraphDescription& Graph, const FString& ContextName);

	/**
	 * Path-shape checks shared by node storage fields and voice-profile clips: a
	 * path that should be written relative to storage_root is a warning; an
	 * absolute path or ".." traversal is an error. Where prefixes each message
	 * (e.g. "Node 'gen'"). Returns false when an error was added. Empty is valid.
	 */
	static bool ValidateStoragePathShape(
		const FString& Where, const FString& FieldName, const FString& Value, TArray<FTryllWorkflowIssue>& OutIssues);
};
