// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllAgentVariables.h"
#include "TryllAgent.h"
#include "TryllClientModule.h"

namespace
{
	const TCHAR* VariableTypeName(ETryllVariableType Type)
	{
		switch (Type)
		{
		case ETryllVariableType::Int:       return TEXT("Int");
		case ETryllVariableType::Float:     return TEXT("Float");
		case ETryllVariableType::String:    return TEXT("String");
		case ETryllVariableType::Bool:      return TEXT("Bool");
		case ETryllVariableType::StringSet: return TEXT("StringSet");
		default:                            return TEXT("Unknown");
		}
	}
}

FTryllAgentVariables::FTryllAgentVariables(FTryllAgent* InAgent, const TArray<FTryllVariableDecl>& Declared)
	: Agent(InAgent)
{
	for (const FTryllVariableDecl& D : Declared)
	{
		if (D.Name.IsEmpty())
			continue;
		FTryllVariableDecl Normalized = D;
		if (D.Type == ETryllVariableType::StringSet)
			Normalized.StringSetValue = NormalizeSet(D.StringSetValue);
		DeclaredTypes.Add(D.Name, D.Type);
		Initial.Add(D.Name, Normalized);
		Values.Add(D.Name, Normalized);
	}
}

TArray<FString> FTryllAgentVariables::NormalizeSet(const TArray<FString>& In)
{
	TSet<FString> Unique;
	for (const FString& S : In)
		Unique.Add(S);
	TArray<FString> Out = Unique.Array();
	Out.Sort();
	return Out;
}

TOptional<ETryllVariableType> FTryllAgentVariables::DeclaredType(const FString& Name) const
{
	if (const ETryllVariableType* Found = DeclaredTypes.Find(Name))
		return *Found;
	return {};
}

int64 FTryllAgentVariables::GetInt(const FString& Name) const
{
	const FTryllVariableDecl* V = Values.Find(Name);
	return V ? V->IntValue : 0;
}

double FTryllAgentVariables::GetFloat(const FString& Name) const
{
	const FTryllVariableDecl* V = Values.Find(Name);
	return V ? V->FloatValue : 0.0;
}

FString FTryllAgentVariables::GetString(const FString& Name) const
{
	const FTryllVariableDecl* V = Values.Find(Name);
	return V ? V->StringValue : FString{};
}

bool FTryllAgentVariables::GetBool(const FString& Name) const
{
	const FTryllVariableDecl* V = Values.Find(Name);
	return V ? V->bBoolValue : false;
}

TArray<FString> FTryllAgentVariables::GetStringSet(const FString& Name) const
{
	const FTryllVariableDecl* V = Values.Find(Name);
	return V ? V->StringSetValue : TArray<FString>{};
}

FTryllError FTryllAgentVariables::UnknownVariableError(const FString& Name)
{
	return FTryllError(ETryllErrorCode::UnknownVariable,
		FString::Printf(TEXT("unknown agent variable '%s'"), *Name));
}

FTryllError FTryllAgentVariables::RequireType(const FString& Name, ETryllVariableType Expected) const
{
	const ETryllVariableType* Actual = DeclaredTypes.Find(Name);
	if (!Actual)
		return UnknownVariableError(Name);
	if (*Actual != Expected)
	{
		return FTryllError(ETryllErrorCode::VariableTypeMismatch,
			FString::Printf(TEXT("variable '%s' is declared %s — cannot use as %s"),
				*Name, VariableTypeName(*Actual), VariableTypeName(Expected)));
	}
	return FTryllError();
}

FTryllAgentVariables::FPendingEntry& FTryllAgentVariables::TouchPending(const FString& Name)
{
	if (!PendingByName.Contains(Name))
	{
		PendingByName.Add(Name, FPendingEntry{});
		PendingOrder.Add(Name);
	}
	if (const int32* Existing = MutationSeq.Find(Name))
	{
		MutationSeq[Name] = *Existing + 1;
	}
	else
	{
		MutationSeq.Add(Name, 1);
	}
	return PendingByName[Name];
}

void FTryllAgentVariables::MarkWholesale(const FString& Name, const TCHAR* Mode)
{
	FPendingEntry& E = TouchPending(Name);
	E.Mode = Mode;
	E.Adds.Reset();
	E.Removes.Reset();
}

FTryllError FTryllAgentVariables::SetInt(const FString& Name, int64 Value)
{
	FTryllError Err = RequireType(Name, ETryllVariableType::Int);
	if (!Err.IsOk()) return Err;
	Values[Name].IntValue = Value;
	MarkWholesale(Name, TEXT("assign"));
	return FTryllError();
}

