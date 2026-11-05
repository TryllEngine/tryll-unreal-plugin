// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllVoiceProfileFactory.h"

#include "TryllVoiceProfile.h"

uint32 UTryllVoiceProfileFactory::MenuCategory = 0;

UTryllVoiceProfileFactory::UTryllVoiceProfileFactory()
{
	SupportedClass = UTryllVoiceProfile::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

void UTryllVoiceProfileFactory::SetMenuCategory(uint32 InMenuCategory)
{
	MenuCategory = InMenuCategory;
}

UObject* UTryllVoiceProfileFactory::FactoryCreateNew(UClass* Class, UObject* InParent,
	FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UTryllVoiceProfile>(InParent, Class, Name, Flags | RF_Transactional);
}

uint32 UTryllVoiceProfileFactory::GetMenuCategories() const
{
	return MenuCategory;
}

FText UTryllVoiceProfileFactory::GetDisplayName() const
{
	return NSLOCTEXT("TryllVoiceProfileFactory", "DisplayName", "Voice Profile");
}
