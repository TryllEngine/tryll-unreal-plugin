// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllWorkflowValidator.h"

#include "Generated/Nodes/TryllNodeParamsBase.h"
#include "Generated/Nodes/TryllGenerateParams.h"
#include "Generated/Nodes/TryllRegexGuardrailParams.h"
#include "Generated/Nodes/TryllCannedResponseParams.h"
#include "Generated/Nodes/TryllToolCallParams.h"
#include "Generated/Nodes/TryllRetrieveParams.h"
#include "Generated/Nodes/TryllInstructionParams.h"
#include "Generated/Nodes/TryllClassifyIntentParams.h"
#include "Generated/Nodes/TryllClassifyIntentLLMParams.h"
#include "Generated/Nodes/TryllIntentToInstructionParams.h"
#include "Generated/Nodes/TryllGenerateAndSpeakParams.h"
#include "Generated/Nodes/TryllSpeakParams.h"
#include "Generated/Nodes/TryllPauseParams.h"
#include "Generated/Nodes/TryllTransformParams.h"
#include "Generated/Nodes/TryllBranchParams.h"
#include "Generated/Nodes/TryllClassifyAttributesLLMParams.h"
#include "Generated/Nodes/TryllSummarizeParams.h"
#include "TryllSlotInput.h"
#include "TryllSlotName.h"
#include "TryllUncertainChoice.h"
#include "TryllRuntimeSettings.h"
#include "UObject/UnrealType.h"
#include "Internationalization/Regex.h"

DEFINE_LOG_CATEGORY_STATIC(LogTryllWorkflowValidator, Log, All);

// ── Internal helpers ──────────────────────────────────────────────────────────

