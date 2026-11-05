// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllTextResolve.h"

#include "TryllAgent.h"
#include "TryllClientModule.h"
#include "Generated/Nodes/TryllNodeParamsBase.h"

#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

namespace
{
	bool IsStructuralProperty(const FProperty* Prop)
	{
		if (Prop == nullptr)
		{
			return false;
		}
#if WITH_METADATA
		if (Prop->HasMetaData(TEXT("TryllStructural")))
		{
			return true;
		}
#endif
		// Packaged builds strip metadata. The only v1 structural container of a
		// localizable string is ClassifyIntentEntry.prompt inside Intents.
		return Prop->GetName() == TEXT("Intents");
	}

	// Not named MakeError: Unreal's Templates/ValueOrError.h declares a
	// better-matching MakeError template, and a forced unity build picks
	// that one. Its TValueOrError proxy cannot be assigned to FTryllError.
	FTryllError MakeLocalizedTextError(
		const FString& Asset, const FString& Node, const FString& Field,
		const FString& Table, const FString& Id, const TCHAR* Problem)
	{
		return FTryllError(
			ETryllErrorCode::InvalidParamValue,
			FString::Printf(
				TEXT("Localized text: asset '%s', node '%s', field '%s', table '%s', id '%s': %s"),
				*Asset, *Node, *Field, *Table, *Id, Problem));
	}

	FString JoinPath(const FString& Path, const FString& Name)
	{
		return Path.IsEmpty() ? Name : Path + TEXT(".") + Name;
	}

	/** The Localized flag and Text ID companions of a string property, or null. */
	bool FindCompanions(const UStruct* Struct, const FProperty* Str, FBoolProperty*& OutFlag, FStrProperty*& OutId)
	{
		const FString Name = Str->GetName();
		OutFlag = CastField<FBoolProperty>(Struct->FindPropertyByName(*(FString(TEXT("b")) + Name + TEXT("Localized"))));
		OutId = CastField<FStrProperty>(Struct->FindPropertyByName(*(Name + TEXT("TextId"))));
		return OutFlag != nullptr && OutId != nullptr;
	}

	/**
	 * Walks every string with Localized/TextId companions. In resolve mode
	 * (Provider walk) it writes the provider text into the string; in scan mode
	 * (bScanOnly) it only reports whether a ticked field exists.
	 */
	struct FWalk
	{
		UTryllTextProvider* Provider = nullptr;
		FString Asset;
		FString Table;
		bool bLiveOnly = false;
		bool bScanOnly = false;
		bool bAnyTicked = false;
		bool bChanged = false;
		FTryllError Error;

		bool VisitStruct(const UStruct* Struct, void* Container, bool bStructural, const FString& Path, const FString& Node)
		{
			if (Struct == nullptr || Container == nullptr)
			{
				return true;
			}

			for (TFieldIterator<FProperty> It(Struct); It; ++It)
			{
				FProperty* Prop = *It;
				const bool bNextStructural = bStructural || IsStructuralProperty(Prop);
				void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Container);

				if (FStrProperty* Str = CastField<FStrProperty>(Prop))
				{
					FBoolProperty* Flag = nullptr;
					FStrProperty* IdProp = nullptr;
					if (!FindCompanions(Struct, Prop, Flag, IdProp) || !Flag->GetPropertyValue_InContainer(Container))
					{
						continue;
					}
					if (bLiveOnly && bNextStructural)
					{
						continue;
					}
					bAnyTicked = true;
					if (bScanOnly)
					{
						continue;
					}

					const FString FieldPath = JoinPath(Path, Prop->GetName());
					const FString Id = IdProp->GetPropertyValue_InContainer(Container);
					if (Id.IsEmpty())
					{
						Error = MakeLocalizedTextError(Asset, Node, FieldPath, Table, Id, TEXT("Text ID is empty"));
						return false;
					}
					if (Provider == nullptr)
					{
						Error = MakeLocalizedTextError(Asset, Node, FieldPath, Table, Id, TEXT("no text provider is set"));
						return false;
					}

					FString Text;
					if (!Provider->GetText(FName(*Table), Id, Text) || Text.IsEmpty())
					{
						Error = MakeLocalizedTextError(Asset, Node, FieldPath, Table, Id, TEXT("not found"));
						return false;
					}

					FString& Current = *Str->GetPropertyValuePtr(ValuePtr);
					if (!Current.Equals(Text, ESearchCase::CaseSensitive))
					{
						Current = Text;
						bChanged = true;
					}
					// Optional strings are packed only when bOverride<Name> is set.
					if (FBoolProperty* Override = CastField<FBoolProperty>(
						Struct->FindPropertyByName(*(FString(TEXT("bOverride")) + Prop->GetName()))))
					{
						if (!Override->GetPropertyValue_InContainer(Container))
						{
							Override->SetPropertyValue_InContainer(Container, true);
							bChanged = true;
						}
					}
					continue;
				}

				if (FArrayProperty* Array = CastField<FArrayProperty>(Prop))
				{
					FStructProperty* Inner = CastField<FStructProperty>(Array->Inner);
					if (Inner == nullptr)
					{
						continue;
					}
					FScriptArrayHelper Helper(Array, ValuePtr);
					for (int32 Index = 0; Index < Helper.Num(); ++Index)
					{
						const FString Child = FString::Printf(TEXT("%s[%d]"), *JoinPath(Path, Prop->GetName()), Index);
						if (!VisitStruct(Inner->Struct, Helper.GetRawPtr(Index), bNextStructural, Child, Node))
						{
							return false;
						}
					}
					continue;
				}

				if (FStructProperty* Nested = CastField<FStructProperty>(Prop))
				{
					if (!VisitStruct(Nested->Struct, ValuePtr, bNextStructural, JoinPath(Path, Prop->GetName()), Node))
					{
						return false;
					}
				}
			}
			return true;
		}
	};
}

