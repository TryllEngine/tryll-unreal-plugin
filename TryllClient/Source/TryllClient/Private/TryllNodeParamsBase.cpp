// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "Generated/Nodes/TryllNodeParamsBase.h"
#include "TryllGraphDescription.h"
#include "TryllWorkflowAsset.h"
#include "TryllAssetVersion.h"
#include "TryllMigrations.h"

void UTryllNodeParamsBase::Serialize(FArchive& Ar)
{
	Ar.UsingCustomVersion(FTryllAssetVersion::Guid);
	if (Ar.IsLoading() && Ar.IsPersistent() && !Ar.IsTransacting()
		&& GetClass()->HasAnyClassFlags(CLASS_Native) && GetArchetype() == GetClass()->GetDefaultObject())
	{
		ApplyOldDefaults(Ar.CustomVer(FTryllAssetVersion::Guid));
	}
	Super::Serialize(Ar);
}

void UTryllNodeParamsBase::PostLoad()
{
	Super::PostLoad();
	if (UpgradeFrom(GetLinkerCustomVersion(FTryllAssetVersion::Guid)))
		MarkPackageDirty();
}

void UTryllNodeParamsBase::ApplyOldDefaults(int32 SavedVersion)
{
	TryllApplyOldDefaults(*this, SavedVersion);
}

bool UTryllNodeParamsBase::UpgradeFrom(int32 SavedVersion)
{
	return TryllUpgradeParams(*this, SavedVersion);
}

// ---------------------------------------------------------------------------
// UTryllNodeParamsBase::FindEnclosingGraph
//
// Walks the UObject Outer chain from a UTryllNodeParamsBase instance to locate
// the FTryllGraphDescription that contains it.  The chain is:
//
//   Params  →  FTryllNodeDescription (not a UObject)
//           →  UTryllWorkflowAsset (Graph field)
//
// We check each Outer by class rather than by property reflection to keep this
// file free of heavy reflection dependencies.
// ---------------------------------------------------------------------------
const FTryllGraphDescription* UTryllNodeParamsBase::FindEnclosingGraph(const UObject* Obj)
{
	for (const UObject* Outer = Obj ? Obj->GetOuter() : nullptr;
		 Outer != nullptr;
		 Outer = Outer->GetOuter())
	{
		if (const UTryllWorkflowAsset* Asset = Cast<UTryllWorkflowAsset>(Outer))
			return &Asset->Graph;
	}
	return nullptr;
}

FString UTryllNodeParamsBase::GetOwnerNodeName() const
{
	if (const FTryllGraphDescription* Graph = FindEnclosingGraph(this))
	{
		for (const FTryllNodeDescription& Node : Graph->Nodes)
			if (Node.Params == this)
				return Node.Name;
	}
	return FString();
}

TArray<FString> UTryllNodeParamsBase::GetExitTargetOptions() const
{
	TArray<FString> Options;
	Options.Add(TEXT("")); // END — turn finishes after this node
	if (const FTryllGraphDescription* Graph = FindEnclosingGraph(this))
		Options.Append(FTryllGraphOptions::GetNodeNameOptions(*Graph));
	return Options;
}
