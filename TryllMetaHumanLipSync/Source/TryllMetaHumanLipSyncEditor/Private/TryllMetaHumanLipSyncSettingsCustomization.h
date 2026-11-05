// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class IDetailLayoutBuilder;

/**
 * Adds the Set Up button to Project Settings > Plugins > Tryll MetaHuman Lip Sync.
 *
 * A CallInEditor function cannot do this: the settings page edits the class default
 * object, and the details panel never calls CallInEditor functions on archetypes
 * (FObjectDetails::AddCallInEditorMethods drops objects flagged RF_ArchetypeObject).
 */
class FTryllMetaHumanLipSyncSettingsCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder) override;
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	TWeakPtr<IDetailLayoutBuilder> WeakDetailBuilder;
};