bool FTryllTextResolve::HasTickedFields(const UTryllNodeParamsBase* Params, bool bLiveOnly)
{
	if (Params == nullptr)
	{
		return false;
	}
	FWalk Walk;
	Walk.bScanOnly = true;
	Walk.bLiveOnly = bLiveOnly;
	Walk.VisitStruct(Params->GetClass(), const_cast<UTryllNodeParamsBase*>(Params), false, FString(), FString());
	return Walk.bAnyTicked;
}

bool FTryllTextResolve::HasTickedFields(const FTryllGraphDescription& Graph)
{
	for (const FTryllNodeDescription& Node : Graph.Nodes)
	{
		if (HasTickedFields(Node.Params.Get()))
		{
			return true;
		}
	}
	return false;
}

TArray<FTryllTextResolve::FTextIdRef> FTryllTextResolve::CollectTextIds(const FTryllGraphDescription& Graph)
{
	TArray<FTextIdRef> Found;

	struct FCollect
	{
		TArray<FTextIdRef>& Out;
		FString Node;

		void Visit(const UStruct* Struct, void* Container, const FString& Path)
		{
			if (Struct == nullptr || Container == nullptr)
			{
				return;
			}
			for (TFieldIterator<FProperty> It(Struct); It; ++It)
			{
				FProperty* Prop = *It;
				void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Container);
				if (CastField<FStrProperty>(Prop))
				{
					FBoolProperty* Flag = nullptr;
					FStrProperty* IdProp = nullptr;
					if (FindCompanions(Struct, Prop, Flag, IdProp) && Flag->GetPropertyValue_InContainer(Container))
					{
						FTextIdRef Ref;
						Ref.NodeName = Node;
						Ref.FieldPath = JoinPath(Path, Prop->GetName());
						Ref.Id = IdProp->GetPropertyValue_InContainer(Container);
						Out.Add(Ref);
					}
					continue;
				}
				if (FArrayProperty* Array = CastField<FArrayProperty>(Prop))
				{
					FStructProperty* Inner = CastField<FStructProperty>(Array->Inner);
					if (Inner == nullptr)
					{
						continue;
					}
					FScriptArrayHelper Helper(Array, ValuePtr);
					for (int32 Index = 0; Index < Helper.Num(); ++Index)
					{
						const FString Child = FString::Printf(TEXT("%s[%d]"), *JoinPath(Path, Prop->GetName()), Index);
						Visit(Inner->Struct, Helper.GetRawPtr(Index), Child);
					}
					continue;
				}
				if (FStructProperty* Nested = CastField<FStructProperty>(Prop))
				{
					Visit(Nested->Struct, ValuePtr, JoinPath(Path, Prop->GetName()));
				}
			}
		}
	};

	FCollect Collect{Found, FString()};
	for (const FTryllNodeDescription& Node : Graph.Nodes)
	{
		if (Node.Params == nullptr)
		{
			continue;
		}
		Collect.Node = Node.Name;
		Collect.Visit(Node.Params->GetClass(), Node.Params.Get(), FString());
	}
	return Found;
}

