// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllSlotGraphAnalysis.h"

#include "TryllGraphDescription.h"
#include "TryllSlotInput.h"
#include "TryllSlotName.h"
#include "Generated/Nodes/TryllClassifyAttributesLLMParams.h"
#include "Generated/Nodes/TryllIntentToInstructionParams.h"
#include "Generated/Nodes/TryllNodeParamsBase.h"
#include "Generated/Nodes/TryllToolCallParams.h"
#include "UObject/UnrealType.h"

namespace
{
	/** Finds the (at most one) FTryllSlotName-typed property on a Params class. */
	const FStructProperty* FindSlotNameProperty(const UClass* Class)
	{
		if (!Class) return nullptr;
		for (TFieldIterator<FStructProperty> It(Class); It; ++It)
			if (It->Struct == FTryllSlotName::StaticStruct())
				return *It;
		return nullptr;
	}

	/** Visits every `meta=(GetOptions="GetExitTargetOptions")` string field's current value. */
	void VisitExitFieldValues(const UTryllNodeParamsBase* Params, TFunctionRef<void(const FString&)> Visitor)
	{
		if (!Params) return;
		for (TFieldIterator<FStrProperty> It(Params->GetClass()); It; ++It)
		{
			const FStrProperty* StrProp = *It;
#if WITH_EDITORONLY_DATA
			if (!StrProp->HasMetaData(TEXT("GetOptions")) ||
			    StrProp->GetMetaData(TEXT("GetOptions")) != TEXT("GetExitTargetOptions"))
				continue;
#endif
			if (const FString* Value = StrProp->ContainerPtrToValuePtr<FString>(Params))
				Visitor(*Value);
		}
	}

	TSet<FString> ReachableFrom(const FString& Start, const TMap<FString, TArray<FString>>& Forward)
	{
		TSet<FString> Visited;
		if (Start.IsEmpty() || !Forward.Contains(Start))
			return Visited;

		TArray<FString> Queue;
		Queue.Add(Start);
		Visited.Add(Start);
		while (Queue.Num() > 0)
		{
			const FString Cur = Queue.Pop();
			for (const FString& Next : Forward[Cur])
				if (!Visited.Contains(Next))
				{
					Visited.Add(Next);
					Queue.Add(Next);
				}
		}
		return Visited;
	}

	TSet<FString> AncestorsOf(const FString& Target, const TMap<FString, TArray<FString>>& Forward)
	{
		TMap<FString, TArray<FString>> Reverse;
		for (const auto& Kv : Forward)
			for (const FString& To : Kv.Value)
				Reverse.FindOrAdd(To).Add(Kv.Key);

		TSet<FString> Visited;
		const TArray<FString>* Preds = Reverse.Find(Target);
		if (!Preds) return Visited;

		TArray<FString> Queue = *Preds;
		for (const FString& P : *Preds)
			Visited.Add(P);

		while (Queue.Num() > 0)
		{
			const FString Cur = Queue.Pop();
			if (const TArray<FString>* Ps = Reverse.Find(Cur))
				for (const FString& P : *Ps)
					if (!Visited.Contains(P))
					{
						Visited.Add(P);
						Queue.Add(P);
					}
		}
		return Visited;
	}
}

FString FTryllSlotOption::ShortReason() const
{
	switch (Availability)
	{
	case ETryllSlotAvailability::Available:        return FString();
	case ETryllSlotAvailability::BranchDependent:   return TEXT("(branch-dependent)");
	case ETryllSlotAvailability::NotUpstream:       return TEXT("(not upstream)");
	case ETryllSlotAvailability::StartUnset:        return TEXT("(Start Node not set)");
	case ETryllSlotAvailability::StartUnreachable:  return TEXT("(unreachable from Start)");
	case ETryllSlotAvailability::Self:              return TEXT("(this node)");
	case ETryllSlotAvailability::Missing:           return TEXT("(not in graph)");
	default:                                        return FString();
	}
}

FString FTryllSlotOption::LongReason(const FString& TargetNodeName) const
{
	switch (Availability)
	{
	case ETryllSlotAvailability::Available:
		return FString::Printf(TEXT("Always available: every path to '%s' writes '%s' first."), *TargetNodeName, *Value);
	case ETryllSlotAvailability::BranchDependent:
		return FString::Printf(TEXT("May be empty: '%s' is only written on some paths to '%s'."), *Value, *TargetNodeName);
	case ETryllSlotAvailability::NotUpstream:
		return FString::Printf(TEXT("Unavailable: the producer of '%s' does not run before '%s'."), *Value, *TargetNodeName);
	case ETryllSlotAvailability::StartUnset:
		return TEXT("Set Graph > Start Node so upstream slots resolve.");
	case ETryllSlotAvailability::StartUnreachable:
		return FString::Printf(TEXT("Unavailable: the graph's Start Node can't reach the producer of '%s'."), *Value);
	case ETryllSlotAvailability::Self:
		return TEXT("A node can't read its own output slot.");
	case ETryllSlotAvailability::Missing:
		return FString::Printf(TEXT("'%s' does not match any producer slot in this graph (stale after a rename/rewire, or authored out-of-band)."), *Value);
	default:
		return FString();
	}
}

