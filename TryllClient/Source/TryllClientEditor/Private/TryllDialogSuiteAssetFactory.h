// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "TryllDialogSuiteAssetFactory.generated.h"

/** Creates dialog suite assets from the Content Browser's Tryll category. */
UCLASS()
class UTryllDialogSuiteAssetFactory : public UFactory
{
	GENERATED_BODY()

public:
	UTryllDialogSuiteAssetFactory();

	static void SetMenuCategory(uint32 InMenuCategory);

	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
		EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual uint32 GetMenuCategories() const override;
	virtual FText GetDisplayName() const override;

private:
	static uint32 MenuCategory;
};
