// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

class UEdGraph;
class UTryllWorkflowAsset;
struct FTryllGraphDescription;

/**
 * Bidirectional converter between FTryllGraphDescription (authoritative, owned by the asset and
 * shipped on the wire) and the transient UEdGraph the canvas renders.
 *
 *   Import: description  -> EdGraph nodes/pins/wires + node positions
 *   Export: EdGraph wires -> description Nodes[] + StartNode (DefaultModelName preserved)
 *
 * This is the only genuinely novel logic in the editor and is covered by unit tests in
 * TryllClientTests; everything else is UE machinery. It is intentionally free of Slate/UI
 * dependencies so it can run headless.
 */
struct FTryllGraphSync
{
	/**
	 * Rebuild Graph's nodes from Asset->Graph. Reuses the asset-owned params objects (no copies),
	 * so Details-panel edits flow straight back to the description. Nodes that carry a saved
	 * EditorPos are placed exactly; the rest are auto-laid-out.
	 */
	static void ImportDescriptionToGraph(UTryllWorkflowAsset* Asset, UEdGraph* Graph);

	/**
	 * Rewrite OutDesc.Nodes and OutDesc.StartNode from the current pin connections in Graph.
	 * Exit targets and StartNode are derived from wires (not the stored strings), which is what
	 * makes node renames safe. OutDesc.DefaultModelName is left untouched.
	 */
	static void ExportGraphToDescription(const UEdGraph* Graph, FTryllGraphDescription& OutDesc);

	/** BFS auto-layout from the Entry/Start node. If bOnlyUnpositioned, only moves flagged nodes. */
	static void AutoLayout(UEdGraph* Graph, bool bOnlyUnpositioned);
};