namespace
{
	/** Lower is more available; used to keep the best classification when de-duplicating by slot name. */
	int32 AvailabilityRank(ETryllSlotAvailability A)
	{
		switch (A)
		{
		case ETryllSlotAvailability::Available:       return 0;
		case ETryllSlotAvailability::BranchDependent:  return 1;
		case ETryllSlotAvailability::StartUnreachable: return 2;
		case ETryllSlotAvailability::StartUnset:       return 3;
		case ETryllSlotAvailability::NotUpstream:      return 4;
		case ETryllSlotAvailability::Self:             return 5;
		case ETryllSlotAvailability::Missing:          return 6;
		default:                                       return 7;
		}
	}
}

namespace TryllSlotGraphAnalysis
{
	FString GetEffectiveWriteSlot(const FTryllNodeDescription& Node)
	{
		if (!Node.Params) return FString();

		// ToolCall has no output_name field; CallOrAnswer is the only mode that
		// writes a slot (the node's own name, when the model answers instead of
		// calling a tool). Mirrors ToolCallNode::GetSlotMetadata.
		if (const UTryllToolCallParams* ToolCall = Cast<UTryllToolCallParams>(Node.Params))
			return ToolCall->Mode == ETryllToolCallMode::CallOrAnswer ? Node.Name : FString();

		const FStructProperty* SlotNameProp = FindSlotNameProperty(Node.Params->GetClass());
		if (!SlotNameProp) return FString();

		const FTryllSlotName* SlotName = SlotNameProp->ContainerPtrToValuePtr<FTryllSlotName>(Node.Params);
		const FString Override = SlotName ? SlotName->Value : FString();
		return Override.IsEmpty() ? Node.Name : Override;
	}

	void AppendWriteSlots(const FTryllNodeDescription& Node, TArray<FString>& Out)
	{
		if (const UTryllClassifyAttributesLLMParams* Attributes =
			Cast<UTryllClassifyAttributesLLMParams>(Node.Params))
		{
			const FString Prefix = Attributes->OutputName.Value.IsEmpty()
				? Node.Name : Attributes->OutputName.Value;
			for (const FTryllClassifyAttributeDef& Attribute : Attributes->Attributes)
			{
				if (!Attribute.Name.IsEmpty())
					Out.Add(Prefix + TEXT("_") + Attribute.Name);
			}
			return;
		}
		const FString Single = GetEffectiveWriteSlot(Node);
		if (!Single.IsEmpty())
			Out.Add(Single);
	}

	TArray<FString> GetUpstreamSlots(const FTryllGraphDescription& Graph, const FString& TargetNodeName)
	{
		TArray<FString> Result;
		Result.Add(TEXT("user_message"));
		for (const FTryllSlotOption& Option : ClassifySlots(Graph, TargetNodeName))
			if (Option.IsEnabled())
				Result.Add(Option.Value);
		return Result;
	}

