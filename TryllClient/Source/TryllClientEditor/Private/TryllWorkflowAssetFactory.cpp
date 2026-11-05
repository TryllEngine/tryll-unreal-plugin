// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllWorkflowAssetFactory.h"

#include "AssetTypeCategories.h"
#include "TryllWorkflowAsset.h"

uint32 UTryllWorkflowAssetFactory::MenuCategory = EAssetTypeCategories::Misc;

UTryllWorkflowAssetFactory::UTryllWorkflowAssetFactory()
{
	SupportedClass = UTryllWorkflowAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

void UTryllWorkflowAssetFactory::SetMenuCategory(uint32 InMenuCategory)
{
	MenuCategory = InMenuCategory;
}

UObject* UTryllWorkflowAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent,
	FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UTryllWorkflowAsset>(InParent, Class, Name, Flags | RF_Transactional);
}

uint32 UTryllWorkflowAssetFactory::GetMenuCategories() const
{
	return MenuCategory;
}

FText UTryllWorkflowAssetFactory::GetDisplayName() const
{
	return NSLOCTEXT("TryllWorkflowAssetFactory", "DisplayName", "Workflow Asset");
}