FTryllError FTryllAgentVariables::SetFloat(const FString& Name, double Value)
{
	FTryllError Err = RequireType(Name, ETryllVariableType::Float);
	if (!Err.IsOk()) return Err;
	Values[Name].FloatValue = Value;
	MarkWholesale(Name, TEXT("assign"));
	return FTryllError();
}

FTryllError FTryllAgentVariables::SetString(const FString& Name, const FString& Value)
{
	FTryllError Err = RequireType(Name, ETryllVariableType::String);
	if (!Err.IsOk()) return Err;
	Values[Name].StringValue = Value;
	MarkWholesale(Name, TEXT("assign"));
	return FTryllError();
}

FTryllError FTryllAgentVariables::SetBool(const FString& Name, bool bValue)
{
	FTryllError Err = RequireType(Name, ETryllVariableType::Bool);
	if (!Err.IsOk()) return Err;
	Values[Name].bBoolValue = bValue;
	MarkWholesale(Name, TEXT("assign"));
	return FTryllError();
}

FTryllError FTryllAgentVariables::SetStringSet(const FString& Name, const TArray<FString>& NewValues)
{
	FTryllError Err = RequireType(Name, ETryllVariableType::StringSet);
	if (!Err.IsOk()) return Err;
	Values[Name].StringSetValue = NormalizeSet(NewValues);
	MarkWholesale(Name, TEXT("assign"));
	return FTryllError();
}

FTryllError FTryllAgentVariables::AddToSet(const FString& Name, const FString& Element)
{
	FTryllError Err = RequireType(Name, ETryllVariableType::StringSet);
	if (!Err.IsOk()) return Err;
	TArray<FString>& Set = Values[Name].StringSetValue;
	Set.AddUnique(Element);
	Set.Sort();
	FPendingEntry& E = TouchPending(Name);
	if (E.Mode.IsEmpty())
	{
		E.Removes.Remove(Element);
		E.Adds.Add(Element);
	}
	else
	{
		// An edit after a wholesale assign/reset: fold into an assign of the
		// final mirror value (a pure Reset no longer describes it).
		E.Mode = TEXT("assign");
	}
	return FTryllError();
}

FTryllError FTryllAgentVariables::RemoveFromSet(const FString& Name, const FString& Element)
{
	FTryllError Err = RequireType(Name, ETryllVariableType::StringSet);
	if (!Err.IsOk()) return Err;
	Values[Name].StringSetValue.Remove(Element);
	FPendingEntry& E = TouchPending(Name);
	if (E.Mode.IsEmpty())
	{
		E.Adds.Remove(Element);
		E.Removes.Add(Element);
	}
	else
	{
		E.Mode = TEXT("assign");
	}
	return FTryllError();
}

FTryllError FTryllAgentVariables::Reset(const FString& Name)
{
	const ETryllVariableType* Actual = DeclaredTypes.Find(Name);
	if (!Actual)
		return UnknownVariableError(Name);
	if (const FTryllVariableDecl* Init = Initial.Find(Name))
		Values[Name] = *Init;
	MarkWholesale(Name, TEXT("reset"));
	return FTryllError();
}