	TArray<FTryllSlotOption> ClassifySlots(const FTryllGraphDescription& Graph, const FString& TargetNodeName, const FString& CurrentValue)
	{
		TMap<FString, TArray<FString>> Forward;
		TSet<FString> AllNames;
		for (const FTryllNodeDescription& Node : Graph.Nodes)
		{
			if (Node.Name.IsEmpty()) continue;
			AllNames.Add(Node.Name);
			Forward.FindOrAdd(Node.Name);
		}

		for (const FTryllNodeDescription& Node : Graph.Nodes)
		{
			if (Node.Name.IsEmpty() || !Node.Params) continue;
			VisitExitFieldValues(Node.Params, [&](const FString& Target)
			{
				if (!Target.IsEmpty() && AllNames.Contains(Target))
					Forward[Node.Name].Add(Target);
			});
		}

		const bool bStartSet = !Graph.StartNode.IsEmpty();
		const TSet<FString> ReachableFromStart = ReachableFrom(Graph.StartNode, Forward);
		const TSet<FString> Ancestors = AncestorsOf(TargetNodeName, Forward);

		TMap<FString, ETryllSlotAvailability> BySlot;
		TArray<FString> Order;

		for (const FTryllNodeDescription& Node : Graph.Nodes)
		{
			if (Node.Name.IsEmpty()) continue;
			TArray<FString> Slots;
			AppendWriteSlots(Node, Slots);
			for (const FString& Slot : Slots)
			{
			if (Slot.IsEmpty()) continue;

			ETryllSlotAvailability Availability;
			if (Node.Name == TargetNodeName)
			{
				Availability = ETryllSlotAvailability::Self;
			}
			else if (!Ancestors.Contains(Node.Name))
			{
				Availability = ETryllSlotAvailability::NotUpstream;
			}
			else if (!bStartSet)
			{
				Availability = ETryllSlotAvailability::StartUnset;
			}
			else if (!ReachableFromStart.Contains(Node.Name))
			{
				Availability = ETryllSlotAvailability::StartUnreachable;
			}
			else if (TryllSlotGraphAnalysis::IsMustAvailable(Graph, TargetNodeName, Slot))
			{
				Availability = ETryllSlotAvailability::Available;
			}
			else
			{
				Availability = ETryllSlotAvailability::BranchDependent;
			}

			if (const ETryllSlotAvailability* Existing = BySlot.Find(Slot))
			{
				if (AvailabilityRank(Availability) < AvailabilityRank(*Existing))
					BySlot[Slot] = Availability;
			}
			else
			{
				BySlot.Add(Slot, Availability);
				Order.Add(Slot);
			}
			}
		}

		TArray<FTryllSlotOption> Result;
		Result.Reserve(Order.Num() + 1);
		for (const FString& Slot : Order)
			Result.Add(FTryllSlotOption{ Slot, BySlot[Slot] });

		if (!CurrentValue.IsEmpty() && CurrentValue != TEXT("user_message") && !BySlot.Contains(CurrentValue))
			Result.Add(FTryllSlotOption{ CurrentValue, ETryllSlotAvailability::Missing });

		return Result;
	}

	bool IsMustAvailable(const FTryllGraphDescription& Graph, const FString& TargetNodeName, const FString& SlotName)
	{
		if (SlotName == TEXT("user_message")) return true;
		if (SlotName.IsEmpty()) return false;

		TMap<FString, TArray<FString>> Forward;
		TSet<FString> AllNames;
		for (const FTryllNodeDescription& Node : Graph.Nodes)
		{
			if (Node.Name.IsEmpty()) continue;
			AllNames.Add(Node.Name);
			Forward.FindOrAdd(Node.Name);
		}
		for (const FTryllNodeDescription& Node : Graph.Nodes)
		{
			if (Node.Name.IsEmpty() || !Node.Params) continue;
			VisitExitFieldValues(Node.Params, [&](const FString& Target)
			{
				if (!Target.IsEmpty() && AllNames.Contains(Target))
					Forward[Node.Name].Add(Target);
			});
		}

		// Conditional producers are excluded from the cut set: removing one as a vertex
		// would wrongly assume every path through it writes the slot, when it may not —
		// such a producer can only ever contribute to may-availability, never must.
		TSet<FString> Producers;
		for (const FTryllNodeDescription& Node : Graph.Nodes)
		{
			TArray<FString> Slots;
			AppendWriteSlots(Node, Slots);
			if (Slots.Contains(SlotName) && !IsWriteConditional(Node))
				Producers.Add(Node.Name);
		}
		if (Producers.Num() == 0) return false;

		if (Graph.StartNode.IsEmpty() || !Forward.Contains(Graph.StartNode))
			return false;

		TSet<FString> Visited;
		TArray<FString> Queue;
		Queue.Add(Graph.StartNode);
		Visited.Add(Graph.StartNode);
		while (Queue.Num() > 0)
		{
			const FString Cur = Queue.Pop();
			if (Producers.Contains(Cur))
				continue; // do not traverse past a producer while "avoiding" it
			for (const FString& Next : Forward[Cur])
				if (!Visited.Contains(Next))
				{
					Visited.Add(Next);
					Queue.Add(Next);
				}
		}

		// If target is reachable while avoiding every producer, some path never
		// produces the slot — not must-available.
		return !Visited.Contains(TargetNodeName);
	}

	bool IsWriteConditional(const FTryllNodeDescription& Node)
	{
		if (Cast<UTryllIntentToInstructionParams>(Node.Params) != nullptr)
			return true;
		if (const UTryllToolCallParams* ToolCall = Cast<UTryllToolCallParams>(Node.Params))
			return ToolCall->Mode == ETryllToolCallMode::CallOrAnswer;
		return false;
	}
}
