// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllDialogLabIntent.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

const TCHAR* FTryllDialogLabIntent::TypeEmbed = TEXT("ClassifyIntentNode");
const TCHAR* FTryllDialogLabIntent::TypeLlm   = TEXT("ClassifyIntentLLMNode");

namespace
{
	TSharedPtr<FJsonObject> AsObject(const TSharedPtr<FJsonValue>& Value)
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		if (Value.IsValid() && Value->TryGetObject(Object) && Object != nullptr)
		{
			return *Object;
		}
		return nullptr;
	}

	TSharedPtr<FJsonObject> ChildObject(const TSharedPtr<FJsonObject>& Parent, const TCHAR* Field)
	{
		if (!Parent.IsValid())
		{
			return nullptr;
		}
		const TSharedPtr<FJsonObject>* Child = nullptr;
		return Parent->TryGetObjectField(Field, Child) && Child != nullptr ? *Child : nullptr;
	}

	TSharedPtr<FJsonValue> ChildField(const TSharedPtr<FJsonObject>& Parent, const TCHAR* Field)
	{
		return Parent.IsValid() ? Parent->TryGetField(Field) : nullptr;
	}

	TArray<TSharedPtr<FJsonValue>> ChildArray(const TSharedPtr<FJsonObject>& Parent, const TCHAR* Field)
	{
		if (!Parent.IsValid())
		{
			return {};
		}
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		return Parent->TryGetArrayField(Field, Values) && Values != nullptr
			? *Values : TArray<TSharedPtr<FJsonValue>>();
	}

	// Not "ReadString": TryllDialogLabImport.cpp has one with this exact signature, and
	// the two would redefine each other in a unity blob (td-042).
	FString ReadStringField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		FString Value;
		if (Object.IsValid())
		{
			Object->TryGetStringField(Field, Value);
		}
		return Value;
	}

	TOptional<double> ReadDouble(const TSharedPtr<FJsonValue>& Value)
	{
		if (!Value.IsValid() || Value->Type != EJson::Number)
		{
			return TOptional<double>();
		}
		return Value->AsNumber();
	}

	int32 LetterIndex(const FString& Letter)
	{
		if (Letter.Len() != 1)
		{
			return -1;
		}
		const TCHAR C = FChar::ToUpper(Letter[0]);
		if (C < TEXT('A') || C > TEXT('Z'))
		{
			return -1;
		}
		return C - TEXT('A');
	}

	void ParseEmbed(
		FTryllIntentExecution& Execution,
		const TSharedPtr<FJsonObject>& Parameters,
		const TSharedPtr<FJsonObject>& Classification)
	{
		Execution.StorageName   = ReadStringField(Parameters, TEXT("embedded_string_storage"));
		Execution.MetadataField = ReadStringField(Parameters, TEXT("metadata_field"));

		if (const TSharedPtr<FJsonObject> Winner = ChildObject(Classification, TEXT("winner")))
		{
			Execution.Intent     = ReadStringField(Winner, TEXT("intent"));
			Execution.RecordId   = ReadStringField(Winner, TEXT("id"));
			Execution.RecordText = ReadStringField(Winner, TEXT("text"));
			Execution.Distance   = ReadDouble(ChildField(Winner, TEXT("distance")));
			const TSharedPtr<FJsonValue> Index = ChildField(Winner, TEXT("index"));
			if (Index.IsValid() && Index->Type == EJson::Number)
			{
				Execution.RecordIndex = static_cast<int32>(Index->AsNumber());
			}
		}

		if (const TSharedPtr<FJsonObject> Runner = ChildObject(Classification, TEXT("runner_up")))
		{
			Execution.RunnerUpIntent   = ReadStringField(Runner, TEXT("intent"));
			Execution.RunnerUpDistance = ReadDouble(ChildField(Runner, TEXT("distance")));
			Execution.Margin           = ReadDouble(ChildField(Runner, TEXT("margin")));
		}

		for (const TSharedPtr<FJsonValue>& Row : ChildArray(Classification, TEXT("topk")))
		{
			const TSharedPtr<FJsonObject> Object = AsObject(Row);
			if (!Object.IsValid())
			{
				continue;
			}
			FTryllIntentCandidate Candidate;
			double Rank = 0.0;
			if (Object->TryGetNumberField(TEXT("rank"), Rank))
			{
				Candidate.Rank = static_cast<int32>(Rank);
			}
			Candidate.Intent   = ReadStringField(Object, TEXT("intent"));
			Candidate.Distance = ReadDouble(ChildField(Object, TEXT("distance")));
			Execution.Topk.Add(MoveTemp(Candidate));
		}

		Execution.RejectedIntent = ReadStringField(Classification, TEXT("rejected_intent"));
		const TOptional<double> RejectedDist =
			ReadDouble(ChildField(Classification, TEXT("rejected_distance")));
		if (!Execution.Distance.IsSet())
		{
			Execution.Distance = RejectedDist;
		}
		if (!Execution.Distance.IsSet() && Execution.Topk.Num() > 0)
		{
			Execution.Distance = Execution.Topk[0].Distance;
		}
	}

	void ParseLlm(
		FTryllIntentExecution& Execution,
		const TSharedPtr<FJsonObject>& Parameters,
		const TSharedPtr<FJsonObject>& Classification)
	{
		Execution.ModelName       = ReadStringField(Parameters, TEXT("model_name"));
		Execution.MarginThreshold = ReadDouble(ChildField(Parameters, TEXT("margin")));
		const TSharedPtr<FJsonValue> HistoryTurns = ChildField(Parameters, TEXT("history_turns"));
		if (HistoryTurns.IsValid() && HistoryTurns->Type == EJson::Number)
		{
			Execution.HistoryTurns = static_cast<int32>(HistoryTurns->AsNumber());
		}

		Execution.Intent          = ReadStringField(Classification, TEXT("intent"));
		Execution.RejectedIntent  = ReadStringField(Classification, TEXT("rejected_intent"));
		Execution.PredictedLetter = ReadStringField(Classification, TEXT("predicted_letter"));
		Execution.TopProb         = ReadDouble(ChildField(Classification, TEXT("top_prob")));
		Execution.SecondProb      = ReadDouble(ChildField(Classification, TEXT("second_prob")));
		Execution.MarginResult    = ReadDouble(ChildField(Classification, TEXT("margin_result")));

		for (const TSharedPtr<FJsonValue>& Id : ChildArray(Parameters, TEXT("intents_ids")))
		{
			FString Text;
			if (Id.IsValid())
			{
				Id->TryGetString(Text);
			}
			Execution.IntentsIds.Add(Text);
		}

		TArray<double> RawProbs;
		for (const TSharedPtr<FJsonValue>& Prob : ChildArray(Classification, TEXT("probs")))
		{
			if (!Prob.IsValid() || Prob->Type != EJson::Number)
			{
				continue;
			}
			RawProbs.Add(Prob->AsNumber());
		}

		Execution.ProbsCount      = RawProbs.Num();
		Execution.IntentsIdsCount = Execution.IntentsIds.Num();
		Execution.bProbsLengthMismatch = Execution.ProbsCount > 0 && Execution.IntentsIdsCount > 0
			&& Execution.ProbsCount != Execution.IntentsIdsCount;

		int32 Common = FMath::Min(
			Execution.ProbsCount > 0 ? Execution.ProbsCount : 0,
			Execution.IntentsIdsCount > 0 ? Execution.IntentsIdsCount : Execution.ProbsCount);
		// When ids are missing, still surface the numeric bars with blank labels.
		if (Execution.IntentsIdsCount == 0)
		{
			Common = Execution.ProbsCount;
		}
		if (Execution.ProbsCount == 0)
		{
			Common = 0;
		}

		for (int32 Index = 0; Index < Common; ++Index)
		{
			FTryllIntentLabelProb Entry;
			Entry.Label       = Execution.IntentsIds.IsValidIndex(Index)
				? Execution.IntentsIds[Index] : FString();
			Entry.Prob        = RawProbs[Index];
			Entry.SourceIndex = Index;
			Execution.Probs.Add(MoveTemp(Entry));
		}

		// Descending probability; ties keep intents_ids order (SourceIndex).
		Execution.Probs.Sort([](const FTryllIntentLabelProb& A, const FTryllIntentLabelProb& B)
		{
			if (A.Prob != B.Prob)
			{
				return A.Prob > B.Prob;
			}
			return A.SourceIndex < B.SourceIndex;
		});
	}

	FString RecoverRejected(
		const TSharedPtr<FJsonObject>& Parameters,
		const TSharedPtr<FJsonObject>& Classification)
	{
		const FString Rejected = ReadStringField(Classification, TEXT("rejected_intent"));
		if (!Rejected.IsEmpty())
		{
			return Rejected;
		}

		const TArray<TSharedPtr<FJsonValue>> Topk = ChildArray(Classification, TEXT("topk"));
		if (Topk.Num() > 0)
		{
			if (const TSharedPtr<FJsonObject> First = AsObject(Topk[0]))
			{
				const FString Intent = ReadStringField(First, TEXT("intent"));
				if (!Intent.IsEmpty())
				{
					return Intent;
				}
			}
		}

		const TArray<TSharedPtr<FJsonValue>> Ids = ChildArray(Parameters, TEXT("intents_ids"));
		const int32 MappedIndex = LetterIndex(ReadStringField(Classification, TEXT("predicted_letter")));
		if (MappedIndex >= 0 && Ids.IsValidIndex(MappedIndex) && Ids[MappedIndex].IsValid())
		{
			FString Mapped;
			Ids[MappedIndex]->TryGetString(Mapped);
			if (!Mapped.IsEmpty())
			{
				return Mapped;
			}
		}

		double Best = -TNumericLimits<double>::Max();
		int32 BestI = -1;
		const TArray<TSharedPtr<FJsonValue>> Probs = ChildArray(Classification, TEXT("probs"));
		for (int32 Index = 0; Index < Probs.Num(); ++Index)
		{
			if (!Probs[Index].IsValid() || Probs[Index]->Type != EJson::Number)
			{
				continue;
			}
			const double Prob = Probs[Index]->AsNumber();
			if (Prob > Best)
			{
				Best = Prob;
				BestI = Index;
			}
		}
		if (BestI >= 0 && Ids.IsValidIndex(BestI) && Ids[BestI].IsValid())
		{
			FString Mapped;
			Ids[BestI]->TryGetString(Mapped);
			return Mapped;
		}
		return FString();
	}
}