TArray<FTryllVariableUpdate> FTryllAgentVariables::BuildWireBatch() const
{
	TArray<FTryllVariableUpdate> Wire;
	for (const FString& Name : PendingOrder)
	{
		const FPendingEntry* E = PendingByName.Find(Name);
		if (!E) continue;
		const ETryllVariableType Type = DeclaredTypes[Name];
		const FTryllVariableDecl& V = Values[Name];

		if (E->Mode == TEXT("reset"))
		{
			// Preserve the explicit Reset — smaller wire than assigning a large
			// initial set, and clearer intent.
			FTryllVariableUpdate U;
			U.Name = Name; U.Op = ETryllVariableOp::Reset;
			Wire.Add(MoveTemp(U));
			continue;
		}
		if (E->Mode == TEXT("assign"))
		{
			FTryllVariableUpdate U;
			U.Name = Name; U.Op = ETryllVariableOp::Assign; U.ValueType = Type;
			U.IntValue = V.IntValue; U.FloatValue = V.FloatValue; U.StringValue = V.StringValue;
			U.bBoolValue = V.bBoolValue; U.StringSetValue = V.StringSetValue;
			Wire.Add(MoveTemp(U));
			continue;
		}

		// Pure set delta — incremental ops, unless a whole-set assign encodes
		// smaller (fewer elements): §6.3 "smaller encoding".
		TArray<FString> Adds = E->Adds.Array();   Adds.Sort();
		TArray<FString> Removes = E->Removes.Array(); Removes.Sort();
		if (Adds.Num() + Removes.Num() <= V.StringSetValue.Num())
		{
			for (const FString& El : Adds)
			{
				FTryllVariableUpdate U;
				U.Name = Name; U.Op = ETryllVariableOp::AddToSet; U.ValueType = ETryllVariableType::String; U.StringValue = El;
				Wire.Add(MoveTemp(U));
			}
			for (const FString& El : Removes)
			{
				FTryllVariableUpdate U;
				U.Name = Name; U.Op = ETryllVariableOp::RemoveFromSet; U.ValueType = ETryllVariableType::String; U.StringValue = El;
				Wire.Add(MoveTemp(U));
			}
		}
		else
		{
			FTryllVariableUpdate U;
			U.Name = Name; U.Op = ETryllVariableOp::Assign; U.ValueType = ETryllVariableType::StringSet;
			U.StringSetValue = V.StringSetValue;
			Wire.Add(MoveTemp(U));
		}
	}
	return Wire;
}

bool FTryllAgentVariables::IsBatchCurrent(const FString& Name, const TMap<FString, int32>& BatchSeq) const
{
	const int32 Batch = BatchSeq.FindRef(Name);
	const int32 Current = MutationSeq.FindRef(Name);
	return Current == Batch;
}

void FTryllAgentVariables::FlushIfDirty(
	TFunction<void(const FTryllError&)> OnComplete,
	uint64 CarrierRequestId)
{
	if (PendingOrder.Num() == 0)
	{
		if (OnComplete) OnComplete(FTryllError());
		return;
	}
	if (!TestSendUpdate && (!Agent || !Agent->IsValid()))
	{
		if (OnComplete) OnComplete(FTryllError(ETryllErrorCode::InvalidAgentId, TEXT("Agent not ready")));
		return;
	}

	TArray<FTryllVariableUpdate> Wire = BuildWireBatch();
	// Capture this request's coalesced batch for recovery. Leave pending clear
	// so later writes stage independently; overlapping flushes each own their
	// snapshot instead of sharing one mutable in-flight slot.
	TArray<FString> BatchOrder = MoveTemp(PendingOrder);
	TMap<FString, FPendingEntry> BatchByName = MoveTemp(PendingByName);
	PendingOrder.Reset();
	PendingByName.Reset();

	TMap<FString, int32> BatchSeq;
	for (const FString& Name : BatchOrder)
	{
		BatchSeq.Add(Name, MutationSeq.FindRef(Name));
		if (int32* Count = InFlightRefCounts.Find(Name))
		{
			++(*Count);
		}
		else
		{
			InFlightRefCounts.Add(Name, 1);
		}
	}

	auto OpName = [](ETryllVariableOp Op) -> const TCHAR*
	{
		switch (Op)
		{
		case ETryllVariableOp::Assign:        return TEXT("Assign");
		case ETryllVariableOp::AddToSet:      return TEXT("AddToSet");
		case ETryllVariableOp::RemoveFromSet: return TEXT("RemoveFromSet");
		case ETryllVariableOp::Reset:         return TEXT("Reset");
		default:                              return TEXT("?");
		}
	};
	TArray<FString> Parts;
	Parts.Reserve(Wire.Num());
	for (const FTryllVariableUpdate& U : Wire)
	{
		Parts.Add(FString::Printf(TEXT("%s:%s"), *U.Name, OpName(U.Op)));
	}
	const uint64 AgentId = Agent ? Agent->GetAgentId() : 0ull;
	UE_LOG(LogTryll, Log,
		TEXT("AgentVariables: flushing %d update(s) for agent %llu: %s"),
		Wire.Num(), AgentId, *FString::Join(Parts, TEXT(", ")));

	const bool bAutoFlush = !OnComplete;
	FTryllAgent* AgentPtr = Agent;
	auto OnWireComplete =
		[this, AgentPtr, bAutoFlush, OnComplete = MoveTemp(OnComplete),
			BatchOrder = MoveTemp(BatchOrder), BatchByName = MoveTemp(BatchByName),
			BatchSeq = MoveTemp(BatchSeq)](const FTryllError& Error) mutable
		{
			HandleFlushResult(Error, MoveTemp(BatchOrder), MoveTemp(BatchByName), MoveTemp(BatchSeq));
			if (Error.IsOk())
			{
				UE_LOG(LogTryll, Log,
					TEXT("AgentVariables: flush ack OK (agent %llu)"),
					AgentPtr ? AgentPtr->GetAgentId() : 0ull);
			}
			else
			{
				UE_LOG(LogTryll, Warning,
					TEXT("AgentVariables: flush rejected (agent %llu): %d %s"),
					AgentPtr ? AgentPtr->GetAgentId() : 0ull, Error.Code, *Error.Message);
			}
			// Auto-flush (no caller callback): surface failures on the agent's
			// OnError so a UTryllAgentComponent.OnError listener sees them, not
			// only the global subsystem error channel.
			if (!Error.IsOk() && bAutoFlush && AgentPtr)
				AgentPtr->DispatchError(Error);
			if (OnComplete) OnComplete(Error);
		};

	if (TestSendUpdate)
	{
		TestSendUpdate(MoveTemp(Wire), MoveTemp(OnWireComplete));
	}
	else
	{
		Agent->SendUpdateVariables(MoveTemp(Wire), MoveTemp(OnWireComplete), CarrierRequestId);
	}
}

