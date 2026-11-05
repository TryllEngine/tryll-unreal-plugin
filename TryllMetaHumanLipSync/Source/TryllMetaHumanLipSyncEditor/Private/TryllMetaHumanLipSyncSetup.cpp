// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllMetaHumanLipSyncSetup.h"

#include "Misc/PackageName.h"
#include "NNE.h"
#include "NNEModelData.h"
#include "TryllMetaHumanLipSyncPaths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

#define LOCTEXT_NAMESPACE "TryllMetaHumanLipSync"

namespace
{
bool HasCpuTarget(const UNNEModelData& ModelData)
{
	return ModelData.GetTargetRuntimes().Contains(TryllMetaHumanLipSync::CpuRuntimeName);
}

bool SaveModel(UNNEModelData& ModelData)
{
	const FString Runtimes[] = { TryllMetaHumanLipSync::CpuRuntimeName };
	ModelData.SetTargetRuntimes(Runtimes);
	const FString FileName = FPackageName::LongPackageNameToFilename(TryllMetaHumanLipSync::ModelPackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	return UPackage::SavePackage(ModelData.GetPackage(), &ModelData, *FileName, SaveArgs);
}
}

namespace TryllMetaHumanLipSyncSetup
{

bool IsSetUp()
{
	const UNNEModelData* ModelData = LoadObject<UNNEModelData>(nullptr, TryllMetaHumanLipSync::ModelObjectPath);
	return ModelData && HasCpuTarget(*ModelData);
}

bool RunSetup(FText& OutMessage)
{
	if (UNNEModelData* Existing = LoadObject<UNNEModelData>(nullptr, TryllMetaHumanLipSync::ModelObjectPath))
	{
		if (HasCpuTarget(*Existing))
		{
			OutMessage = LOCTEXT("AlreadySetUp", "MetaHuman lip sync is already set up.");
			return true;
		}
		if (!SaveModel(*Existing))
		{
			OutMessage = LOCTEXT("ResaveFailed", "The model asset could not be updated. Check the Output Log.");
			return false;
		}
		OutMessage = LOCTEXT("Repaired", "The model asset was updated.");
		return true;
	}

	UNNEModelData* Source = LoadObject<UNNEModelData>(nullptr, TryllMetaHumanLipSync::EngineModelObjectPath);
	if (!Source || Source->GetFileData().IsEmpty())
	{
		OutMessage = LOCTEXT("NoEngineModel", "Epic's speech-to-animation model was not found. Ensure the MetaHuman Core Tech plugin is enabled.");
		return false;
	}

	UPackage* Package = CreatePackage(TryllMetaHumanLipSync::ModelPackagePath);
	UNNEModelData* Copy = Cast<UNNEModelData>(StaticDuplicateObject(Source, Package, FName(FPackageName::GetLongPackageAssetName(TryllMetaHumanLipSync::ModelPackagePath))));
	if (!Copy)
	{
		OutMessage = LOCTEXT("DuplicateFailed", "The model asset could not be created. Check the Output Log.");
		return false;
	}
	Copy->SetFlags(RF_Public | RF_Standalone);
	if (!SaveModel(*Copy))
	{
		OutMessage = LOCTEXT("SaveFailed", "The model asset could not be saved. Check the Output Log.");
		return false;
	}

	OutMessage = LOCTEXT("SetUpDone", "MetaHuman lip sync is set up.");
	return true;
}

}

#undef LOCTEXT_NAMESPACE