bool FTryllDialogLabIntent::IsClassifierType(const FString& Type)
{
	return Type == TypeEmbed || Type == TypeLlm;
}

FString FTryllDialogLabIntent::FormatDistance(TOptional<double> Value)
{
	return Value.IsSet()
		? FString::Printf(TEXT("%.4f"), Value.GetValue())
		: FString(TEXT("—"));
}

FString FTryllDialogLabIntent::FormatProb(TOptional<double> Value)
{
	return Value.IsSet()
		? FString::Printf(TEXT("%.3f"), Value.GetValue())
		: FString(TEXT("—"));
}

FString FTryllDialogLabIntent::ResolveLabel(const FTryllIntentExecution& Execution)
{
	return Execution.bAccepted ? Execution.Intent : Execution.RejectedIntent;
}

FString FTryllDialogLabIntent::FormatHeader(const TArray<FTryllIntentExecution>& Executions)
{
	if (Executions.Num() == 0)
	{
		return FString();
	}
	if (Executions.Num() == 1)
	{
		const FTryllIntentExecution& Execution = Executions[0];
		if (Execution.bAccepted)
		{
			const FString Label = Execution.Intent.IsEmpty()
				? ResolveLabel(Execution) : Execution.Intent;
			return Label.IsEmpty() ? FString(TEXT("found")) : Label + TEXT(" · found");
		}
		if (!Execution.NotFoundReason.IsEmpty())
		{
			return FString(TEXT("not_found · ")) + Execution.NotFoundReason;
		}
		return TEXT("not_found");
	}

	int32 Classified = 0;
	for (const FTryllIntentExecution& Execution : Executions)
	{
		if (Execution.bAccepted)
		{
			++Classified;
		}
	}
	return FString::Printf(TEXT("%d nodes · %d classified"), Executions.Num(), Classified);
}

