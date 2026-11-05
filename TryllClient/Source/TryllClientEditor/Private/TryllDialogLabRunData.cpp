// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllDialogLabRunData.h"

#include "TryllDialogLabExperiment.h"
#include "TryllAgentComponent.h"
#include "TryllRuntimeSettings.h"
#include "TryllVariableOverrideUtils.h"
#include "TryllWorkflowAsset.h"
#include "UI/TryllEditorStyle.h"
#include "Misc/Guid.h"

namespace
{
	const FTryllGraphDescription* GraphOf(const UTryllAgentComponent* Agent)
	{
		if (Agent == nullptr)
		{
			return nullptr;
		}
		return Agent->WorkflowAsset != nullptr
			? &Agent->WorkflowAsset->Graph
			: nullptr;
	}

	TArray<FTryllVariableDecl> VariablesOf(const UTryllAgentComponent* Agent)
	{
		if (Agent == nullptr)
		{
			return {};
		}
		const TArray<FTryllVariableDecl> Declared = Agent->WorkflowAsset != nullptr
			? Agent->WorkflowAsset->Variables
			: TArray<FTryllVariableDecl>{};
		return TryllVariableOverrideUtils::MergeVariableOverrides(Declared, Agent->VariableOverrides);
	}

	FString PinnedModelOf(const UTryllNodeParamsBase* Params)
	{
		return FTryllDialogLabRunSnapshot::PinnedModelOf(Params);
	}
}

bool FTryllDialogLabRunSnapshot::DeclaresModel(const UTryllNodeParamsBase* Params)
{
	if (Params == nullptr)
	{
		return false;
	}
	const FProperty* Property = Params->GetClass()->FindPropertyByName(TEXT("ModelName"));
	const FStructProperty* AsStruct = CastField<FStructProperty>(Property);
	return (AsStruct != nullptr && AsStruct->Struct == FTryllModelName::StaticStruct())
		|| CastField<FStrProperty>(Property) != nullptr;
}

FString FTryllDialogLabRunSnapshot::PinnedModelOf(const UTryllNodeParamsBase* Params)
{
	if (Params == nullptr)
	{
		return FString();
	}
	const FProperty* Property = Params->GetClass()->FindPropertyByName(TEXT("ModelName"));
	const FStructProperty* AsStruct = CastField<FStructProperty>(Property);
	if (AsStruct != nullptr && AsStruct->Struct == FTryllModelName::StaticStruct())
	{
		const FTryllModelName* Value = AsStruct->ContainerPtrToValuePtr<FTryllModelName>(Params);
		return Value != nullptr ? Value->Name : FString();
	}
	if (const FStrProperty* AsString = CastField<FStrProperty>(Property))
	{
		return *AsString->ContainerPtrToValuePtr<FString>(Params);
	}
	return FString();
}

namespace
{
	void FillNodeProvenance(FTryllLabVariantSnapshot& Variant, const FTryllGraphDescription* Graph)
	{
		TSet<FString> Models;
		if (!Variant.DefaultModelName.IsEmpty())
		{
			Models.Add(Variant.DefaultModelName);
		}
		if (Graph == nullptr)
		{
			return;
		}
		for (const FTryllNodeDescription& Node : Graph->Nodes)
		{
			FTryllLabNodeSummary Summary;
			Summary.Name = Node.Name;
			Summary.Type = Node.Params != nullptr ? Node.Params->GetClass()->GetName() : FString();
			Variant.Nodes.Add(MoveTemp(Summary));

			const FString Pinned = PinnedModelOf(Node.Params);
			if (!Pinned.IsEmpty())
			{
				Models.Add(Pinned);
			}
		}
		Variant.ModelNames = Models.Array();
		Variant.ModelNames.Sort([](const FString& A, const FString& B)
		{
			return A.Compare(B, ESearchCase::CaseSensitive) < 0;
		});
	}
}

FTryllGraphDescription FTryllDialogLabRunSnapshot::CloneGraph(
	const FTryllGraphDescription& Graph,
	TArray<TStrongObjectPtr<UTryllNodeParamsBase>>& OutKeepAlive)
{
	FTryllGraphDescription Clone;
	Clone.StartNode           = Graph.StartNode;
	Clone.DefaultModelName    = Graph.DefaultModelName;
	Clone.TextTable           = Graph.TextTable;
	Clone.LocalizedAssetName  = Graph.LocalizedAssetName;

	for (const FTryllNodeDescription& Node : Graph.Nodes)
	{
		FTryllNodeDescription NodeClone;
		NodeClone.Name = Node.Name;
		if (Node.Params != nullptr)
		{
			UTryllNodeParamsBase* ParamsClone =
				DuplicateObject<UTryllNodeParamsBase>(Node.Params, GetTransientPackage());
			NodeClone.Params = ParamsClone;
			OutKeepAlive.Add(TStrongObjectPtr<UTryllNodeParamsBase>(ParamsClone));
		}
		Clone.Nodes.Add(MoveTemp(NodeClone));
	}
	return Clone;
}

