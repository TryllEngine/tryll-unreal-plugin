// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllDialogScriptAssetFactory.h"

#include "TryllDialogScriptAsset.h"

uint32 UTryllDialogScriptAssetFactory::MenuCategory = 0;

UTryllDialogScriptAssetFactory::UTryllDialogScriptAssetFactory()
{
	SupportedClass = UTryllDialogScriptAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

void UTryllDialogScriptAssetFactory::SetMenuCategory(uint32 InMenuCategory)
{
	MenuCategory = InMenuCategory;
}

UObject* UTryllDialogScriptAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent,
	FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UTryllDialogScriptAsset>(InParent, Class, Name, Flags | RF_Transactional);
}

uint32 UTryllDialogScriptAssetFactory::GetMenuCategories() const
{
	return MenuCategory;
}

FText UTryllDialogScriptAssetFactory::GetDisplayName() const
{
	return NSLOCTEXT("TryllDialogScriptAssetFactory", "DisplayName", "Dialog Script");
}