namespace
{

/**
 * Visit each exit field on a typed params object and call Visitor(exitName, targetNodeName)
 * for every declared exit.  Uses Cast<> dispatch over all known param types.
 */
template <typename FVisitor>
void VisitExits(const UTryllNodeParamsBase* Params, FVisitor&& Visitor)
{
	if (!Params) return;

	if (const auto* GenP = Cast<UTryllGenerateParams>(Params))
	{
		Visitor(TEXT("default"), GenP->DefaultExit);
	}
	else if (const auto* GuardP = Cast<UTryllRegexGuardrailParams>(Params))
	{
		Visitor(TEXT("triggered"),     GuardP->TriggeredExit);
		Visitor(TEXT("not_triggered"), GuardP->NotTriggeredExit);
	}
	else if (const auto* CannedP = Cast<UTryllCannedResponseParams>(Params))
	{
		Visitor(TEXT("default"), CannedP->DefaultExit);
	}
	else if (const auto* ToolP = Cast<UTryllToolCallParams>(Params))
	{
		Visitor(TEXT("tool_called"),    ToolP->ToolCalledExit);
		Visitor(TEXT("no_tool_called"), ToolP->NoToolCalledExit);
	}
	else if (const auto* RetP = Cast<UTryllRetrieveParams>(Params))
	{
		Visitor(TEXT("found"),     RetP->FoundExit);
		Visitor(TEXT("not_found"), RetP->NotFoundExit);
	}
	else if (const auto* InstrP = Cast<UTryllInstructionParams>(Params))
	{
		Visitor(TEXT("default"), InstrP->DefaultExit);
	}
	else if (const auto* ClassifyP = Cast<UTryllClassifyIntentParams>(Params))
	{
		Visitor(TEXT("found"),     ClassifyP->FoundExit);
		Visitor(TEXT("not_found"), ClassifyP->NotFoundExit);
	}
	else if (const auto* ClassifyLLMP = Cast<UTryllClassifyIntentLLMParams>(Params))
	{
		Visitor(TEXT("found"),     ClassifyLLMP->FoundExit);
		Visitor(TEXT("not_found"), ClassifyLLMP->NotFoundExit);
	}
	else if (const auto* IntentP = Cast<UTryllIntentToInstructionParams>(Params))
	{
		Visitor(TEXT("default"), IntentP->DefaultExit);
	}
	else if (const auto* GenSpeakP = Cast<UTryllGenerateAndSpeakParams>(Params))
	{
		Visitor(TEXT("default"), GenSpeakP->DefaultExit);
	}
	else if (const auto* SpeakP = Cast<UTryllSpeakParams>(Params))
	{
		Visitor(TEXT("default"), SpeakP->DefaultExit);
	}
	else if (const auto* PauseP = Cast<UTryllPauseParams>(Params))
	{
		Visitor(TEXT("default"), PauseP->DefaultExit);
		// Validated like any other exit: an unresolvable timeout target would
		// otherwise only surface at CreateAgent as GraphCompilationFailed.
		Visitor(TEXT("timeout"), PauseP->TimeoutExit);
	}
	else if (const auto* TransformP = Cast<UTryllTransformParams>(Params))
	{
		Visitor(TEXT("default"), TransformP->DefaultExit);
	}
	else if (const auto* BranchP = Cast<UTryllBranchParams>(Params))
	{
		Visitor(TEXT("then"), BranchP->ThenExit);
		Visitor(TEXT("else"), BranchP->ElseExit);
	}
	else if (const auto* SummarizeP = Cast<UTryllSummarizeParams>(Params))
	{
		Visitor(TEXT("default"), SummarizeP->DefaultExit);
	}
	else if (const auto* AttrsP = Cast<UTryllClassifyAttributesLLMParams>(Params))
	{
		Visitor(TEXT("default"), AttrsP->DefaultExit);
		Visitor(TEXT("triggered"), AttrsP->TriggeredExit);
	}
}

/**
 * Visit each storage file-picker field on a typed params object and call
 * Visitor(FieldName, Value, bIsEmbedded) for every declared storage field.
 */
template <typename FVisitor>
void VisitStorageFields(const UTryllNodeParamsBase* Params, FVisitor&& Visitor)
{
	if (!Params) return;

	if (const auto* GuardP = Cast<UTryllRegexGuardrailParams>(Params))
		Visitor(TEXT("StringStorage"), GuardP->StringStorage.Path, false);
	else if (const auto* CannedP = Cast<UTryllCannedResponseParams>(Params))
		Visitor(TEXT("StringStorage"), CannedP->StringStorage.Path, false);
	else if (const auto* RetP = Cast<UTryllRetrieveParams>(Params))
		Visitor(TEXT("EmbeddedStringStorage"), RetP->EmbeddedStringStorage.Path, true);
	else if (const auto* ClassifyP = Cast<UTryllClassifyIntentParams>(Params))
		Visitor(TEXT("EmbeddedStringStorage"), ClassifyP->EmbeddedStringStorage.Path, true);
	else if (const auto* IntentP = Cast<UTryllIntentToInstructionParams>(Params))
		Visitor(TEXT("StringStorage"), IntentP->StringStorage.Path, false);
	// Voice clips live on the agent's UTryllVoiceProfile since protocol v27 and are
	// checked by FTryllVoiceProfileData::Validate.
}

/**
 * Visit the (at most one) FTryllSlotName-typed property value on a params object, via
 * generic UProperty reflection (no per-type Cast<> dispatch needed — new producer node
 * types need no changes here). Calls Visitor(FieldName, OverrideValue).
 */
template <typename FVisitor>
void VisitSlotNameField(const UTryllNodeParamsBase* Params, FVisitor&& Visitor)
{
	if (!Params) return;
	for (TFieldIterator<FStructProperty> It(Params->GetClass()); It; ++It)
	{
		if (It->Struct != FTryllSlotName::StaticStruct()) continue;
		if (const FTryllSlotName* Value = It->ContainerPtrToValuePtr<FTryllSlotName>(Params))
			Visitor(It->GetName(), Value->Value);
		return; // at most one per type
	}
}

/** Visits every FTryllSlotInput-typed property value on a params object (generic reflection). */
template <typename FVisitor>
void VisitSlotInputFields(const UTryllNodeParamsBase* Params, FVisitor&& Visitor)
{
	if (!Params) return;
	for (TFieldIterator<FStructProperty> It(Params->GetClass()); It; ++It)
	{
		if (It->Struct != FTryllSlotInput::StaticStruct()) continue;
		if (const FTryllSlotInput* Value = It->ContainerPtrToValuePtr<FTryllSlotInput>(Params))
			Visitor(It->GetName(), Value->Value);
	}
}

/** Effective write slot for a node: its FTryllSlotName override, or its own name when empty. */
FString GetEffectiveWriteSlot(const FTryllNodeDescription& Node)
{
	if (!Node.Params) return FString();
	FString Result;
	bool bFound = false;
	VisitSlotNameField(Node.Params, [&](const FString& /*FieldName*/, const FString& Override)
	{
		Result = Override.IsEmpty() ? Node.Name : Override;
		bFound = true;
	});
	return bFound ? Result : FString();
}

/** Every slot a node writes. ClassifyAttributesLLM writes one slot per attribute. */
void AppendProducedSlots(const FTryllNodeDescription& Node, TArray<FString>& Out)
{
	if (const auto* Attrs = Cast<UTryllClassifyAttributesLLMParams>(Node.Params))
	{
		const FString Prefix = Attrs->OutputName.Value.IsEmpty()
			? Node.Name : Attrs->OutputName.Value;
		for (const FTryllClassifyAttributeDef& Attribute : Attrs->Attributes)
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

const FRegexPattern& GrammarPattern()
{
	static const FRegexPattern Pattern(TEXT("^[A-Za-z_][A-Za-z0-9_]*$"));
	return Pattern;
}

bool IsValidGrammar(const FString& Name)
{
	if (Name.IsEmpty()) return false;
	FRegexMatcher Matcher(GrammarPattern(), Name);
	return Matcher.FindNext();
}

bool IsReservedName(const FString& Name)
{
	return Name == TEXT("user_message");
}

} // namespace

// ── FTryllWorkflowValidator::Validate ─────────────────────────────────────────

TArray<FTryllWorkflowIssue> FTryllWorkflowValidator::Validate(const FTryllGraphDescription& Graph)
{
	TArray<FTryllWorkflowIssue> Issues;

	const auto AddIssue = [&](FTryllWorkflowIssue::ESeverity Sev, FString Msg)
	{
		Issues.Add({ Sev, MoveTemp(Msg) });
	};

	// ── 1. StartNode ──────────────────────────────────────────────────────────
	if (Graph.StartNode.IsEmpty())
		AddIssue(FTryllWorkflowIssue::ESeverity::Error, TEXT("StartNode is empty."));

	// ── 2. Build name → index map; catch duplicates ───────────────────────────
	TMap<FString, int32> NameToIndex;
	for (int32 I = 0; I < Graph.Nodes.Num(); ++I)
	{
		const FString& Name = Graph.Nodes[I].Name;
		if (Name.IsEmpty())
		{
			AddIssue(FTryllWorkflowIssue::ESeverity::Error,
			         FString::Printf(TEXT("Nodes[%d] has an empty name."), I));
			continue;
		}
		if (NameToIndex.Contains(Name))
		{
			AddIssue(FTryllWorkflowIssue::ESeverity::Error,
			         FString::Printf(TEXT("Duplicate node name '%s' at indices %d and %d."),
			                         *Name, NameToIndex[Name], I));
		}
		else
		{
			NameToIndex.Add(Name, I);
		}
	}

	// ── 3. StartNode must resolve ─────────────────────────────────────────────
	if (!Graph.StartNode.IsEmpty() && !NameToIndex.Contains(Graph.StartNode))
	{
		AddIssue(FTryllWorkflowIssue::ESeverity::Error,
		         FString::Printf(TEXT("StartNode '%s' is not present in Nodes."), *Graph.StartNode));
	}

	// ── 4. Exit field validation; track reachability ──────────────────────────
	TSet<FString> Reachable;
	if (!Graph.StartNode.IsEmpty())
		Reachable.Add(Graph.StartNode);

	for (const FTryllNodeDescription& Node : Graph.Nodes)
	{
		VisitExits(Node.Params, [&](const TCHAR* ExitName, const FString& Target)
		{
			if (Target.IsEmpty())
				return; // empty = END, always valid

			if (!NameToIndex.Contains(Target))
			{
				AddIssue(FTryllWorkflowIssue::ESeverity::Error,
				         FString::Printf(TEXT("Node '%s' exit '%s' references unknown node '%s'."),
				                         *Node.Name, ExitName, *Target));
			}
			else
			{
				Reachable.Add(Target);
			}
		});
	}

	// ── 5. Orphan detection ───────────────────────────────────────────────────
	for (const FTryllNodeDescription& Node : Graph.Nodes)
	{
		if (Node.Name.IsEmpty()) continue;
		if (!Reachable.Contains(Node.Name))
		{
			AddIssue(FTryllWorkflowIssue::ESeverity::Warning,
			         FString::Printf(TEXT("Node '%s' is an orphan (no incoming exits and not the start node)."),
			                         *Node.Name));
		}
	}

	// ── 5b. Pausing graphs need game code ─────────────────────────────────────
	// A pause holds the turn open until Resume; nothing in the graph can end it.
	// The validator cannot see whether the game binds OnPaused, so this is an
	// advisory rather than an error — but an unresumed pause wedges the agent and
	// rejects every later Send Message with AgentBusy, so it is worth saying once.
	for (const FTryllNodeDescription& Node : Graph.Nodes)
	{
		const auto* PauseP = Cast<UTryllPauseParams>(Node.Params);
		if (PauseP == nullptr)
			continue;
		if (PauseP->PauseTimeoutMs > 0)
			continue;  // author gave it a deadline — it cannot hang
		AddIssue(FTryllWorkflowIssue::ESeverity::Warning,
		         FString::Printf(
			         TEXT("Node '%s' pauses the turn and has no Pause Timeout Ms: your game "
			              "must call Resume (bind UTryllSubsystem::OnPaused, or Register Tool a "
			              "handler for a pausing Tool Call). An unresumed pause holds the agent "
			              "open and rejects further Send Message calls with AgentBusy. Set a "
			              "Pause Timeout Ms (with a Timeout Exit for a fallback line) if the "
			              "game cannot guarantee a Resume."),
			         *Node.Name));
	}

	// ── 6. Grammar / reserved-name / duplicate-slot checks ─────────────────────
	TMap<FString, TArray<FString>> EffectiveSlots; // slot -> producing node names
	for (const FTryllNodeDescription& Node : Graph.Nodes)
	{
		if (!Node.Name.IsEmpty() && (!IsValidGrammar(Node.Name) || IsReservedName(Node.Name)))
		{
			AddIssue(FTryllWorkflowIssue::ESeverity::Error,
			         FString::Printf(TEXT("Node name '%s' must match [A-Za-z_][A-Za-z0-9_]* and not be the reserved 'user_message'."), *Node.Name));
		}

		if (!Node.Params) continue;
		VisitSlotNameField(Node.Params, [&](const FString& /*FieldName*/, const FString& Override)
		{
			if (!Override.IsEmpty() && (!IsValidGrammar(Override) || IsReservedName(Override)))
			{
				AddIssue(FTryllWorkflowIssue::ESeverity::Error,
				         FString::Printf(TEXT("Node '%s' slot override '%s' must match [A-Za-z_][A-Za-z0-9_]* and not be the reserved 'user_message'."),
				                         *Node.Name, *Override));
			}
		});

		TArray<FString> Produced;
		AppendProducedSlots(Node, Produced);
		for (const FString& Effective : Produced)
			EffectiveSlots.FindOrAdd(Effective).Add(Node.Name);
	}
	for (const auto& Kv : EffectiveSlots)
	{
		if (Kv.Value.Num() > 1)
		{
			AddIssue(FTryllWorkflowIssue::ESeverity::Warning,
			         FString::Printf(TEXT("Slot '%s' is produced by multiple nodes (%s); the effective value is whichever branch executes."),
			                         *Kv.Key, *FString::Join(Kv.Value, TEXT(", "))));
		}
	}

	// ── 7. May/must slot-availability checks for `input` selectors ─────────────
	// Template `{{slot.<name>}}` references are not scanned here: the Mustache scanner
	// is editor-only (TryllClientEditor); the editor's TryllWorkflowValidator.cs (Unity)
	// and the input-dropdown/rename tooling (Unreal, TryllClientEditor) cover that case.
	{
		TMap<FString, TArray<FString>> Forward;
		TSet<FString> AllNames;
		for (const FTryllNodeDescription& Node : Graph.Nodes)
			if (!Node.Name.IsEmpty())
			{
				AllNames.Add(Node.Name);
				Forward.FindOrAdd(Node.Name);
			}
		for (const FTryllNodeDescription& Node : Graph.Nodes)
		{
			VisitExits(Node.Params, [&](const TCHAR* /*ExitName*/, const FString& Target)
			{
				if (!Target.IsEmpty() && AllNames.Contains(Target))
					Forward[Node.Name].Add(Target);
			});
		}

		auto ReachableFrom = [&](const FString& From, const TSet<FString>* Avoid) -> TSet<FString>
		{
			TSet<FString> Visited;
			if (From.IsEmpty() || !Forward.Contains(From)) return Visited;
			TArray<FString> Queue{From};
			Visited.Add(From);
			while (Queue.Num() > 0)
			{
				const FString Cur = Queue.Pop();
				if (Avoid && Avoid->Contains(Cur)) continue;
				for (const FString& Next : Forward[Cur])
					if (!Visited.Contains(Next)) { Visited.Add(Next); Queue.Add(Next); }
			}
			return Visited;
		};

		for (const FTryllNodeDescription& Node : Graph.Nodes)
		{
			if (!Node.Params || Node.Name.IsEmpty()) continue;

			TArray<FString> Refs;
			VisitSlotInputFields(Node.Params, [&](const FString& /*FieldName*/, const FString& Value)
			{
				if (!Value.IsEmpty()) Refs.Add(Value);
			});
			if (Refs.Num() == 0) continue;

			// Ancestors of Node that are also reachable from Start (the may-set).
			TSet<FString> ReachableFromStart = ReachableFrom(Graph.StartNode, nullptr);
			TMap<FString, TArray<FString>> Reverse;
			for (const auto& Kv : Forward)
				for (const FString& To : Kv.Value)
					Reverse.FindOrAdd(To).Add(Kv.Key);
			TSet<FString> Ancestors;
			if (const TArray<FString>* Preds = Reverse.Find(Node.Name))
			{
				TArray<FString> Queue = *Preds;
				for (const FString& P : *Preds) Ancestors.Add(P);
				while (Queue.Num() > 0)
				{
					const FString Cur = Queue.Pop();
					if (const TArray<FString>* Ps = Reverse.Find(Cur))
						for (const FString& P : *Ps)
							if (!Ancestors.Contains(P)) { Ancestors.Add(P); Queue.Add(P); }
				}
			}

			for (const FString& Slot : Refs)
			{
				if (Slot == TEXT("user_message")) continue;

				TSet<FString> Producers;
				for (const FTryllNodeDescription& Other : Graph.Nodes)
				{
					TArray<FString> Produced;
					AppendProducedSlots(Other, Produced);
					if (Produced.Contains(Slot))
						Producers.Add(Other.Name);
				}

				bool bMayAvailable = false;
				for (const FString& P : Producers)
					if (Ancestors.Contains(P) && ReachableFromStart.Contains(P)) { bMayAvailable = true; break; }

				if (Producers.Num() == 0 || !bMayAvailable)
				{
					AddIssue(FTryllWorkflowIssue::ESeverity::Error,
					         FString::Printf(TEXT("Node '%s' references slot '%s', which is not producible on any path reaching this node."),
					                         *Node.Name, *Slot));
					continue;
				}

				const TSet<FString> AvoidingProducers = ReachableFrom(Graph.StartNode, &Producers);
				const bool bMustAvailable = !AvoidingProducers.Contains(Node.Name);
				if (!bMustAvailable)
				{
					AddIssue(FTryllWorkflowIssue::ESeverity::Warning,
					         FString::Printf(TEXT("Node '%s' references slot '%s', which is only produced on some branches reaching this node (may be empty at runtime)."),
					                         *Node.Name, *Slot));
				}
			}
		}
	}

	// ── 8. Storage file-picker field checks ──────────────────────────────────
	for (const FTryllNodeDescription& Node : Graph.Nodes)
	{
		VisitStorageFields(Node.Params, [&](const TCHAR* FieldName, const FString& Value, bool bIsEmbedded)
		{
			if (Value.IsEmpty()) return;

			if (!ValidateStoragePathShape(FString::Printf(TEXT("Node '%s'"), *Node.Name), FieldName, Value, Issues))
			{
				return;
			}
			const FString Normalized = UTryllRuntimeSettings::NormalizeStorageRelativePath(Value);

			// Embedded storages are referenced by a .json config (or the .kb.json records
			// array). Plain storages are .txt (List), .json (Map), or .mm.json (Multimap).
			// .kb.json is embedded-only.
			const bool bIsKbRecordsExt = Normalized.EndsWith(TEXT(".kb.json"), ESearchCase::IgnoreCase);
			const bool bIsJsonExt      = Normalized.EndsWith(TEXT(".json"), ESearchCase::IgnoreCase);
			const bool bIsTxtExt       = Normalized.EndsWith(TEXT(".txt"),  ESearchCase::IgnoreCase);
			const bool bHasKnownExt    = bIsJsonExt || bIsTxtExt;

			if (bHasKnownExt)
			{
				// Only flag a genuine kind mismatch: a plain-only .txt on an embedded field,
				// or the embedded-only .kb.json records file on a plain field. A .json path is
				// accepted on both (config for embedded, Map for plain).
				if (bIsEmbedded && bIsTxtExt)
					AddIssue(FTryllWorkflowIssue::ESeverity::Warning,
					         FString::Printf(TEXT("Node '%s' field '%s': '%s' looks like plain storage; expected a .json config for embedded fields."),
					                         *Node.Name, FieldName, *Value));
				else if (!bIsEmbedded && bIsKbRecordsExt)
					AddIssue(FTryllWorkflowIssue::ESeverity::Warning,
					         FString::Printf(TEXT("Node '%s' field '%s': '%s' looks like embedded storage records; expected .txt or .json for plain fields."),
					                         *Node.Name, FieldName, *Value));
			}
			else
			{
				AddIssue(FTryllWorkflowIssue::ESeverity::Warning,
				         FString::Printf(TEXT("Node '%s' field '%s' value '%s' has no recognized extension; if this is a virtual name it must be registered via CreateStringStorageRequest before CreateAgent."),
				                         *Node.Name, FieldName, *Value));
			}
		});
	}

	// ── 9. Summarize cross-field coherence ──────────────────────────────────
	for (const FTryllNodeDescription& Node : Graph.Nodes)
	{
		const auto* Summarize = Cast<UTryllSummarizeParams>(Node.Params);
		if (!Summarize)
			continue;

		if (Summarize->TriggerTokens < 0 || Summarize->TriggerTurns < 0)
		{
			AddIssue(FTryllWorkflowIssue::ESeverity::Error,
				FString::Printf(
					TEXT("Node '%s': trigger_tokens and trigger_turns must be >= 0."),
					*Node.Name));
		}
		else if (Summarize->TriggerTokens == 0 && Summarize->TriggerTurns == 0)
		{
			AddIssue(FTryllWorkflowIssue::ESeverity::Warning,
				FString::Printf(
					TEXT("Node '%s': both triggers are 0, so this node never folds."),
					*Node.Name));
		}

		if (Summarize->KeepRecentTurns < 0 || Summarize->KeepRecentTurns > 512)
		{
			AddIssue(FTryllWorkflowIssue::ESeverity::Error,
				FString::Printf(
					TEXT("Node '%s': keep_recent_turns must be between 0 and 512."),
					*Node.Name));
		}
		if (Summarize->MaxSummaryTokens < 16)
		{
			AddIssue(FTryllWorkflowIssue::ESeverity::Error,
				FString::Printf(
					TEXT("Node '%s': max_summary_tokens must be >= 16."),
					*Node.Name));
		}
		else if (Summarize->ContextSize > 0
			&& static_cast<int64>(Summarize->MaxSummaryTokens) + 64
				>= Summarize->ContextSize)
		{
			AddIssue(FTryllWorkflowIssue::ESeverity::Error,
				FString::Printf(
					TEXT("Node '%s': max_summary_tokens must leave at least 64 tokens ")
					TEXT("in context_size for the summarizer prompt."),
					*Node.Name));
		}
		if (!Summarize->bRunInline)
		{
			AddIssue(FTryllWorkflowIssue::ESeverity::Error,
				FString::Printf(
					TEXT("Node '%s': run_inline=false is reserved but not implemented; ")
					TEXT("enable run_inline."),
					*Node.Name));
		}
	}

	// ── 10. ClassifyAttributesLLM shape ─────────────────────────────────────
	for (const FTryllNodeDescription& Node : Graph.Nodes)
	{
		const auto* Attrs = Cast<UTryllClassifyAttributesLLMParams>(Node.Params);
		if (!Attrs)
			continue;
		const int32 Count = Attrs->Attributes.Num();
		if (Count < 1 || Count > 16)
		{
			AddIssue(FTryllWorkflowIssue::ESeverity::Error,
				FString::Printf(
					TEXT("Node '%s': attributes must contain 1 to 16 entries."),
					*Node.Name));
		}
		TSet<FString> Names;
		bool bCanTrigger = false;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FTryllClassifyAttributeDef& Attribute = Attrs->Attributes[Index];
			const FString Prefix = FString::Printf(
				TEXT("Node '%s' attributes[%d]"), *Node.Name, Index);
			if (!IsValidGrammar(Attribute.Name))
			{
				AddIssue(FTryllWorkflowIssue::ESeverity::Error,
					Prefix + TEXT(": name must match [A-Za-z_][A-Za-z0-9_]*."));
			}
			else if (Names.Contains(Attribute.Name))
			{
				AddIssue(FTryllWorkflowIssue::ESeverity::Error,
					Prefix + TEXT(": duplicate name '") + Attribute.Name + TEXT("'."));
			}
			else
			{
				Names.Add(Attribute.Name);
			}
			if (!Attribute.SystemPrompt.IsEmpty()
				&& !Attribute.SystemPrompt.Contains(TEXT("{{intents_block}}")))
			{
				AddIssue(FTryllWorkflowIssue::ESeverity::Error,
					Prefix + TEXT(": system_prompt must reference {{intents_block}}."));
			}
			if (Attribute.Kind == ETryllAttributeKind::Boolean)
			{
				if (Attribute.Choices.Num() > 0)
				{
					AddIssue(FTryllWorkflowIssue::ESeverity::Error,
						Prefix + TEXT(": Boolean cannot define choices."));
				}
				if (Attribute.Statement.IsEmpty())
				{
					AddIssue(FTryllWorkflowIssue::ESeverity::Error,
						Prefix + TEXT(": statement is required for Boolean."));
				}
				if (!Attribute.SystemPrompt.IsEmpty()
					&& !Attribute.SystemPrompt.Contains(TEXT("{{statement}}")))
				{
					AddIssue(FTryllWorkflowIssue::ESeverity::Error,
						Prefix + TEXT(": Boolean system_prompt must reference {{statement}}."));
				}
				bCanTrigger |= Attribute.BooleanTrigger != ETryllBooleanTrigger::Never;
				if (!Attribute.UncertainChoice.IsEmpty()
					|| Attribute.ScoreTrigger != ETryllScoreTrigger::Never
					|| !FMath::IsNearlyEqual(Attribute.ScoreThreshold, 0.5f)
					|| !FMath::IsNearlyEqual(Attribute.Margin, 0.15f))
				{
					AddIssue(FTryllWorkflowIssue::ESeverity::Error,
						Prefix + TEXT(": only statement, threshold, and boolean_trigger apply."));
				}
				continue;
			}
			if (!Attribute.Statement.IsEmpty())
			{
				AddIssue(FTryllWorkflowIssue::ESeverity::Error,
					Prefix + TEXT(": statement is only valid for Boolean."));
			}
			if (Attribute.Choices.Num() < 2 || Attribute.Choices.Num() > 26)
			{
				AddIssue(FTryllWorkflowIssue::ESeverity::Error,
					Prefix + TEXT(": needs 2 to 26 choices."));
			}
			TSet<FString> Ids;
			bool bSink = false;
			for (const FTryllClassifyAttributeChoice& Choice : Attribute.Choices)
			{
				if (Choice.Id.IsEmpty() || Choice.Prompt.IsEmpty())
				{
					AddIssue(FTryllWorkflowIssue::ESeverity::Error,
						Prefix + TEXT(": each choice needs an id and a prompt."));
				}
				else if (Choice.Id == Tryll::kUncertainChoiceId)
				{
					AddIssue(FTryllWorkflowIssue::ESeverity::Error,
						Prefix + TEXT(": choice id '") + Choice.Id + TEXT("' is reserved."));
				}
				else if (Ids.Contains(Choice.Id))
				{
					AddIssue(FTryllWorkflowIssue::ESeverity::Error,
						Prefix + TEXT(": duplicate choice id '") + Choice.Id + TEXT("'."));
				}
				else
				{
					Ids.Add(Choice.Id);
				}
				if (Choice.Id == Attribute.UncertainChoice)
					bSink = true;
				if (Attribute.Kind == ETryllAttributeKind::Score && Choice.bTriggers)
				{
					AddIssue(FTryllWorkflowIssue::ESeverity::Error,
						Prefix + TEXT(": Score choice triggers are not used."));
				}
				if (Attribute.Kind == ETryllAttributeKind::Choice && Choice.bTriggers)
					bCanTrigger = true;
			}
			if (Attribute.Kind == ETryllAttributeKind::Choice
				&& Attribute.UncertainChoice != Tryll::kUncertainChoiceId
				&& !bSink)
			{
				AddIssue(FTryllWorkflowIssue::ESeverity::Error,
					Prefix + TEXT(": uncertain_choice must name one of its choices or '")
					+ Tryll::kUncertainChoiceId + TEXT("'."));
			}
			if (Attribute.Kind == ETryllAttributeKind::Score
				&& Attribute.ScoreTrigger != ETryllScoreTrigger::Never)
			{
				bCanTrigger = true;
			}
			if (Attribute.Kind == ETryllAttributeKind::Score)
			{
				if (!Attribute.UncertainChoice.IsEmpty()
					|| Attribute.BooleanTrigger != ETryllBooleanTrigger::Never
					|| !FMath::IsNearlyEqual(Attribute.Threshold, 0.5f)
					|| !FMath::IsNearlyEqual(Attribute.Margin, 0.15f))
				{
					AddIssue(FTryllWorkflowIssue::ESeverity::Error,
						Prefix + TEXT(": only the ladder, score_trigger, and score_threshold apply."));
				}
			}
			else if (Attribute.BooleanTrigger != ETryllBooleanTrigger::Never
				|| Attribute.ScoreTrigger != ETryllScoreTrigger::Never
				|| !FMath::IsNearlyEqual(Attribute.ScoreThreshold, 0.5f))
			{
				AddIssue(FTryllWorkflowIssue::ESeverity::Error,
					Prefix + TEXT(": boolean_trigger, score_trigger, and score_threshold do not apply."));
			}
		}
		const bool bWired = !Attrs->TriggeredExit.IsEmpty();
		if (bWired != bCanTrigger)
		{
			AddIssue(FTryllWorkflowIssue::ESeverity::Error,
				FString::Printf(
					TEXT("Node '%s': triggered_exit must be set exactly when an attribute can trigger."),
					*Node.Name));
		}
	}

	return Issues;
}

// ── FTryllWorkflowValidator::ValidateStoragePathShape ─────────────────────────

bool FTryllWorkflowValidator::ValidateStoragePathShape(
	const FString& Where, const FString& FieldName, const FString& Value, TArray<FTryllWorkflowIssue>& OutIssues)
{
	if (Value.IsEmpty())
	{
		return true;
	}

	const FString Normalized = UTryllRuntimeSettings::NormalizeStorageRelativePath(Value);
	if (!Normalized.Equals(Value, ESearchCase::CaseSensitive))
	{
		OutIssues.Add({ FTryllWorkflowIssue::ESeverity::Warning,
			FString::Printf(TEXT("%s field '%s' path '%s' should be relative to storage_root (e.g. '%s')."),
				*Where, *FieldName, *Value, *Normalized) });
	}

	if (!FPaths::IsRelative(Normalized) || Normalized.StartsWith(TEXT("/")) || Normalized.StartsWith(TEXT("\\")))
	{
		OutIssues.Add({ FTryllWorkflowIssue::ESeverity::Error,
			FString::Printf(TEXT("%s field '%s' has an absolute path '%s'; must be relative to storage_root."),
				*Where, *FieldName, *Value) });
		return false;
	}

	if (Normalized.Contains(TEXT("..")))
	{
		OutIssues.Add({ FTryllWorkflowIssue::ESeverity::Error,
			FString::Printf(TEXT("%s field '%s' path '%s' contains '..'; directory traversal is not allowed."),
				*Where, *FieldName, *Normalized) });
		return false;
	}
	return true;
}

// ── FTryllWorkflowValidator::ValidateAndLog ───────────────────────────────────

bool FTryllWorkflowValidator::ValidateAndLog(const FTryllGraphDescription& Graph, const FString& ContextName)
{
	TArray<FTryllWorkflowIssue> Issues = Validate(Graph);

	if (Issues.IsEmpty())
	{
		UE_LOG(LogTryllWorkflowValidator, Display,
		       TEXT("[Tryll] Workflow '%s': OK — no issues found."), *ContextName);
		return true;
	}

	bool bHasErrors = false;
	for (const FTryllWorkflowIssue& Issue : Issues)
	{
		if (Issue.Severity == FTryllWorkflowIssue::ESeverity::Error)
		{
			UE_LOG(LogTryllWorkflowValidator, Error,
			       TEXT("[Tryll] Workflow '%s': %s"), *ContextName, *Issue.Message);
			bHasErrors = true;
		}
		else
		{
			UE_LOG(LogTryllWorkflowValidator, Warning,
			       TEXT("[Tryll] Workflow '%s': %s"), *ContextName, *Issue.Message);
		}
	}
	return !bHasErrors;
}
