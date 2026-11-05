// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "GameDelegates.h"
#include "Misc/CoreDelegates.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "TryllMetaHumanLipSyncPIEGuard.h"
#include "TryllMetaHumanLipSyncPaths.h"
#include "TryllMetaHumanLipSyncSettings.h"
#include "TryllMetaHumanLipSyncSettingsCustomization.h"

class FTryllMetaHumanLipSyncEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FCoreDelegates::OnPostEngineInit.AddLambda([]()
		{
			GetMutableDefault<UTryllMetaHumanLipSyncSettings>()->RefreshStatus();
		});
		FTryllMetaHumanLipSyncPIEGuard::Register();

		FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		PropertyEditor.RegisterCustomClassLayout(
			UTryllMetaHumanLipSyncSettings::StaticClass()->GetFName(),
			FOnGetDetailCustomizationInstance::CreateStatic(&FTryllMetaHumanLipSyncSettingsCustomization::MakeInstance));

		ModifyCookHandle = FGameDelegates::Get().GetModifyCookDelegate().AddLambda(
			[](TConstArrayView<const ITargetPlatform*>, TArray<FName>& PackagesToCook, TArray<FName>& /*PackagesToNeverCook*/)
		{
			for (const TCHAR* PackagePath : { TryllMetaHumanLipSync::ModelPackagePath, TryllMetaHumanLipSync::FaceAnimBlueprintPackagePath })
			{
				if (FPackageName::DoesPackageExist(PackagePath))
				{
					PackagesToCook.AddUnique(FName(PackagePath));
				}
			}
		});
	}

	virtual void ShutdownModule() override
	{
		FGameDelegates::Get().GetModifyCookDelegate().Remove(ModifyCookHandle);
		if (FPropertyEditorModule* PropertyEditor = FModuleManager::GetModulePtr<FPropertyEditorModule>(TEXT("PropertyEditor")))
		{
			PropertyEditor->UnregisterCustomClassLayout(UTryllMetaHumanLipSyncSettings::StaticClass()->GetFName());
		}
		FTryllMetaHumanLipSyncPIEGuard::Unregister();
	}

private:
	FDelegateHandle ModifyCookHandle;
};

IMPLEMENT_MODULE(FTryllMetaHumanLipSyncEditorModule, TryllMetaHumanLipSyncEditor)
