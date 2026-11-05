// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "Graph/AssetTypeActions_TryllWorkflow.h"
#include "Graph/TryllWorkflowAssetEditor.h"
#include "TryllWorkflowAsset.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions_TryllWorkflow"

FAssetTypeActions_TryllWorkflow::FAssetTypeActions_TryllWorkflow(EAssetTypeCategories::Type InCategory)
	: AssetCategory(InCategory)
{
}

FText FAssetTypeActions_TryllWorkflow::GetName() const
{
	return LOCTEXT("Name", "Tryll Workflow");
}

FColor FAssetTypeActions_TryllWorkflow::GetTypeColor() const
{
	return FColor(40, 112, 200);
}

UClass* FAssetTypeActions_TryllWorkflow::GetSupportedClass() const
{
	return UTryllWorkflowAsset::StaticClass();
}

uint32 FAssetTypeActions_TryllWorkflow::GetCategories()
{
	return AssetCategory;
}

void FAssetTypeActions_TryllWorkflow::OpenAssetEditor(const TArray<UObject*>& InObjects,
	TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid()
		? EToolkitMode::WorldCentric
		: EToolkitMode::Standalone;

	for (UObject* Object : InObjects)
	{
		if (UTryllWorkflowAsset* Asset = Cast<UTryllWorkflowAsset>(Object))
		{
			TSharedRef<FTryllWorkflowAssetEditor> Editor = MakeShared<FTryllWorkflowAssetEditor>();
			Editor->InitEditor(Mode, EditWithinLevelEditor, Asset);
		}
	}
}

#undef LOCTEXT_NAMESPACE