void FTryllAgentVariables::HandleFlushResult(
	const FTryllError& Error,
	TArray<FString> BatchOrder,
	TMap<FString, FPendingEntry> BatchByName,
	TMap<FString, int32> BatchSeq)
{
	for (const FString& Name : BatchOrder)
	{
		if (int32* Count = InFlightRefCounts.Find(Name))
		{
			if (*Count <= 1)
			{
				InFlightRefCounts.Remove(Name);
			}
			else
			{
				--(*Count);
			}
		}
	}

	if (Error.IsOk())
		return;

	if (Error.Code == static_cast<int32>(ETryllErrorCode::AgentBusy))
	{
		// Transient: re-stage entries not superseded by a newer local mutation
		// (pending or a later flush), to retry on the next flush.
		for (const FString& Name : BatchOrder)
		{
			if (!IsBatchCurrent(Name, BatchSeq)) continue;
			if (PendingByName.Contains(Name)) continue;
			PendingByName.Add(Name, MoveTemp(BatchByName[Name]));
			PendingOrder.Add(Name);
		}
		return;
	}

	// Hard rejection: re-sync names still matching this batch's mutation
	// generation so the optimistic mirror is reconciled.
	TArray<FString> Resync;
	for (const FString& Name : BatchOrder)
		if (IsBatchCurrent(Name, BatchSeq) && !PendingByName.Contains(Name))
			Resync.Add(Name);
	if (Resync.Num() > 0)
		SyncFromServerAsync(MoveTemp(Resync));
}

void FTryllAgentVariables::SyncFromServerAsync(TArray<FString> Names, TFunction<void(const FTryllError&)> OnComplete)
{
	if (!Agent || !Agent->IsValid())
	{
		if (OnComplete) OnComplete(FTryllError(ETryllErrorCode::InvalidAgentId, TEXT("Agent not ready")));
		return;
	}
	Agent->SendGetVariables(MoveTemp(Names),
		[this, OnComplete = MoveTemp(OnComplete)](const TArray<FTryllVariableDecl>& Variables, const FTryllError& Error)
		{
			if (Error.IsOk())
			{
				for (const FTryllVariableDecl& V : Variables)
				{
					if (V.Name.IsEmpty()) continue;
					// Never clobber a name with a locally newer write.
					if (PendingByName.Contains(V.Name) || InFlightRefCounts.Contains(V.Name)) continue;
					FTryllVariableDecl Normalized = V;
					if (const ETryllVariableType* T = DeclaredTypes.Find(V.Name))
						if (*T == ETryllVariableType::StringSet)
							Normalized.StringSetValue = NormalizeSet(V.StringSetValue);
					Values.Add(V.Name, Normalized);
				}
			}
			if (OnComplete) OnComplete(Error);
		});
}

// ── UTryllAgentVariables (Blueprint wrapper) ────────────────────────────────

FTryllError UTryllAgentVariables::SetIntVariable(const FString& Name, int64 Value)
{
	if (TSharedPtr<FTryllAgent> A = Agent.Pin())
		return A->GetVariables().SetInt(Name, Value);
	return FTryllError(ETryllErrorCode::InvalidAgentId, TEXT("Agent not ready"));
}

