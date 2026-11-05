// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllWorkflowToolset.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/PackageName.h"
#include "TryllWorkflowAsset.h"
#include "TryllWorkflowGraphJson.h"
#include "TryllWorkflowValidator.h"
#include "TryllWorkflowVariablesValidator.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace
{
	/**
	 * Raised as a Blueprint exception rather than logged: that is what the toolset
	 * layer turns into a tool-call error the caller actually sees. A logged message
	 * is invisible to the caller, who would just get a default-constructed result
	 * back and read the refusal as a silent no-op.
	 */
	void Raise(const FString& Message)
	{
		UKismetSystemLibrary::RaiseScriptError(Message);
	}

	FTryllWorkflowIssueInfo MakeIssue(const FTryllWorkflowIssue& Issue)
	{
		FTryllWorkflowIssueInfo Info;
		Info.Severity =
			Issue.Severity == FTryllWorkflowIssue::ESeverity::Error ? TEXT("Error") : TEXT("Warning");
		Info.Message = Issue.Message;
		return Info;
	}

	FTryllWorkflowIssueInfo MakeError(const FString& Message)
	{
		FTryllWorkflowIssueInfo Info;
		Info.Severity = TEXT("Error");
		Info.Message  = Message;
		return Info;
	}

	bool SplitAssetPath(
		const FString& In, FString& OutPackagePath, FString& OutAssetName, FString& OutObjectPath)
	{
		FString Trimmed = In.TrimStartAndEnd();
		if (Trimmed.IsEmpty())
		{
			return false;
		}

		// Accept both "/Game/Foo/WF_Bar" and "/Game/Foo/WF_Bar.WF_Bar".
		FString Left;
		FString Right;
		OutPackagePath = Trimmed.Split(TEXT("."), &Left, &Right) ? Left : Trimmed;

		if (!FPackageName::IsValidLongPackageName(OutPackagePath))
		{
			return false;
		}

		OutAssetName  = FPackageName::GetShortName(OutPackagePath);
		OutObjectPath = FString::Printf(TEXT("%s.%s"), *OutPackagePath, *OutAssetName);
		return true;
	}

	/**
	 * Parses and validates against a throwaway outer so that neither a failed
	 * WriteWorkflow nor any ValidateWorkflow can leave a partially built asset
	 * behind. The caller re-parses against the real asset once this comes back clean.
	 */
	bool ParseAndCollectIssues(
		const FString& GraphJson, TArray<FTryllWorkflowIssueInfo>& OutIssues)
	{
		const TryllWorkflowGraphJson::FParseResult Parsed =
			TryllWorkflowGraphJson::Parse(GraphJson, GetTransientPackage());

		for (const FString& Error : Parsed.Errors)
		{
			OutIssues.Add(MakeError(Error));
		}

		// Structural problems make graph-level checks meaningless — a node that
		// failed to parse is simply absent, which reads as a dangling exit.
		if (!Parsed.IsValid())
		{
			return false;
		}

		for (const FTryllWorkflowIssue& Issue : FTryllWorkflowValidator::Validate(Parsed.Graph))
		{
			OutIssues.Add(MakeIssue(Issue));
		}
		for (const FTryllWorkflowIssue& Issue :
			 TryllWorkflowVariablesValidator::Validate(Parsed.Graph, Parsed.Variables))
		{
			OutIssues.Add(MakeIssue(Issue));
		}

		return true;
	}

	bool HasError(const TArray<FTryllWorkflowIssueInfo>& Issues)
	{
		return Issues.ContainsByPredicate([](const FTryllWorkflowIssueInfo& Issue)
		{
			return Issue.Severity == TEXT("Error");
		});
	}

	FString JoinIssues(const TArray<FTryllWorkflowIssueInfo>& Issues)
	{
		TArray<FString> Lines;
		for (const FTryllWorkflowIssueInfo& Issue : Issues)
		{
			Lines.Add(FString::Printf(TEXT("[%s] %s"), *Issue.Severity, *Issue.Message));
		}
		return FString::Join(Lines, TEXT("\n"));
	}
}