FTryllDialogLabRunSnapshot FTryllDialogLabRunSnapshot::Capture(
	const UTryllRuntimeSettings*     Settings,
	const UTryllAgentComponent*      DefaultAgent,
	const TArray<const UTryllDialogScriptAsset*>& ScriptAssets,
	const FTryllDialogLabExperiment& Experiment,
	TFunctionRef<UTryllAgentComponent*(const FTryllDialogLabVariant&)> EffectiveAgent)
{
	FTryllDialogLabRunSnapshot Snapshot;
	Snapshot.RunId             = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	Snapshot.CapturedAtUtc     = FDateTime::UtcNow();
	for (const UTryllDialogScriptAsset* ScriptAsset : ScriptAssets)
	{
		if (ScriptAsset != nullptr)
		{
			Snapshot.Dialogues.Add(FTryllLabScriptSnapshot::FromAsset(ScriptAsset));
		}
	}
	Snapshot.Repeats           = FMath::Max(1, Experiment.Repeats);
	Snapshot.BaseSeed          = FMath::Max(1, Experiment.BaseSeed);
	Snapshot.DefaultAgentName  = DefaultAgent != nullptr ? DefaultAgent->GetName() : FString();
	Snapshot.SettingsAssetName = Settings != nullptr ? Settings->GetName() : FString();

	for (int32 Index = 0; Index < Experiment.Variants.Num(); ++Index)
	{
		const FTryllDialogLabVariant& Authored = Experiment.Variants[Index];
		UTryllAgentComponent* Agent = EffectiveAgent(Authored);
		TOptional<FTryllVoiceProfileData> Voice;
		if (Agent != nullptr)
		{
			Voice = Agent->GetEffectiveVoiceProfile();
		}
		Snapshot.Variants.Add(BuildVariant(Index, Authored,
			Agent != nullptr ? Agent->GetName() : FString(),
			Agent != nullptr ? Agent->WorkflowAsset : nullptr,
			VariablesOf(Agent),
			Voice,
			// No agent is a blocked run (the panel refuses before Capture), so the fallback only
			// matters to defensive callers.
			Agent == nullptr || Agent->bMaintainDialogueHistory));
	}

	return Snapshot;
}

FTryllDialogLabRunSnapshot FTryllDialogLabRunSnapshot::CaptureFromSpec(
	const UTryllRuntimeSettings* Settings,
	const FTryllLabAgentSpec& Agent,
	const TArray<const UTryllDialogScriptAsset*>& ScriptAssets,
	const int32 Seeds,
	const int32 BaseSeed)
{
	FTryllDialogLabRunSnapshot Snapshot;
	Snapshot.RunId             = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	Snapshot.CapturedAtUtc     = FDateTime::UtcNow();
	for (const UTryllDialogScriptAsset* ScriptAsset : ScriptAssets)
	{
		if (ScriptAsset != nullptr)
		{
			Snapshot.Dialogues.Add(FTryllLabScriptSnapshot::FromAsset(ScriptAsset));
		}
	}
	Snapshot.Repeats           = FMath::Max(1, Seeds);
	Snapshot.BaseSeed          = FMath::Max(1, BaseSeed);
	Snapshot.DefaultAgentName  = Agent.Name;
	Snapshot.SettingsAssetName = Settings != nullptr ? Settings->GetName() : FString();

	FTryllDialogLabVariant Authored;
	Authored.Label         = Agent.Name;
	Authored.ModelOverride = Agent.Model;
	const TArray<FTryllVariableDecl> Declared = Agent.Workflow != nullptr
		? Agent.Workflow->Variables
		: TArray<FTryllVariableDecl>{};
	Snapshot.Variants.Add(BuildVariant(0, Authored, Agent.Name, Agent.Workflow,
		TryllVariableOverrideUtils::MergeVariableOverrides(Declared, Agent.VariableOverrides),
		TOptional<FTryllVoiceProfileData>(), Agent.bMaintainDialogueHistory));
	return Snapshot;
}

FTryllLabVariantSnapshot FTryllDialogLabRunSnapshot::BuildVariant(
	const int32 Index, const FTryllDialogLabVariant& Authored, const FString& AgentName,
	const UTryllWorkflowAsset* Workflow, const TArray<FTryllVariableDecl>& Variables,
	const TOptional<FTryllVoiceProfileData>& Voice, const bool bMaintainDialogueHistory)
{
	const FTryllGraphDescription* Graph = Workflow != nullptr ? &Workflow->Graph : nullptr;
	const FString ModelOverride = Authored.ModelOverride.TrimStartAndEnd();

	FTryllLabVariantSnapshot Variant;
	Variant.Index             = Index;
	Variant.Label             = Authored.Label.IsEmpty()
		? FString::Printf(TEXT("Variant %d"), Index + 1)
		: Authored.Label;
	// Positional, like Unity: a variant's colour follows its column, not what it was authored with.
	Variant.Swatch            = FTryllEditorStyle::VariantSwatch(Index);
	Variant.AgentName         = AgentName;
	Variant.bAgentIsOverride  = Authored.AgentOverride.IsValid();
	Variant.WorkflowAssetName = Workflow != nullptr ? Workflow->GetName() : FString();
	Variant.GraphStartNode    = Graph != nullptr ? Graph->StartNode : FString();
	Variant.DefaultModelName  = !ModelOverride.IsEmpty()
		? ModelOverride
		: (Graph != nullptr ? Graph->DefaultModelName.Name : FString());
	Variant.ModelOverride     = ModelOverride;
	Variant.Overrides         = Authored.Overrides;
	Variant.bMaintainDialogueHistory = bMaintainDialogueHistory;
	Variant.bUseLiveTools     = Authored.bUseLiveTools;

	if (Graph != nullptr)
	{
		Variant.Graph = CloneGraph(*Graph, Variant.ParamsKeepAlive);
	}
	Variant.Variables = Variables;
	Variant.Voice     = Voice;

	FillNodeProvenance(Variant, Graph);
	return Variant;
}
