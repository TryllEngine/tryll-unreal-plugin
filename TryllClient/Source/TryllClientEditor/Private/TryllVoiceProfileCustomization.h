// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "IDetailCustomization.h"
#include "IPropertyTypeCustomization.h"
#include "Templates/SharedPointer.h"

class IPropertyHandle;
class UTryllVoiceProfile;
struct FTryllTtsCapabilities;
struct FTryllVoiceCapsCache;

/**
 * Details panel for UTryllVoiceProfile, driven by the voice model's catalog
 * tts_capabilities (FTryllModelCatalogFile, so it works with no server running):
 *   - a field the model cannot use is hidden, unless it already holds a value
 *     (then it stays visible and the issues row says to clear it);
 *   - Speaker Id shows the model's preset label or speaker range;
 *   - Default Delivery Mode is a dropdown of the defined modes;
 *   - an issues row lists what the server would reject (3023), plus the local
 *     checks of FTryllVoiceProfileData::Validate.
 * A model missing from the catalog shows every field.
 */
class FTryllVoiceProfileCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};

/**
 * One delivery mode row inside a voice profile: the header shows the mode name,
 * and the children follow the same capability rules as the profile fields.
 */
class FTryllDeliveryModeCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		class FDetailWidgetRow& HeaderRow,
		IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	virtual void CustomizeChildren(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		class IDetailChildrenBuilder& ChildBuilder,
		IPropertyTypeCustomizationUtils& CustomizationUtils) override;
};
