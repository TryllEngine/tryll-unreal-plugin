// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

/**
 * Node-params object inside the graph editor's Details panel:
 *   - orders the generated group categories (Input, Model, Prompt, Storage, Behavior, Output,
 *     Advanced) and starts Advanced collapsed; Hidden and Exits never show.
 *   - hides exit-tagged (`TryllExit`) properties, since exits are edited by dragging wires.
 * Node name and docs button live in FTryllWorkflowAssetEditor's header bar above the panel.
 *
 * Registered per-instance on the graph editor's details view only, so the
 * Workflow Asset Details panel keeps the exit dropdowns and rename via
 * FTryllNodeDescriptionCustomization's Name field.
 */
class FTryllNodeParamsGraphCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};

/**
 * For the Entry node selection: shows only the workflow-level DefaultModelName, hiding the
 * Nodes array and StartNode (those are edited on the canvas). Registered per-instance on the
 * graph editor's details view only.
 */
class FTryllWorkflowAssetGraphCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
