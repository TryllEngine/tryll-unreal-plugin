// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllDialogLabExport.h"

#include "TryllDialogCheckJson.h"
#include "TryllDialogLabRunData.h"
#include "TryllDialogLabStatistics.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	FString Quoted(const FString& Value)
	{
		return TEXT("\"") + FTryllDialogLabExport::EscapeJson(Value) + TEXT("\"");
	}

	/** Returned as a raw pointer so it can feed %s directly, like the other literals here. */
	const TCHAR* Boolean(bool bValue)
	{
		return bValue ? TEXT("true") : TEXT("false");
	}

	/**
	 * .NET's round-trip "o" format, which is what Unity writes: seven fractional digits and an
	 * explicit offset. FDateTime::ToIso8601 emits milliseconds and a 'Z', so using it would make
	 * two otherwise identical exports differ byte-for-byte — and §19 is an exact-equality format.
	 */
	FString RoundTripUtc(const FDateTime& Value)
	{
		// A tick is 100 ns, so the sub-second remainder is exactly the seven digits "o" wants.
		const int64 SubSecondTicks = Value.GetTicks() % ETimespan::TicksPerSecond;
		return FString::Printf(
			TEXT("%04d-%02d-%02dT%02d:%02d:%02d.%07lld+00:00"),
			Value.GetYear(), Value.GetMonth(), Value.GetDay(),
			Value.GetHour(), Value.GetMinute(), Value.GetSecond(), SubSecondTicks);
	}

	FString ArrayOfStrings(const TArray<FString>& Values)
	{
		TArray<FString> Parts;
		for (const FString& Value : Values)
		{
			Parts.Add(Quoted(Value));
		}
		return TEXT("[") + FString::Join(Parts, TEXT(",")) + TEXT("]");
	}

	const TCHAR* StatusName(ETryllLabCellStatus Status)
	{
		switch (Status)
		{
			case ETryllLabCellStatus::Ok:        return TEXT("Ok");
			case ETryllLabCellStatus::CheckFail: return TEXT("CheckFail");
			case ETryllLabCellStatus::Error:     return TEXT("Error");
			default:                             return TEXT("Skipped");
		}
	}

	/** Run status is lower-cased; per-cell status keeps its original casing. Do not tidy either. */
	const TCHAR* RunStatusName(ETryllDialogLabRunStatus Status)
	{
		switch (Status)
		{
			case ETryllDialogLabRunStatus::Stopped: return TEXT("stopped");
			case ETryllDialogLabRunStatus::Failed:  return TEXT("failed");
			default:                                return TEXT("completed");
		}
	}

	FString ScriptJson(const FTryllLabScriptSnapshot& Script)
	{
		TArray<FString> Steps;
		for (const FTryllLabStepSnapshot& Step : Script.Steps)
		{
			TArray<FString> Tools;
			for (const FTryllCannedToolResult& Canned : Step.ToolResults)
			{
				Tools.Add(FString::Printf(TEXT("{\"tool_name\":%s,\"result\":%s}"),
					*Quoted(Canned.ToolName), *Quoted(Canned.Result)));
			}
			Steps.Add(FString::Printf(
				TEXT("{\"user\":%s,\"assistant\":%s,\"checks\":%s,\"tool_results\":[%s]}"),
				*Quoted(Step.User), *Quoted(Step.Assistant),
				*FTryllDialogCheckJson::WriteList(Step.Checks),
				*FString::Join(Tools, TEXT(","))));
		}
		return FString::Printf(
			TEXT("{\"name\":%s,\"description\":%s,\"seed_turn_count\":%d,")
			TEXT("\"global_checks\":%s,\"steps\":[%s]}"),
			*Quoted(Script.Name), *Quoted(Script.Description), Script.SeedTurnCount,
			*FTryllDialogCheckJson::WriteList(Script.GlobalChecks),
			*FString::Join(Steps, TEXT(",")));
	}

	FString DialoguesJson(const TArray<FTryllLabScriptSnapshot>& Dialogues)
	{
		TArray<FString> Items;
		for (const FTryllLabScriptSnapshot& Script : Dialogues)
		{
			Items.Add(ScriptJson(Script));
		}
		return FString::Join(Items, TEXT(","));
	}

	FString VariantsJson(const TArray<FTryllLabVariantSnapshot>& Variants)
	{
		TArray<FString> Items;
		for (const FTryllLabVariantSnapshot& Variant : Variants)
		{
			TArray<FString> Nodes;
			for (const FTryllLabNodeSummary& Node : Variant.Nodes)
			{
				Nodes.Add(FString::Printf(TEXT("{\"name\":%s,\"type\":%s}"),
					*Quoted(Node.Name), *Quoted(Node.Type)));
			}
			TArray<FString> Overrides;
			for (const FTryllLabParamOverride& Override : Variant.Overrides)
			{
				Overrides.Add(FString::Printf(
					TEXT("{\"node\":%s,\"field\":%s,\"value\":%s}"),
					*Quoted(Override.NodeName), *Quoted(Override.FieldPath), *Quoted(Override.Value)));
			}

			Items.Add(FString::Printf(
				TEXT("{\"index\":%d,\"label\":%s,\"agent\":%s,\"agent_is_override\":%s,")
				TEXT("\"workflow_asset\":%s,\"graph_start_node\":%s,\"default_model_name\":%s,")
				TEXT("\"model_names\":%s,\"maintain_dialogue_history\":%s,\"use_live_tools\":%s,")
				TEXT("\"model_override\":%s,\"nodes\":[%s],\"overrides\":[%s]}"),
				Variant.Index, *Quoted(Variant.Label), *Quoted(Variant.AgentName),
				Boolean(Variant.bAgentIsOverride), *Quoted(Variant.WorkflowAssetName),
				*Quoted(Variant.GraphStartNode), *Quoted(Variant.DefaultModelName),
				*ArrayOfStrings(Variant.ModelNames), Boolean(Variant.bMaintainDialogueHistory),
				Boolean(Variant.bUseLiveTools),
				*Quoted(Variant.ModelOverride),
				*FString::Join(Nodes, TEXT(",")), *FString::Join(Overrides, TEXT(","))));
		}
		return TEXT("[") + FString::Join(Items, TEXT(",")) + TEXT("]");
	}

	/** Embedded raw when it parses, escaped as a string when it does not, literal null when absent. */
	FString DebugInfoJson(const FString& Raw)
	{
		if (Raw.IsEmpty())
		{
			return TEXT("null");
		}
		TSharedPtr<FJsonValue> Parsed;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Raw);
		if (FJsonSerializer::Deserialize(Reader, Parsed) && Parsed.IsValid())
		{
			return Raw;
		}
		return Quoted(Raw);
	}
}