FString FTryllDialogLabIntent::FormatEmptyMessage(const FTryllIntentExecution& Execution)
{
	const FString Reason = Execution.NotFoundReason;
	if (Reason == TEXT("empty_message"))
	{
		return TEXT("no query text to classify");
	}
	if (Reason == TEXT("no_results"))
	{
		return TEXT("the storage returned no hits");
	}
	if (Reason == TEXT("score_failed"))
	{
		return TEXT("scoring failed");
	}
	if (!Reason.IsEmpty())
	{
		return Reason;
	}
	return TEXT("no classification candidates");
}

FString FTryllDialogLabIntent::FormatMismatchWarning(const FTryllIntentExecution& Execution)
{
	if (!Execution.bProbsLengthMismatch)
	{
		return FString();
	}
	return FString::Printf(
		TEXT("probs/intents_ids length mismatch (%d vs %d) — labels may be misaligned"),
		Execution.ProbsCount, Execution.IntentsIdsCount);
}

FString FTryllDialogLabIntent::FormatEmbedSubHeader(const FTryllIntentExecution& Execution)
{
	const FString Query = Execution.Query.IsEmpty() ? FString(TEXT("(no query)")) : Execution.Query;
	FString Out = Query;
	Out += TEXT(" → ");
	Out += Execution.StorageName.IsEmpty() ? FString(TEXT("storage")) : Execution.StorageName;
	if (!Execution.MetadataField.IsEmpty())
	{
		Out += TEXT(" · ");
		Out += Execution.MetadataField;
	}
	Out += TEXT(" · threshold ");
	Out += Execution.Threshold.IsSet()
		? FString::Printf(TEXT("%.2f"), Execution.Threshold.GetValue())
		: FString(TEXT("—"));
	return Out;
}

