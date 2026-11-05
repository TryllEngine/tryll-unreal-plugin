// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

/** Content Browser integration for UTryllWorkflowAsset: double-click opens the visual graph editor. */
class FAssetTypeActions_TryllWorkflow : public FAssetTypeActions_Base
{
public:
	explicit FAssetTypeActions_TryllWorkflow(EAssetTypeCategories::Type InCategory);

	// IAssetTypeActions
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects,
		TSharedPtr<IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;

private:
	EAssetTypeCategories::Type AssetCategory;
};
