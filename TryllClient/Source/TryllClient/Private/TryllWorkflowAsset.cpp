// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllWorkflowAsset.h"

#include "TryllClientModule.h"
#include "TryllModelManifest.h"
#include "TryllWorkflowModelRefs.h"

#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableRegistry.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#include "UObject/ObjectSaveContext.h"
#endif

void UTryllWorkflowAsset::SyncLocalizedTextFields()
{
	// A picked asset is the table id (string-table ids are their object paths).
	if (!TextTableAsset.IsNull())
	{
		TextTable = FName(*TextTableAsset.ToSoftObjectPath().ToString());
	}
	Graph.TextTable = TextTable;
	Graph.LocalizedAssetName = GetName();
}

void UTryllWorkflowAsset::PostLoad()
{
	Super::PostLoad();
	SyncLocalizedTextFields();
}

TArray<FString> UTryllWorkflowAsset::GetTextTableOptions() const
{
	TArray<FString> Options;
	// Registered tables: loaded assets and CSV tables registered from code.
	FStringTableRegistry::Get().EnumerateStringTables(
		[&Options](const FName& TableId, const FStringTableConstRef& /*Table*/)
		{
			Options.AddUnique(TableId.ToString());
			return true;
		});
	// String-table assets that are not loaded yet.
	if (FModuleManager::Get().IsModuleLoaded(TEXT("AssetRegistry")))
	{
		IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
		TArray<FAssetData> Assets;
		Registry.GetAssetsByClass(UStringTable::StaticClass()->GetClassPathName(), Assets);
		for (const FAssetData& Asset : Assets)
		{
			Options.AddUnique(Asset.GetSoftObjectPath().ToString());
		}
	}
	Options.Sort();
	return Options;
}

#if WITH_EDITOR

namespace
{
	EDataValidationResult ValidateProductionModels(
		const FTryllGraphDescription& Graph,
		const FString& AssetLabel,
		FDataValidationContext& Context)
	{
		const UTryllModelManifest* Manifest = UTryllModelManifest::GetActive();
		const TArray<FTryllWorkflowModelRef> Refs = FTryllWorkflowModelRefs::Collect(Graph);
		const TArray<FTryllWorkflowModelViolation> Violations =
			FTryllWorkflowModelRefs::FindNonProductionRefs(Graph, Manifest);

		EDataValidationResult Result = EDataValidationResult::Valid;
		for (const FTryllWorkflowModelViolation& V : Violations)
		{
			Context.AddError(FText::FromString(V.ToString(AssetLabel)));
			Result = EDataValidationResult::Invalid;
		}

		FString VadReason;
		if (FTryllWorkflowModelRefs::NeedsProductionVad(Refs, Manifest, VadReason))
		{
			Context.AddError(FText::FromString(VadReason));
			Result = EDataValidationResult::Invalid;
		}
		return Result;
	}
}

EDataValidationResult UTryllWorkflowAsset::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	const EDataValidationResult Local = ValidateProductionModels(Graph, GetPathName(), Context);
	return CombineDataValidationResults(Result, Local);
}

void UTryllWorkflowAsset::PreSave(FObjectPreSaveContext ObjectSaveContext)
{
	SyncLocalizedTextFields();
	Super::PreSave(ObjectSaveContext);

	if (!ObjectSaveContext.IsCooking())
	{
		return;
	}

	// Cook / package: Error-level logs fail the cook so a broken player is never shipped silently.
	const UTryllModelManifest* Manifest = UTryllModelManifest::GetActive();
	const FString AssetLabel = GetPathName();
	for (const FTryllWorkflowModelViolation& V :
		FTryllWorkflowModelRefs::FindNonProductionRefs(Graph, Manifest))
	{
		UE_LOG(LogTryll, Error,
			TEXT("[Tryll] Cook aborted — workflow references a non-Production model: %s. ")
			TEXT("Mark it Production in the Model Manager, or remove the reference."),
			*V.ToString(AssetLabel));
	}

	FString VadReason;
	if (FTryllWorkflowModelRefs::NeedsProductionVad(
			FTryllWorkflowModelRefs::Collect(Graph), Manifest, VadReason))
	{
		UE_LOG(LogTryll, Error, TEXT("[Tryll] Cook aborted — %s"), *VadReason);
	}
}

void UTryllWorkflowAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	SyncLocalizedTextFields();
}

#endif // WITH_EDITOR