FTryllError UTryllAgentVariables::SetFloatVariable(const FString& Name, float Value)
{
	if (TSharedPtr<FTryllAgent> A = Agent.Pin())
		return A->GetVariables().SetFloat(Name, static_cast<double>(Value));
	return FTryllError(ETryllErrorCode::InvalidAgentId, TEXT("Agent not ready"));
}

FTryllError UTryllAgentVariables::SetStringVariable(const FString& Name, const FString& Value)
{
	if (TSharedPtr<FTryllAgent> A = Agent.Pin())
		return A->GetVariables().SetString(Name, Value);
	return FTryllError(ETryllErrorCode::InvalidAgentId, TEXT("Agent not ready"));
}

FTryllError UTryllAgentVariables::SetBoolVariable(const FString& Name, bool bValue)
{
	if (TSharedPtr<FTryllAgent> A = Agent.Pin())
		return A->GetVariables().SetBool(Name, bValue);
	return FTryllError(ETryllErrorCode::InvalidAgentId, TEXT("Agent not ready"));
}

FTryllError UTryllAgentVariables::SetStringSetVariable(const FString& Name, const TArray<FString>& Values)
{
	if (TSharedPtr<FTryllAgent> A = Agent.Pin())
		return A->GetVariables().SetStringSet(Name, Values);
	return FTryllError(ETryllErrorCode::InvalidAgentId, TEXT("Agent not ready"));
}

FTryllError UTryllAgentVariables::AddToSetVariable(const FString& Name, const FString& Element)
{
	if (TSharedPtr<FTryllAgent> A = Agent.Pin())
		return A->GetVariables().AddToSet(Name, Element);
	return FTryllError(ETryllErrorCode::InvalidAgentId, TEXT("Agent not ready"));
}

FTryllError UTryllAgentVariables::RemoveFromSetVariable(const FString& Name, const FString& Element)
{
	if (TSharedPtr<FTryllAgent> A = Agent.Pin())
		return A->GetVariables().RemoveFromSet(Name, Element);
	return FTryllError(ETryllErrorCode::InvalidAgentId, TEXT("Agent not ready"));
}

FTryllError UTryllAgentVariables::ResetVariable(const FString& Name)
{
	if (TSharedPtr<FTryllAgent> A = Agent.Pin())
		return A->GetVariables().Reset(Name);
	return FTryllError(ETryllErrorCode::InvalidAgentId, TEXT("Agent not ready"));
}

bool UTryllAgentVariables::IsDirty() const
{
	if (TSharedPtr<FTryllAgent> A = Agent.Pin())
		return A->GetVariables().IsDirty();
	return false;
}

void UTryllAgentVariables::FlushIfDirty()
{
	if (TSharedPtr<FTryllAgent> A = Agent.Pin())
		A->GetVariables().FlushIfDirty();
}

int64 UTryllAgentVariables::GetIntVariable(const FString& Name, bool& bFound) const
{
	if (TSharedPtr<FTryllAgent> A = Agent.Pin())
	{
		bFound = A->GetVariables().IsDeclared(Name);
		return A->GetVariables().GetInt(Name);
	}
	bFound = false;
	return 0;
}

float UTryllAgentVariables::GetFloatVariable(const FString& Name, bool& bFound) const
{
	if (TSharedPtr<FTryllAgent> A = Agent.Pin())
	{
		bFound = A->GetVariables().IsDeclared(Name);
		return static_cast<float>(A->GetVariables().GetFloat(Name));
	}
	bFound = false;
	return 0.0f;
}

FString UTryllAgentVariables::GetStringVariable(const FString& Name, bool& bFound) const
{
	if (TSharedPtr<FTryllAgent> A = Agent.Pin())
	{
		bFound = A->GetVariables().IsDeclared(Name);
		return A->GetVariables().GetString(Name);
	}
	bFound = false;
	return FString{};
}

bool UTryllAgentVariables::GetBoolVariable(const FString& Name, bool& bFound) const
{
	if (TSharedPtr<FTryllAgent> A = Agent.Pin())
	{
		bFound = A->GetVariables().IsDeclared(Name);
		return A->GetVariables().GetBool(Name);
	}
	bFound = false;
	return false;
}

TArray<FString> UTryllAgentVariables::GetStringSetVariable(const FString& Name, bool& bFound) const
{
	if (TSharedPtr<FTryllAgent> A = Agent.Pin())
	{
		bFound = A->GetVariables().IsDeclared(Name);
		return A->GetVariables().GetStringSet(Name);
	}
	bFound = false;
	return TArray<FString>{};
}