FString FTryllDialogLabExport::EscapeJson(const FString& Value)
{
	FString Out;
	Out.Reserve(Value.Len() + 2);
	for (const TCHAR Ch : Value)
	{
		switch (Ch)
		{
			case TEXT('\"'): Out += TEXT("\\\""); break;
			case TEXT('\\'): Out += TEXT("\\\\"); break;
			case TEXT('\n'): Out += TEXT("\\n");  break;
			case TEXT('\r'): Out += TEXT("\\r");  break;
			case TEXT('\t'): Out += TEXT("\\t");  break;
			default:
				if (Ch < 0x20)
				{
					Out += FString::Printf(TEXT("\\u%04x"), static_cast<int32>(Ch));
				}
				else
				{
					Out.AppendChar(Ch);
				}
				break;
		}
	}
	return Out;
}

FString FTryllDialogLabExport::ExportToJsonl(const FTryllDialogLabRunResult& Run)
{
	FString Out;

	Out += FString::Printf(
		TEXT("{\"format_version\":%d,\"kind\":\"dialog_lab_run\",\"run_id\":%s,")
		TEXT("\"started_at\":%s,\"completed_at\":%s,\"status\":\"%s\",")
		TEXT("\"default_agent_name\":%s,\"settings_asset_name\":%s,")
		TEXT("\"repeats\":%d,\"base_seed\":%d,\"dialogues\":[%s],\"variants\":%s,\"stats\":%s,")
		TEXT("\"server\":{\"exe_path\":%s,\"build_config\":%s,\"version\":%s,\"run_id\":%s,")
		TEXT("\"managed\":%s},\"issues\":%s}\n"),
		FormatVersion,
		*Quoted(Run.Snapshot.RunId),
		*Quoted(RoundTripUtc(Run.StartedAtUtc)),
		*Quoted(RoundTripUtc(Run.CompletedAtUtc)),
		RunStatusName(Run.Status),
		*Quoted(Run.Snapshot.DefaultAgentName),
		*Quoted(Run.Snapshot.SettingsAssetName),
		Run.Snapshot.Repeats, Run.Snapshot.BaseSeed,
		*DialoguesJson(Run.Snapshot.Dialogues),
		*VariantsJson(Run.Snapshot.Variants),
		// Derived, for readers of the file; import recomputes rather than trusting it.
		*FTryllDialogLabStatistics::Compute(Run).ToJson(),
		*Quoted(Run.Server.ExePath), *Quoted(Run.Server.BuildConfig),
		*Quoted(Run.Server.Version), *Quoted(Run.Server.RunId),
		Boolean(Run.Server.bManaged),
		*ArrayOfStrings(Run.Issues));

	for (const FTryllLabSampleResult& Sample : Run.Samples)
	{
		for (const FTryllLabTurnRecord& Turn : Sample.Turns)
		{
			TArray<FString> Labels;
			for (const FTryllLabCheckOutcome& Failure : Turn.FailedChecks())
			{
				Labels.Add(Failure.Label);
			}

			FString Line = FString::Printf(
				TEXT("{\"sample_id\":%s,\"variant_index\":%d,\"variant_label\":%s,\"repeat\":%d,")
				TEXT("\"seed\":%d,\"dialogue_index\":%d,\"turn_index\":%d,\"seeded\":%s,\"user_text\":%s,")
				TEXT("\"answer_text\":%s,\"status\":\"%s\",\"failed_checks\":%s,")
				TEXT("\"check_outcomes\":%s"),
				*Quoted(Sample.SampleId), Sample.VariantIndex, *Quoted(Sample.VariantLabel),
				Sample.Repeat, Sample.Seed, Turn.DialogueIndex, Turn.TurnIndex, Boolean(Turn.bSeeded),
				*Quoted(Turn.UserText), *Quoted(Turn.AnswerText), StatusName(Turn.Status),
				*ArrayOfStrings(Labels), *FTryllDialogCheckJson::WriteOutcomes(Turn.CheckOutcomes));

			if (!Turn.ErrorMessage.IsEmpty())
			{
				Line += FString::Printf(TEXT(",\"error\":%s"), *Quoted(Turn.ErrorMessage));
			}

			Line += FString::Printf(
				TEXT(",\"duration_ms\":%d,\"ttft_ms\":%d,\"tokens_generated\":%d,\"debug_info\":%s}\n"),
				Turn.DurationMs, Turn.TtftMs, Turn.TokensGenerated,
				*DebugInfoJson(Turn.DebugInfoRaw));

			Out += Line;
		}
	}

	return Out;
}
