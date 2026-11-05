// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "IPropertyTypeCustomization.h"

class FTryllStoragePathCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		FDetailWidgetRow& HeaderRow,
		IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	virtual void CustomizeChildren(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		IDetailChildrenBuilder& ChildBuilder,
		IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	/**
	 * When a picked path sits inside a `loc/<language>/` folder, offer to link the
	 * default file instead - the one the translation belongs to. Linking a
	 * translation directly pins the field to that language everywhere, because
	 * the server looks for `loc/<language>/` beside whatever is linked.
	 */
	static FString OfferDefaultPath(const FString& RelativePath);

	static bool IsInvalidPath(const FString& Value);
	static FString BuildFileDialogFilter(const FString& StorageKind);
	static bool BrowseForStorageFile(
		const FString& StorageKind,
		FString& OutRelativePath);
};