bool FTryllTextResolve::ResolveParams(
	UTryllNodeParamsBase* Params,
	UTryllTextProvider* Provider,
	FName Table,
	const FString& AssetName,
	const FString& NodeName,
	bool bLiveOnly,
	bool& bOutChanged,
	FTryllError& OutError)
{
	bOutChanged = false;
	if (Params == nullptr)
	{
		return true;
	}
	FWalk Walk;
	Walk.Provider = Provider;
	Walk.bLiveOnly = bLiveOnly;
	Walk.Asset = AssetName.IsEmpty() ? TEXT("workflow") : AssetName;
	Walk.Table = Table.IsNone() ? FString() : Table.ToString();
	if (!Walk.VisitStruct(Params->GetClass(), Params, false, FString(), NodeName))
	{
		OutError = Walk.Error;
		return false;
	}
	bOutChanged = Walk.bChanged;
	return true;
}

bool FTryllTextResolve::PrepareForCreate(
	const FTryllGraphDescription& Graph,
	UObject* Outer,
	UTryllTextProvider* Provider,
	FTryllGraphDescription& OutGraph,
	TArray<UTryllNodeParamsBase*>& OutOwned,
	FTryllError& OutError)
{
	OutOwned.Reset();
	OutGraph = Graph;
	if (!HasTickedFields(Graph))
	{
		return true;
	}

	OutGraph.Nodes.Reset();
	for (const FTryllNodeDescription& Node : Graph.Nodes)
	{
		FTryllNodeDescription Copy = Node;
		if (Node.Params != nullptr)
		{
			UTryllNodeParamsBase* Duplicate = DuplicateObject<UTryllNodeParamsBase>(Node.Params, Outer);
			Copy.Params = Duplicate;
			OutOwned.Add(Duplicate);
		}
		OutGraph.Nodes.Add(MoveTemp(Copy));
	}

	for (FTryllNodeDescription& Node : OutGraph.Nodes)
	{
		bool bChanged = false;
		if (!ResolveParams(Node.Params.Get(), Provider, Graph.TextTable, Graph.LocalizedAssetName, Node.Name,
				/*bLiveOnly=*/false, bChanged, OutError))
		{
			return false;
		}
	}
	return true;
}

// ── FTryllLocalizedAgentSet ───────────────────────────────────────────────────

FTryllLocalizedAgentSet::FTryllLocalizedAgentSet()
	: AliveToken(MakeShared<int32>(0))
{
}

void FTryllLocalizedAgentSet::Track(const TSharedPtr<FTryllAgent>& Agent, FName Table, const FString& AssetName)
{
	if (!Agent.IsValid() || Agent->GetAgentId() == 0)
	{
		return;
	}
	FBinding Binding;
	Binding.Agent = Agent;
	Binding.Table = Table;
	Binding.AssetName = AssetName.IsEmpty() ? TEXT("workflow") : AssetName;
	ById.Add(Agent->GetAgentId(), MoveTemp(Binding));
}

void FTryllLocalizedAgentSet::Forget(uint64 AgentId)
{
	ById.Remove(AgentId);
}

void FTryllLocalizedAgentSet::ForgetAll()
{
	ById.Reset();
}

bool FTryllLocalizedAgentSet::IsTracked(uint64 AgentId) const
{
	return ById.Contains(AgentId);
}

bool FTryllLocalizedAgentSet::HasPending(uint64 AgentId) const
{
	const FBinding* Binding = ById.Find(AgentId);
	return Binding != nullptr && Binding->bDirty;
}

void FTryllLocalizedAgentSet::MarkDirty(uint64 AgentId)
{
	if (FBinding* Binding = ById.Find(AgentId))
	{
		Binding->bDirty = true;
	}
}

int32 FTryllLocalizedAgentSet::Refresh(UTryllTextProvider* Provider, const FSend& Send, const FFail& Fail, const FApplied& Applied)
{
	if (Provider == nullptr || !Send)
	{
		return 0;
	}
	TArray<uint64> Ids;
	ById.GetKeys(Ids);
	int32 Sent = 0;
	for (uint64 Id : Ids)
	{
		Sent += RefreshAgent(Id, Provider, Send, Fail, Applied);
	}
	return Sent;
}

