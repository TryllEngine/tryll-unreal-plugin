// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllDialogSuiteAssetFactory.h"

#include "TryllDialogSuiteAsset.h"

uint32 UTryllDialogSuiteAssetFactory::MenuCategory = 0;

UTryllDialogSuiteAssetFactory::UTryllDialogSuiteAssetFactory()
{
	SupportedClass = UTryllDialogSuiteAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

void UTryllDialogSuiteAssetFactory::SetMenuCategory(uint32 InMenuCategory)
{
	MenuCategory = InMenuCategory;
}

UObject* UTryllDialogSuiteAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent,
	FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UTryllDialogSuiteAsset>(InParent, Class, Name, Flags | RF_Transactional);
}

uint32 UTryllDialogSuiteAssetFactory::GetMenuCategories() const
{
	return MenuCategory;
}

FText UTryllDialogSuiteAssetFactory::GetDisplayName() const
{
	return NSLOCTEXT("TryllDialogSuiteAssetFactory", "DisplayName", "Dialog Suite");
}
