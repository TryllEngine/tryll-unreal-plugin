// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

class IDetailCategoryBuilder;
class IDetailChildrenBuilder;
class IDetailLayoutBuilder;
class IPropertyHandle;
class UTryllWorkflowAsset;

/** Preview rows and the Check texts window for Localized prompt fields. */
struct FTryllLocalizablePreview
{
	static void AddPreviews(IDetailLayoutBuilder& DetailBuilder, UObject* ParamsObject);
	/** Adds every child, then a preview under each Text ID. For a struct layout that replaces the default children. */
	static void AddPreviews(IDetailChildrenBuilder& ChildBuilder, TSharedRef<IPropertyHandle> ObjectOrStruct);
	/**
	 * Draws an instanced node-params object property (class picker header plus its
	 * fields) the same way the graph editor does: the inline text of a Localized
	 * field is hidden while it is ticked, and its Text ID gets a preview row.
	 */
	static void AddParamsObject(IDetailChildrenBuilder& ChildBuilder, TSharedRef<IPropertyHandle> ParamsHandle);
	static void OpenCheckTexts(UTryllWorkflowAsset* Asset);
	static void RegisterCustomizations();
	static void UnregisterCustomizations();
};