int32 FTryllLocalizedAgentSet::NoteTurnEnded(uint64 AgentId, UTryllTextProvider* Provider, const FSend& Send, const FFail& Fail, const FApplied& Applied)
{
	const FBinding* Binding = ById.Find(AgentId);
	if (Binding == nullptr || !Binding->bDirty)
	{
		return 0;
	}
	return RefreshAgent(AgentId, Provider, Send, Fail, Applied);
}

int32 FTryllLocalizedAgentSet::RefreshAgent(uint64 AgentId, UTryllTextProvider* Provider, const FSend& Send, const FFail& Fail, const FApplied& Applied)
{
	FBinding* Binding = ById.Find(AgentId);
	if (Binding == nullptr || Provider == nullptr || !Send)
	{
		return 0;
	}
	const TSharedPtr<FTryllAgent> Agent = Binding->Agent.Pin();
	if (!Agent.IsValid() || Agent->GetAgentId() == 0)
	{
		// Released without RequestDestroyAgent; nothing left to update.
		ById.Remove(AgentId);
		return 0;
	}
	Binding->bDirty = false;
	const uint32 Generation = ++Binding->Generation;
	const FName Table = Binding->Table;
	const FString AssetName = Binding->AssetName;

	// Start from what the agent runs now, so a runtime ChangeParams (temperature,
	// a swapped prompt) survives. Resolve everything first: one failed field sends
	// nothing, so the agent never runs half in one language and half in another.
	TArray<TPair<FString, TStrongObjectPtr<UTryllNodeParamsBase>>> Changed;
	for (const FString& NodeName : Agent->GetNodeParamsBaselineNames())
	{
		UTryllNodeParamsBase* Baseline = Agent->GetNodeParamsBaseline(NodeName);
		if (!FTryllTextResolve::HasTickedFields(Baseline, /*bLiveOnly=*/true))
		{
			continue;
		}
		TStrongObjectPtr<UTryllNodeParamsBase> Clone(
			DuplicateObject<UTryllNodeParamsBase>(Baseline, GetTransientPackage()));
		bool bChanged = false;
		FTryllError Error;
		if (!FTryllTextResolve::ResolveParams(Clone.Get(), Provider, Table, AssetName, NodeName,
				/*bLiveOnly=*/true, bChanged, Error))
		{
			UE_LOG(LogTryll, Warning, TEXT("%s (agent %llu not updated)"), *Error.Message,
				static_cast<unsigned long long>(AgentId));
			if (Fail)
			{
				Fail(AgentId, AssetName, Error);
			}
			return 0;
		}
		if (bChanged)
		{
			Changed.Emplace(NodeName, MoveTemp(Clone));
		}
	}

	if (Changed.Num() == 0)
	{
		if (Applied)
		{
			Applied(AgentId, AssetName);
		}
		return 0;
	}

	struct FPending
	{
		int32 Remaining = 0;
		bool bBusy = false;
		FTryllError Error;
	};
	const TSharedRef<FPending> Pending = MakeShared<FPending>();
	Pending->Remaining = Changed.Num();
	const TWeakPtr<int32> Alive = AliveToken;
	for (TPair<FString, TStrongObjectPtr<UTryllNodeParamsBase>>& Item : Changed)
	{
		const FString NodeName = Item.Key;
		Send(Agent, NodeName, Item.Value.Get(),
			[this, Alive, AgentId, AssetName, NodeName, Generation, Pending, Fail, Applied](const FTryllError& Error)
			{
				if (!Alive.IsValid())
				{
					return;
				}
				if (Error.Code == static_cast<int32>(ETryllErrorCode::AgentBusy))
				{
					// Redone from the then-current baseline when the turn ends.
					MarkDirty(AgentId);
					Pending->bBusy = true;
				}
				else if (!Error.IsOk())
				{
					UE_LOG(LogTryll, Warning, TEXT("Localized text: node '%s' of agent %llu was not updated: %s"),
						*NodeName, static_cast<unsigned long long>(AgentId), *Error.Message);
					if (Pending->Error.IsOk())
					{
						Pending->Error = Error;
					}
				}
				if (--Pending->Remaining > 0 || Pending->bBusy)
				{
					return;
				}
				const FBinding* Current = ById.Find(AgentId);
				if (Current == nullptr || Current->Generation != Generation)
				{
					return;
				}
				if (!Pending->Error.IsOk())
				{
					if (Fail)
					{
						Fail(AgentId, AssetName, Pending->Error);
					}
					return;
				}
				if (Applied)
				{
					Applied(AgentId, AssetName);
				}
			});
	}
	return Changed.Num();
}