FString FTryllDialogLabIntent::FormatLlmSubHeader(const FTryllIntentExecution& Execution)
{
	FString Out = Execution.ModelName.IsEmpty() ? FString(TEXT("model")) : Execution.ModelName;
	Out += TEXT(" · threshold ");
	Out += Execution.Threshold.IsSet()
		? FString::Printf(TEXT("%.2f"), Execution.Threshold.GetValue())
		: FString(TEXT("—"));
	Out += TEXT(" · margin ");
	Out += Execution.MarginThreshold.IsSet()
		? FString::Printf(TEXT("%.2f"), Execution.MarginThreshold.GetValue())
		: FString(TEXT("—"));
	Out += TEXT(" · history_turns ");
	Out += Execution.HistoryTurns.IsSet()
		? FString::FromInt(Execution.HistoryTurns.GetValue())
		: FString(TEXT("—"));
	return Out;
}

TArray<FTryllIntentExecution> FTryllDialogLabIntent::ParseExecutions(
	const TSharedPtr<FJsonObject>& /*DebugInfo*/,
	const TArray<TPair<int32, TSharedPtr<FJsonObject>>>& ScopedNodes)
{
	TArray<FTryllIntentExecution> List;
	for (const TPair<int32, TSharedPtr<FJsonObject>>& Entry : ScopedNodes)
	{
		if (!Entry.Value.IsValid())
		{
			continue;
		}
		FString Type;
		Entry.Value->TryGetStringField(TEXT("_type"), Type);
		if (!IsClassifierType(Type))
		{
			continue;
		}
		List.Add(ParseOne(Entry.Key, Entry.Value));
	}
	return List;
}

FTryllIntentExecution FTryllDialogLabIntent::ParseOne(
	int32 NodeIndex, const TSharedPtr<FJsonObject>& Node)
{
	FTryllIntentExecution Execution;
	if (!Node.IsValid())
	{
		return Execution;
	}

	Execution.NodeIndex = NodeIndex;
	Execution.NodeName  = ReadStringField(Node, TEXT("name"));
	Execution.ExitRoute = ReadStringField(Node, TEXT("exit_route"));
	Execution.bAccepted = Execution.ExitRoute == TEXT("found");

	const FString Type = ReadStringField(Node, TEXT("_type"));
	Execution.Kind = Type == TypeLlm ? FTryllIntentKind::Llm : FTryllIntentKind::Embedding;

	const TSharedPtr<FJsonObject> Diag           = ChildObject(Node, TEXT("diagnostics"));
	const TSharedPtr<FJsonObject> Parameters     = ChildObject(Diag, TEXT("parameters"));
	const TSharedPtr<FJsonObject> Output         = ChildObject(Diag, TEXT("output"));
	const TSharedPtr<FJsonObject> Classification = ChildObject(Output, TEXT("classification"));
	const TSharedPtr<FJsonObject> Input          = ChildObject(Diag, TEXT("input"));

	Execution.Query = ReadStringField(Input, TEXT("query"));
	if (Execution.Query.IsEmpty())
	{
		Execution.Query = ReadStringField(Classification, TEXT("query"));
	}

	Execution.NotFoundReason = ReadStringField(Classification, TEXT("not_found_reason"));
	Execution.Threshold      = ReadDouble(ChildField(Parameters, TEXT("threshold")));

	if (Execution.Kind == FTryllIntentKind::Embedding)
	{
		ParseEmbed(Execution, Parameters, Classification);
	}
	else
	{
		ParseLlm(Execution, Parameters, Classification);
	}

	if (Execution.bAccepted)
	{
		if (Execution.Intent.IsEmpty())
		{
			Execution.Intent = Execution.Kind == FTryllIntentKind::Embedding
				? ReadStringField(ChildObject(Classification, TEXT("winner")), TEXT("intent"))
				: ReadStringField(Classification, TEXT("intent"));
		}
	}
	else
	{
		Execution.Intent.Reset();
		if (Execution.RejectedIntent.IsEmpty())
		{
			Execution.RejectedIntent = RecoverRejected(Parameters, Classification);
		}
	}

	return Execution;
}

TArray<FString> FTryllDialogLabIntent::AttachedIntents(const TSharedPtr<FJsonObject>& DebugInfo)
{
	TArray<FString> Result;
	if (!DebugInfo.IsValid())
	{
		return Result;
	}
	const TSharedPtr<FJsonObject> Interaction = ChildObject(DebugInfo, TEXT("interaction"));
	for (const TSharedPtr<FJsonValue>& Component : ChildArray(Interaction, TEXT("components")))
	{
		const TSharedPtr<FJsonObject> Object = AsObject(Component);
		if (!Object.IsValid())
		{
			continue;
		}
		if (ReadStringField(Object, TEXT("_type")) != TEXT("IntentionComponent"))
		{
			continue;
		}
		const FString Intent = ReadStringField(Object, TEXT("intent"));
		if (!Intent.IsEmpty())
		{
			Result.Add(Intent);
		}
	}
	return Result;
}