FString UTryllWorkflowToolset::ReadWorkflow(const FString& AssetPath)
{
	FString PackagePath;
	FString AssetName;
	FString ObjectPath;
	if (!SplitAssetPath(AssetPath, PackagePath, AssetName, ObjectPath))
	{
		Raise(FString::Printf(
			TEXT("'%s' is not a valid asset path. Expected something like /Game/Merchant/WF_Trader."),
			*AssetPath));
		return FString();
	}

	const UTryllWorkflowAsset* Asset = LoadObject<UTryllWorkflowAsset>(nullptr, *ObjectPath);
	if (Asset == nullptr)
	{
		Raise(FString::Printf(TEXT("No workflow asset at '%s'."), *ObjectPath));
		return FString();
	}

	return TryllWorkflowGraphJson::Serialize(Asset->Graph, Asset->Variables);
}

FTryllWorkflowWriteResult UTryllWorkflowToolset::WriteWorkflow(
	const FString& AssetPath, const FString& GraphJson)
{
	FTryllWorkflowWriteResult Result;

	FString PackagePath;
	FString AssetName;
	FString ObjectPath;
	if (!SplitAssetPath(AssetPath, PackagePath, AssetName, ObjectPath))
	{
		Raise(FString::Printf(
			TEXT("'%s' is not a valid asset path. Expected something like /Game/Merchant/WF_Trader."),
			*AssetPath));
		return Result;
	}

	TArray<FTryllWorkflowIssueInfo> Issues;
	ParseAndCollectIssues(GraphJson, Issues);
	if (HasError(Issues))
	{
		Raise(FString::Printf(
			TEXT("Refusing to write '%s' — the graph has errors and the asset was left unchanged:\n%s"),
			*ObjectPath, *JoinIssues(Issues)));
		return Result;
	}

	UTryllWorkflowAsset* Asset = LoadObject<UTryllWorkflowAsset>(nullptr, *ObjectPath);
	UPackage*            Package = nullptr;

	if (Asset != nullptr)
	{
		Package = Asset->GetPackage();
	}
	else
	{
		Package = CreatePackage(*PackagePath);
		if (Package == nullptr)
		{
			Raise(FString::Printf(TEXT("Could not create package '%s'."), *PackagePath));
			return Result;
		}

		Asset = NewObject<UTryllWorkflowAsset>(
			Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		if (Asset == nullptr)
		{
			Raise(FString::Printf(TEXT("Could not create workflow asset '%s'."), *ObjectPath));
			return Result;
		}

		FAssetRegistryModule::AssetCreated(Asset);
		Result.bCreated = true;
	}

	// Re-parsed against the asset because params must be outered to it:
	// UTryllNodeParamsBase::FindEnclosingGraph walks the Outer chain.
	const TryllWorkflowGraphJson::FParseResult Parsed =
		TryllWorkflowGraphJson::Parse(GraphJson, Asset);
	if (!Parsed.IsValid())
	{
		Raise(FString::Printf(
			TEXT("Refusing to write '%s' — the graph has errors and the asset was left unchanged:\n%s"),
			*ObjectPath, *FString::Join(Parsed.Errors, TEXT("\n"))));
		return Result;
	}

	Asset->Modify();
	Asset->Graph     = Parsed.Graph;
	Asset->Variables = Parsed.Variables;
	Asset->MarkPackageDirty();

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags     = SAVE_NoError;

	const FString Filename = FPackageName::LongPackageNameToFilename(
		PackagePath, FPackageName::GetAssetPackageExtension());

	if (!UPackage::SavePackage(Package, Asset, *Filename, SaveArgs))
	{
		Raise(FString::Printf(
			TEXT("Graph applied to '%s' but the package could not be saved to '%s'."),
			*ObjectPath, *Filename));
		return Result;
	}

	Result.AssetPath     = ObjectPath;
	Result.NodeCount     = Asset->Graph.Nodes.Num();
	Result.VariableCount = Asset->Variables.Num();
	Result.Warnings      = MoveTemp(Issues);
	return Result;
}

FTryllWorkflowValidationResult UTryllWorkflowToolset::ValidateWorkflow(const FString& GraphJson)
{
	FTryllWorkflowValidationResult Result;
	ParseAndCollectIssues(GraphJson, Result.Issues);
	Result.bValid = !HasError(Result.Issues);
	return Result;
}

FString UTryllWorkflowToolset::DescribeNodeTypes(const FString& NodeType)
{
	return TryllWorkflowGraphJson::DescribeNodeTypes(NodeType);
}
