// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllDialogLabRunValidation.h"

#include "TryllAgentComponent.h"
#include "TryllDialogLabDialogues.h"
#include "TryllDialogLabExperiment.h"
#include "TryllDialogLabRunData.h"
#include "TryllDialogScriptAsset.h"
#include "TryllDialogScriptValidation.h"
#include "TryllGraphDescription.h"
#include "TryllNodeFieldReflection.h"
#include "TryllWorkflowAsset.h"

TOptional<FTryllLabRunAgent> FTryllLabRunAgent::FromComponent(const UTryllAgentComponent* Agent)
{
	if (Agent == nullptr)
	{
		return TOptional<FTryllLabRunAgent>();
	}
	FTryllLabRunAgent Out;
	Out.Name  = Agent->GetName();
	Out.Graph = Agent->WorkflowAsset != nullptr ? &Agent->WorkflowAsset->Graph : nullptr;
	Out.bMaintainDialogueHistory = Agent->bMaintainDialogueHistory;
	return Out;
}

bool FTryllDialogLabRunValidation::TryValidateScripts(
	const TArray<const UTryllDialogScriptAsset*>& Scripts, FString& OutError)
{
	for (int32 D = 0; D < Scripts.Num(); ++D)
	{
		FString ScriptError;
		if (!FTryllDialogScriptValidation::TryValidate(Scripts[D], ScriptError))
		{
			OutError = FTryllDialogLabDialogues::Prefix(D, Scripts[D]->GetName(), Scripts.Num()) + ScriptError;
			return false;
		}
	}
	OutError.Reset();
	return true;
}

bool FTryllDialogLabRunValidation::TryValidateVariant(const FTryllDialogLabVariant& Variant,
	const TOptional<FTryllLabRunAgent>& Agent, const TArray<const UTryllDialogScriptAsset*>& Scripts, FString& OutError)
{
	OutError.Reset();
	if (!Agent.IsSet())
	{
		OutError = FString::Printf(
			TEXT("Variant '%s' has no agent: set one on the variant, or assign a default ")
			TEXT("Agent Component."), *Variant.Label);
		return false;
	}

	const FTryllGraphDescription* Graph = Agent->Graph;
	if (Graph == nullptr || Graph->Nodes.Num() == 0)
	{
		OutError = FString::Printf(TEXT("Variant '%s': agent '%s' has no workflow graph."),
			*Variant.Label, *Agent->Name);
		return false;
	}

	const TArray<FString> Problems =
		FTryllNodeFieldReflection::CollectOverrideProblems(*Graph, Variant.Overrides);
	if (Problems.Num() > 0)
	{
		OutError = FString::Printf(TEXT("Variant '%s': %s"),
			*Variant.Label, *FString::Join(Problems, TEXT("; ")));
		return false;
	}

	// A model must be resolvable from somewhere, or CreateAgent fails server-side with
	// "no model_name param and no agent default_model_name" — a clearer failure before the
	// run than after it. Detected by reflection (6.29), so a new model-using node type
	// needs no change here.
	if (Variant.ModelOverride.IsEmpty() && Graph->DefaultModelName.Name.IsEmpty())
	{
		// Only nodes that *use* a model count. PinnedModelOf returns empty both for a
		// model-using node that pins nothing and for a Branch or Transform node that has no
		// ModelName at all — treating the second as "needs a model" rejects graphs Unity runs.
		const bool bAnyNodeNeedsModel = Graph->Nodes.ContainsByPredicate(
			[](const FTryllNodeDescription& Node)
			{
				return FTryllDialogLabRunSnapshot::DeclaresModel(Node.Params)
					&& FTryllDialogLabRunSnapshot::PinnedModelOf(Node.Params).IsEmpty();
			});
		if (bAnyNodeNeedsModel)
		{
			OutError = FString::Printf(
				TEXT("Variant '%s': no model. The workflow sets no DefaultModelName and at ")
				TEXT("least one node does not pin its own — pick a Model for this variant."),
				*Variant.Label);
			return false;
		}
	}

	for (int32 D = 0; D < Scripts.Num(); ++D)
	{
		FString AgentError;
		if (!FTryllDialogScriptValidation::TryValidateForAgent(
				Scripts[D]->SeedTurnCount, Agent->bMaintainDialogueHistory, Agent->Name, AgentError))
		{
			OutError = FTryllDialogLabDialogues::Prefix(D, Scripts[D]->GetName(), Scripts.Num())
				+ FString::Printf(TEXT("Variant '%s': %s"), *Variant.Label, *AgentError);
			return false;
		}
	}
	return true;
}
