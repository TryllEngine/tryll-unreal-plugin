// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllDialogLabImport.h"

#include "UI/TryllEditorStyle.h"

#include "TryllDialogCheckJson.h"
#include "TryllDialogLabChecks.h"
#include "TryllDialogLabExport.h"
#include "TryllDialogLabRunData.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	TArray<FString> ReadStrings(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		TArray<FString> Out;
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (Object.IsValid() && Object->TryGetArrayField(Field, Values) && Values != nullptr)
		{
			for (const TSharedPtr<FJsonValue>& Value : *Values)
			{
				FString Text;
				if (Value.IsValid() && Value->TryGetString(Text))
				{
					Out.Add(Text);
				}
			}
		}
		return Out;
	}

	int32 ReadInt(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, int32 Fallback = 0)
	{
		double Value = 0.0;
		return (Object.IsValid() && Object->TryGetNumberField(Field, Value))
			? static_cast<int32>(Value) : Fallback;
	}

	bool ReadBool(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, bool bFallback)
	{
		bool bValue = bFallback;
		return (Object.IsValid() && Object->TryGetBoolField(Field, bValue)) ? bValue : bFallback;
	}

	FString ReadString(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		FString Value;
		if (Object.IsValid())
		{
			Object->TryGetStringField(Field, Value);
		}
		return Value;
	}

	ETryllLabCellStatus ParseCellStatus(const FString& Name)
	{
		if (Name == TEXT("CheckFail")) { return ETryllLabCellStatus::CheckFail; }
		if (Name == TEXT("Error"))     { return ETryllLabCellStatus::Error; }
		if (Name == TEXT("Skipped"))   { return ETryllLabCellStatus::Skipped; }
		return ETryllLabCellStatus::Ok;
	}

	ETryllDialogLabRunStatus ParseRunStatus(const FString& Name)
	{
		const FString Lower = Name.ToLower();
		if (Lower == TEXT("stopped")) { return ETryllDialogLabRunStatus::Stopped; }
		if (Lower == TEXT("failed"))  { return ETryllDialogLabRunStatus::Failed; }
		return ETryllDialogLabRunStatus::Completed;
	}

	FDateTime ParseTime(const FString& Text)
	{
		FDateTime Out;
		if (!FDateTime::ParseIso8601(*Text, Out))
		{
			return FDateTime::UtcNow();
		}
		// ParseIso8601 keeps milliseconds only, but Unity writes the round-trip "o" format with
		// seven fractional digits (100 ns ticks, FDateTime's own resolution). Re-read the fraction
		// so a Unity export re-exports byte for byte.
		int32 Dot = INDEX_NONE;
		if (Text.FindChar(TEXT('.'), Dot))
		{
			int64 FractionTicks = 0;
			int32 Digits = 0;
			for (int32 I = Dot + 1; I < Text.Len() && FChar::IsDigit(Text[I]); ++I, ++Digits)
			{
				if (Digits < 7)
				{
					FractionTicks = FractionTicks * 10 + (Text[I] - TEXT('0'));
				}
			}
			for (int32 Pad = Digits; Pad < 7; ++Pad)
			{
				FractionTicks *= 10;
			}
			const int64 WholeSecondTicks = Out.GetTicks() - Out.GetTicks() % ETimespan::TicksPerSecond;
			Out = FDateTime(WholeSecondTicks + FractionTicks);
		}
		return Out;
	}

	TSharedPtr<FJsonObject> ParseLine(const FString& Line, FString& OutWhy)
	{
		TSharedPtr<FJsonObject> Object;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Line);
		if (!FJsonSerializer::Deserialize(Reader, Object) || !Object.IsValid())
		{
			OutWhy = Reader->GetErrorMessage().IsEmpty()
				? FString(TEXT("expected object")) : Reader->GetErrorMessage();
			return nullptr;
		}
		return Object;
	}

	/**
	 * The debug_info object exactly as the file has it. Both exporters write it last on the line,
	 * embedded raw from the server, so keeping the original text (rather than re-serialising
	 * the parsed object, which rewrites float digits) lets a file from either engine re-export
	 * byte for byte. Empty when the line does not have that shape.
	 */
	FString RawDebugInfo(const FString& Line)
	{
		static const FString Key = TEXT(",\"debug_info\":{");
		const int32 Tokens = Line.Find(TEXT("\"tokens_generated\":"), ESearchCase::CaseSensitive);
		const int32 At = Tokens == INDEX_NONE ? INDEX_NONE
			: Line.Find(Key, ESearchCase::CaseSensitive, ESearchDir::FromStart, Tokens);
		FString Trimmed = Line;
		Trimmed.TrimEndInline();
		if (At == INDEX_NONE || !Trimmed.EndsWith(TEXT("}")))
		{
			return FString();
		}
		const int32 Start = At + Key.Len() - 1;
		const FString Raw = Trimmed.Mid(Start, Trimmed.Len() - 1 - Start);
		TSharedPtr<FJsonObject> Check;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Raw);
		return FJsonSerializer::Deserialize(Reader, Check) && Check.IsValid() ? Raw : FString();
	}

	/** Re-serialised compactly because both the inspector and the raw view read it. */
	FString CompactJson(const TSharedPtr<FJsonObject>& Object)
	{
		if (!Object.IsValid())
		{
			return FString();
		}
		FString Out;
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out);
		FJsonSerializer::Serialize(Object.ToSharedRef(), Writer);
		return Out;
	}

	TArray<FTryllCannedToolResult> ParseToolResults(const TSharedPtr<FJsonObject>& StepObject)
	{
		TArray<FTryllCannedToolResult> Out;
		const TArray<TSharedPtr<FJsonValue>>* Tools = nullptr;
		if (StepObject->TryGetArrayField(TEXT("tool_results"), Tools) && Tools != nullptr)
		{
			for (const TSharedPtr<FJsonValue>& ToolEntry : *Tools)
			{
				const TSharedPtr<FJsonObject>* ToolObject = nullptr;
				if (!ToolEntry.IsValid() || !ToolEntry->TryGetObject(ToolObject) || ToolObject == nullptr)
				{
					continue;
				}
				FTryllCannedToolResult Canned;
				Canned.ToolName = ReadString(*ToolObject, TEXT("tool_name"));
				Canned.Result   = ReadString(*ToolObject, TEXT("result"));
				Out.Add(MoveTemp(Canned));
			}
		}
		return Out;
	}

	TArray<TSharedPtr<FJsonValue>> ArrayField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		return Object->TryGetArrayField(Field, Values) && Values != nullptr
			? *Values : TArray<TSharedPtr<FJsonValue>>();
	}

	bool ParseScriptV4(const TSharedPtr<FJsonObject>& ScriptObject, FTryllLabScriptSnapshot& Script, FString& OutError)
	{
		Script.Name          = ReadString(ScriptObject, TEXT("name"));
		Script.Description   = ReadString(ScriptObject, TEXT("description"));
		Script.SeedTurnCount = ReadInt(ScriptObject, TEXT("seed_turn_count"), 0);
		FString CheckError;
		if (!FTryllDialogCheckJson::TryReadList(ArrayField(ScriptObject, TEXT("global_checks")), Script.GlobalChecks, CheckError))
		{
			OutError = FString::Printf(TEXT("Line 1: global_checks: %s."), *CheckError);
			return false;
		}
		const TArray<TSharedPtr<FJsonValue>> StepValues = ArrayField(ScriptObject, TEXT("steps"));
		for (int32 Index = 0; Index < StepValues.Num(); ++Index)
		{
			const TSharedPtr<FJsonObject>* StepObject = nullptr;
			if (!StepValues[Index].IsValid() || !StepValues[Index]->TryGetObject(StepObject) || StepObject == nullptr)
			{
				continue;
			}
			FTryllLabStepSnapshot Step;
			Step.User        = ReadString(*StepObject, TEXT("user"));
			Step.Assistant   = ReadString(*StepObject, TEXT("assistant"));
			Step.ToolResults = ParseToolResults(*StepObject);
			if (!FTryllDialogCheckJson::TryReadList(ArrayField(*StepObject, TEXT("checks")), Step.Checks, CheckError))
			{
				OutError = FString::Printf(TEXT("Line 1: step %d checks: %s."), Index + 1, *CheckError);
				return false;
			}
			Script.Steps.Add(MoveTemp(Step));
		}
		return true;
	}

	/**
	 * A format-3 script: its fixed check fields go through the same conversion as an old asset
	 * on load (FTryllLabCheckSpec::FromLegacyStep), so the imported checks are the ones that
	 * asset would have today.
	 */
	FTryllLabScriptSnapshot ParseScriptV3(const TSharedPtr<FJsonObject>& ScriptObject)
	{
		FTryllLabScriptSnapshot Script;
		Script.Name          = ReadString(ScriptObject, TEXT("name"));
		Script.Description   = ReadString(ScriptObject, TEXT("description"));
		Script.SeedTurnCount = ReadInt(ScriptObject, TEXT("seed_turn_count"), 0);
		// Absent in files written before script-level patterns existed; empty is what those runs
		// actually evaluated with.
		Script.GlobalChecks  = FTryllLabCheckSpec::FromLegacyGlobal(ReadStrings(ScriptObject, TEXT("global_regex_forbid")));

		const TArray<TSharedPtr<FJsonValue>> StepValues = ArrayField(ScriptObject, TEXT("steps"));
		for (int32 Index = 0; Index < StepValues.Num(); ++Index)
		{
			const TSharedPtr<FJsonObject>* StepObject = nullptr;
			if (!StepValues[Index].IsValid() || !StepValues[Index]->TryGetObject(StepObject) || StepObject == nullptr)
			{
				continue;
			}
			FTryllDialogScriptStep Legacy;
			Legacy.MustContain    = ReadStrings(*StepObject, TEXT("must_contain"));
			Legacy.MustNotContain = ReadStrings(*StepObject, TEXT("must_not_contain"));
			Legacy.RegexForbid    = ReadStrings(*StepObject, TEXT("regex_forbid"));
			Legacy.MaxWords       = ReadInt(*StepObject, TEXT("max_words"), 0);
			for (const TSharedPtr<FJsonValue>& Entry : ArrayField(*StepObject, TEXT("intent_checks")))
			{
				const TSharedPtr<FJsonObject>* CheckObject = nullptr;
				if (Entry.IsValid() && Entry->TryGetObject(CheckObject) && CheckObject != nullptr)
				{
					FTryllIntentCheck Check;
					Check.Node            = ReadString(*CheckObject, TEXT("node"));
					Check.Intent          = ReadString(*CheckObject, TEXT("intent"));
					Check.bExpectNotFound = ReadBool(*CheckObject, TEXT("expect_not_found"), false);
					Legacy.IntentChecks.Add(MoveTemp(Check));
				}
			}
			for (const TSharedPtr<FJsonValue>& Entry : ArrayField(*StepObject, TEXT("attribute_checks")))
			{
				const TSharedPtr<FJsonObject>* CheckObject = nullptr;
				if (Entry.IsValid() && Entry->TryGetObject(CheckObject) && CheckObject != nullptr)
				{
					FTryllAttributeCheck Check;
					Check.Node        = ReadString(*CheckObject, TEXT("node"));
					Check.Name        = ReadString(*CheckObject, TEXT("name"));
					Check.Value       = ReadString(*CheckObject, TEXT("value"));
					Check.bScoreRange = ReadBool(*CheckObject, TEXT("score_range"), false);
					double ScoreMin = 0.0;
					double ScoreMax = 0.0;
					(*CheckObject)->TryGetNumberField(TEXT("score_min"), ScoreMin);
					(*CheckObject)->TryGetNumberField(TEXT("score_max"), ScoreMax);
					Check.ScoreMin = static_cast<float>(ScoreMin);
					Check.ScoreMax = static_cast<float>(ScoreMax);
					Legacy.AttributeChecks.Add(MoveTemp(Check));
				}
			}
			Legacy.ToolResults = ParseToolResults(*StepObject);

			FTryllLabStepSnapshot Step;
			Step.User        = ReadString(*StepObject, TEXT("user"));
			Step.Assistant   = ReadString(*StepObject, TEXT("assistant"));
			Step.ToolResults = Legacy.ToolResults;
			Step.Checks      = FTryllLabCheckSpec::FromLegacyStep(Legacy, Index >= Script.SeedTurnCount);
			Script.Steps.Add(MoveTemp(Step));
		}
		return Script;
	}

	TArray<FTryllLabCheckOutcome> ParseV3Failures(const TSharedPtr<FJsonObject>& Turn)
	{
		TArray<FTryllLabCheckOutcome> Out;
		const TArray<TSharedPtr<FJsonValue>> Details = ArrayField(Turn, TEXT("failed_check_details"));
		if (Details.Num() > 0)
		{
			for (const TSharedPtr<FJsonValue>& Entry : Details)
			{
				const TSharedPtr<FJsonObject>* DetailObject = nullptr;
				if (!Entry.IsValid() || !Entry->TryGetObject(DetailObject) || DetailObject == nullptr)
				{
					continue;
				}
				FTryllLabCheckOutcome Outcome;
				Outcome.Kind   = ReadString(*DetailObject, TEXT("kind"));
				Outcome.Arg    = ReadString(*DetailObject, TEXT("arg"));
				Outcome.Label  = ReadString(*DetailObject, TEXT("label"));
				Outcome.Actual = ReadString(*DetailObject, TEXT("actual"));
				Out.Add(MoveTemp(Outcome));
			}
			return Out;
		}
		// Labels-only degrades to outcomes with empty kind and arg rather than failing.
		for (const FString& Label : ReadStrings(Turn, TEXT("failed_checks")))
		{
			FTryllLabCheckOutcome Outcome;
			Outcome.Label = Label;
			Out.Add(MoveTemp(Outcome));
		}
		return Out;
	}

	/** A max_words failure's arg is the bare limit; the converted word count reports "0|limit". */
	FString V3ArgFor(const FTryllLabCheckOutcome& Failure)
	{
		return Failure.Kind == FTryllDialogLabChecks::KindMaxWords && Failure.Arg.IsNumeric()
			? FTryllDialogLabChecks::WordCountArg(0, FCString::Atoi(*Failure.Arg))
			: Failure.Arg;
	}

	/**
	 * The format-3 kind folds: a malformed pattern failed as invalid_regex, an absent classifier
	 * as *_unavailable, a word limit as max_words, and both "must not call" forms of the tool
	 * assertion are now the gate.
	 */
	bool V3FailureMatches(const FTryllLabCheckOutcome& Failure, const FTryllLabCheck& Row)
	{
		if (Row.Scope == ETryllLabCheckScope::Gate)
		{
			return Failure.Kind == FTryllDialogLabChecks::KindToolCall
				&& (Failure.Arg.IsEmpty() || Failure.Arg == FTryllDialogLabChecks::ToolCallOtherArg);
		}
		if (!V3ArgFor(Failure).Equals(Row.Arg, ESearchCase::CaseSensitive))
		{
			return false;
		}
		const FString& Kind = Failure.Kind;
		if (Kind == FTryllDialogLabChecks::KindInvalidRegex)
		{
			return Row.Kind == FTryllDialogLabChecks::KindRegexForbid;
		}
		if (Kind == FTryllDialogLabChecks::KindMaxWords)
		{
			return Row.Kind == FTryllDialogLabChecks::KindWordCount;
		}
		if (Kind == FTryllDialogLabChecks::KindIntentUnavailable)
		{
			return Row.Kind == FTryllDialogLabChecks::KindIntentIs || Row.Kind == FTryllDialogLabChecks::KindIntentNotFound;
		}
		if (Kind == FTryllDialogLabChecks::KindAttributeUnavailable)
		{
			return Row.Kind == FTryllDialogLabChecks::KindAttributeIs || Row.Kind == FTryllDialogLabChecks::KindAttributeScore;
		}
		if (Kind == FTryllDialogLabChecks::KindToolCall)
		{
			return Row.Kind == FTryllDialogLabChecks::KindToolCall && !Failure.Arg.IsEmpty();
		}
		return Kind == Row.Kind;
	}

	FString CheckTypeAt(const FTryllLabScriptSnapshot& Script, const FTryllLabStepSnapshot& Step, const FTryllLabCheck& Row)
	{
		switch (Row.Scope)
		{
			case ETryllLabCheckScope::Gate:   return FTryllDialogLabChecks::GateUnexpectedToolCall;
			case ETryllLabCheckScope::Global: return Script.GlobalChecks[Row.Index].Type;
			default:                          return Step.Checks[Row.Index].Type;
		}
	}
}

TArray<FTryllLabCheckOutcome> FTryllDialogLabImport::MapV3Outcomes(
	const FTryllLabScriptSnapshot& Script, int32 TurnIndex, const FTryllLabTurnRecord& Record,
	const TArray<FTryllLabCheckOutcome>& Failures)
{
	const FTryllLabStepSnapshot& Step = Script.Steps[TurnIndex];
	const bool bEvaluated = Record.Status == ETryllLabCellStatus::Ok || Record.Status == ETryllLabCellStatus::CheckFail;
	TArray<FTryllLabCheckOutcome> Remaining = Failures;
	TArray<FTryllLabCheckOutcome> Outcomes;
	for (const FTryllLabCheck& Row : FTryllDialogLabChecks::AuthoredChecks(Script, Step, Record.bSeeded))
	{
		const FString CheckType = CheckTypeAt(Script, Step, Row);
		const int32 Found = Remaining.IndexOfByPredicate([&Row](const FTryllLabCheckOutcome& F)
		{
			return V3FailureMatches(F, Row);
		});
		if (Found != INDEX_NONE)
		{
			FTryllLabCheckOutcome Failure = Remaining[Found];
			Remaining.RemoveAt(Found);
			Failure.Scope     = Row.Scope;
			Failure.Index     = Row.Index;
			Failure.CheckType = CheckType;
			Failure.bPassed   = false;
			Outcomes.Add(MoveTemp(Failure));
		}
		else if (bEvaluated)
		{
			FTryllLabCheckOutcome Pass;
			Pass.Scope     = Row.Scope;
			Pass.Index     = Row.Index;
			Pass.CheckType = CheckType;
			Pass.Kind      = Row.Kind;
			Pass.Arg       = Row.Arg;
			Pass.bPassed   = true;
			Pass.Label     = Row.Label;
			Outcomes.Add(MoveTemp(Pass));
		}
	}
	for (FTryllLabCheckOutcome& Orphan : Remaining)
	{
		Orphan.Scope   = ETryllLabCheckScope::Step;
		Orphan.Index   = INDEX_NONE;
		Orphan.bPassed = false;
		Outcomes.Add(MoveTemp(Orphan));
	}
	return Outcomes;
}

FLinearColor FTryllDialogLabImport::PaletteColor(int32 Index)
{
	// Authored as sRGB, like every other colour in the Lab: handed to Slate raw they would be
	// treated as linear and render far brighter than the swatches a live run produces.
	return FTryllEditorStyle::VariantSwatch(Index);
}

bool FTryllDialogLabImport::TryLoad(
	const FString& Text, FTryllDialogLabRunResult& OutRun, FString& OutError)
{
	OutError.Reset();
	OutRun = FTryllDialogLabRunResult();

	if (Text.TrimStartAndEnd().IsEmpty())
	{
		OutError = TEXT("File is empty.");
		return false;
	}

	FString Normalised = Text;
	Normalised.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
	Normalised.ReplaceInline(TEXT("\r"), TEXT("\n"));

	TArray<FString> Lines;
	Normalised.ParseIntoArray(Lines, TEXT("\n"), /*CullEmpty*/ true);
	if (Lines.Num() < 1)
	{
		OutError = TEXT("File has no JSONL lines.");
		return false;
	}

	FString Why;
	const TSharedPtr<FJsonObject> Header = ParseLine(Lines[0], Why);
	if (!Header.IsValid())
	{
		OutError = FString::Printf(TEXT("Line 1: invalid header JSON (%s)."), *Why);
		return false;
	}

	const int32 Format = ReadInt(Header, TEXT("format_version"), 0);
	if (Format != FTryllDialogLabExport::FormatVersion && Format != LegacyFormatVersion)
	{
		OutError = Format < LegacyFormatVersion
			? FString::Printf(
				TEXT("Unsupported format_version %d. Re-export from a current Dialog Lab (format %d)."),
				Format, FTryllDialogLabExport::FormatVersion)
			: FString::Printf(TEXT("Unsupported format_version %d (this Lab reads formats %d and %d)."),
				Format, LegacyFormatVersion, FTryllDialogLabExport::FormatVersion);
		return false;
	}
	const bool bLegacy = Format == LegacyFormatVersion;

	if (ReadString(Header, TEXT("kind")) != TEXT("dialog_lab_run"))
	{
		OutError = TEXT("Line 1: kind must be \"dialog_lab_run\".");
		return false;
	}

	TArray<FTryllLabScriptSnapshot> Dialogues;
	if (bLegacy)
	{
		const TSharedPtr<FJsonObject>* ScriptObject = nullptr;
		if (!Header->TryGetObjectField(TEXT("script"), ScriptObject) || ScriptObject == nullptr)
		{
			OutError = TEXT("Line 1: missing script object.");
			return false;
		}
		Dialogues.Add(ParseScriptV3(*ScriptObject));
	}
	else
	{
		const TArray<TSharedPtr<FJsonValue>>* DialogueValues = nullptr;
		if (!Header->TryGetArrayField(TEXT("dialogues"), DialogueValues) || DialogueValues == nullptr
			|| DialogueValues->Num() == 0)
		{
			OutError = TEXT("Line 1: dialogues array is empty.");
			return false;
		}
		for (const TSharedPtr<FJsonValue>& Value : *DialogueValues)
		{
			FTryllLabScriptSnapshot Script;
			const TSharedPtr<FJsonObject>* ScriptObject = nullptr;
			if (!Value.IsValid() || !Value->TryGetObject(ScriptObject) || ScriptObject == nullptr
				|| !ParseScriptV4(*ScriptObject, Script, OutError))
			{
				if (OutError.IsEmpty())
				{
					OutError = TEXT("Line 1: dialogue is not an object.");
				}
				if (DialogueValues->Num() > 1)
				{
					OutError = OutError.Replace(TEXT("Line 1: "),
						*FString::Printf(TEXT("Line 1: dialogue %d: "), Dialogues.Num() + 1));
				}
				return false;
			}
			Dialogues.Add(MoveTemp(Script));
		}
	}

	for (int32 D = 0; D < Dialogues.Num(); ++D)
	{
		const FTryllLabScriptSnapshot& Script = Dialogues[D];
		const FString Where = Dialogues.Num() > 1
			? FString::Printf(TEXT("Line 1: dialogue %d: "), D + 1)
			: FString(TEXT("Line 1: "));
		if (Script.Steps.Num() == 0)
		{
			OutError = Where + TEXT("script.steps is empty.");
			return false;
		}
		if (Script.SeedTurnCount < 0 || Script.SeedTurnCount > Script.Steps.Num())
		{
			OutError = Where + TEXT("invalid script.seed_turn_count.");
			return false;
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* VariantValues = nullptr;
	if (!Header->TryGetArrayField(TEXT("variants"), VariantValues) || VariantValues == nullptr
		|| VariantValues->Num() == 0)
	{
		OutError = TEXT("Line 1: variants array is empty.");
		return false;
	}

	FTryllDialogLabRunSnapshot Snapshot;
	Snapshot.RunId             = ReadString(Header, TEXT("run_id"));
	Snapshot.CapturedAtUtc     = ParseTime(ReadString(Header, TEXT("started_at")));
	Snapshot.Dialogues         = Dialogues;
	Snapshot.Repeats           = FMath::Max(1, ReadInt(Header, TEXT("repeats"), 1));
	Snapshot.BaseSeed          = FMath::Max(1, ReadInt(Header, TEXT("base_seed"), 1));
	Snapshot.DefaultAgentName  = ReadString(Header, TEXT("default_agent_name"));
	Snapshot.SettingsAssetName = ReadString(Header, TEXT("settings_asset_name"));

	for (int32 Index = 0; Index < VariantValues->Num(); ++Index)
	{
		const TSharedPtr<FJsonObject>* VariantObject = nullptr;
		if (!(*VariantValues)[Index].IsValid()
			|| !(*VariantValues)[Index]->TryGetObject(VariantObject) || VariantObject == nullptr)
		{
			continue;
		}

		FTryllLabVariantSnapshot Variant;
		Variant.Index             = ReadInt(*VariantObject, TEXT("index"), Index);
		// Only an *absent* key falls back: a blank label is authorable, and Unity keeps it blank.
		if (!(*VariantObject)->TryGetStringField(TEXT("label"), Variant.Label))
		{
			Variant.Label = FString::Printf(TEXT("Variant %d"), Index + 1);
		}
		Variant.Swatch            = PaletteColor(Index);
		Variant.AgentName         = ReadString(*VariantObject, TEXT("agent"));
		Variant.bAgentIsOverride  = ReadBool(*VariantObject, TEXT("agent_is_override"), false);
		Variant.WorkflowAssetName = ReadString(*VariantObject, TEXT("workflow_asset"));
		Variant.GraphStartNode    = ReadString(*VariantObject, TEXT("graph_start_node"));
		Variant.DefaultModelName  = ReadString(*VariantObject, TEXT("default_model_name"));
		Variant.ModelNames        = ReadStrings(*VariantObject, TEXT("model_names"));
		// Absent in files written before the Lab honoured the agent's setting, and true is
		// exactly what those runs used — so the default is the correct historical value.
		Variant.bMaintainDialogueHistory =
			ReadBool(*VariantObject, TEXT("maintain_dialogue_history"), true);
		// Absent in files written before the live-tools opt-in, and false is exactly what
		// those runs used (uncanned call failed the cell).
		Variant.bUseLiveTools =
			ReadBool(*VariantObject, TEXT("use_live_tools"), false);
		Variant.ModelOverride     = ReadString(*VariantObject, TEXT("model_override"));

		const TArray<TSharedPtr<FJsonValue>>* Nodes = nullptr;
		if ((*VariantObject)->TryGetArrayField(TEXT("nodes"), Nodes) && Nodes != nullptr)
		{
			for (const TSharedPtr<FJsonValue>& Entry : *Nodes)
			{
				const TSharedPtr<FJsonObject>* NodeObject = nullptr;
				if (!Entry.IsValid() || !Entry->TryGetObject(NodeObject) || NodeObject == nullptr)
				{
					continue;
				}
				FTryllLabNodeSummary Node;
				Node.Name = ReadString(*NodeObject, TEXT("name"));
				Node.Type = ReadString(*NodeObject, TEXT("type"));
				Variant.Nodes.Add(MoveTemp(Node));
			}
		}

		const TArray<TSharedPtr<FJsonValue>>* Overrides = nullptr;
		if ((*VariantObject)->TryGetArrayField(TEXT("overrides"), Overrides) && Overrides != nullptr)
		{
			for (const TSharedPtr<FJsonValue>& Entry : *Overrides)
			{
				const TSharedPtr<FJsonObject>* OverrideObject = nullptr;
				if (!Entry.IsValid() || !Entry->TryGetObject(OverrideObject)
					|| OverrideObject == nullptr)
				{
					continue;
				}
				FTryllLabParamOverride Override;
				Override.NodeName  = ReadString(*OverrideObject, TEXT("node"));
				Override.FieldPath = ReadString(*OverrideObject, TEXT("field"));
				Override.Value     = ReadString(*OverrideObject, TEXT("value"));
				Variant.Overrides.Add(MoveTemp(Override));
			}
		}

		Snapshot.Variants.Add(MoveTemp(Variant));
	}

	OutRun.Snapshot       = Snapshot;
	OutRun.StartedAtUtc   = ParseTime(ReadString(Header, TEXT("started_at")));
	OutRun.CompletedAtUtc = ParseTime(ReadString(Header, TEXT("completed_at")));
	OutRun.Status         = ParseRunStatus(ReadString(Header, TEXT("status")));
	for (const FString& Issue : ReadStrings(Header, TEXT("issues")))
	{
		if (!Issue.IsEmpty())
		{
			OutRun.Issues.Add(Issue);
		}
	}

	const TSharedPtr<FJsonObject>* ServerObject = nullptr;
	if (Header->TryGetObjectField(TEXT("server"), ServerObject) && ServerObject != nullptr)
	{
		OutRun.Server.ExePath     = ReadString(*ServerObject, TEXT("exe_path"));
		OutRun.Server.BuildConfig = ReadString(*ServerObject, TEXT("build_config"));
		OutRun.Server.Version     = ReadString(*ServerObject, TEXT("version"));
		OutRun.Server.RunId       = ReadString(*ServerObject, TEXT("run_id"));
		OutRun.Server.bManaged    = ReadBool(*ServerObject, TEXT("managed"), false);
	}

	TMap<FString, FTryllLabSampleResult> SamplesById;
	TArray<FString> SampleOrder;
	TSet<FString> SeenKeys;

	for (int32 LineIndex = 1; LineIndex < Lines.Num(); ++LineIndex)
	{
		const TSharedPtr<FJsonObject> Turn = ParseLine(Lines[LineIndex], Why);
		if (!Turn.IsValid())
		{
			OutError = FString::Printf(TEXT("Line %d: invalid turn JSON (%s)."), LineIndex + 1, *Why);
			return false;
		}

		const FString SampleId = ReadString(Turn, TEXT("sample_id"));
		// Absent in format 3, which had one script.
		const int32 DialogueIndex = ReadInt(Turn, TEXT("dialogue_index"), 0);
		if (!Dialogues.IsValidIndex(DialogueIndex))
		{
			OutError = FString::Printf(TEXT("Line %d: dialogue_index out of range."), LineIndex + 1);
			return false;
		}
		const FTryllLabScriptSnapshot& Script = Dialogues[DialogueIndex];
		const int32 TurnIndex  = ReadInt(Turn, TEXT("turn_index"), -1);
		if (SampleId.IsEmpty() || TurnIndex < 0 || TurnIndex >= Script.Steps.Num())
		{
			OutError = FString::Printf(TEXT("Line %d: invalid sample_id/turn_index."), LineIndex + 1);
			return false;
		}
		const int32 FlatIndex = Snapshot.TurnOffset(DialogueIndex) + TurnIndex;

		const FString Key = FString::Printf(TEXT("%s:%d:%d"), *SampleId, DialogueIndex, TurnIndex);
		if (SeenKeys.Contains(Key))
		{
			OutError = FString::Printf(TEXT("Line %d: duplicate cell %s."), LineIndex + 1, *Key);
			return false;
		}
		SeenKeys.Add(Key);

		if (!SamplesById.Contains(SampleId))
		{
			const int32 VariantIndex = ReadInt(Turn, TEXT("variant_index"), 0);
			if (VariantIndex < 0 || VariantIndex >= Snapshot.Variants.Num())
			{
				OutError = FString::Printf(TEXT("Line %d: variant_index out of range."), LineIndex + 1);
				return false;
			}
			FTryllLabSampleResult Sample;
			Sample.SampleId      = SampleId;
			Sample.VariantIndex  = VariantIndex;
			Sample.VariantLabel  = ReadString(Turn, TEXT("variant_label"));
			if (Sample.VariantLabel.IsEmpty())
			{
				Sample.VariantLabel = Snapshot.Variants[VariantIndex].Label;
			}
			Sample.VariantSwatch = Snapshot.Variants[VariantIndex].Swatch;
			Sample.Repeat        = ReadInt(Turn, TEXT("repeat"), 0);
			Sample.Seed          = ReadInt(Turn, TEXT("seed"), 0);
			Sample.Turns.SetNum(Snapshot.TotalSteps());
			for (FTryllLabTurnRecord& Record : Sample.Turns)
			{
				Record.TurnIndex = INDEX_NONE;
			}
			SamplesById.Add(SampleId, MoveTemp(Sample));
			SampleOrder.Add(SampleId);
		}

		FTryllLabTurnRecord Record;
		Record.DialogueIndex   = DialogueIndex;
		Record.TurnIndex       = TurnIndex;
		Record.bSeeded         = ReadBool(Turn, TEXT("seeded"), false);
		Record.UserText        = ReadString(Turn, TEXT("user_text"));
		Record.AnswerText      = ReadString(Turn, TEXT("answer_text"));
		Record.Status          = ParseCellStatus(ReadString(Turn, TEXT("status")));
		Record.DurationMs      = ReadInt(Turn, TEXT("duration_ms"), 0);
		Record.TtftMs          = ReadInt(Turn, TEXT("ttft_ms"), 0);
		Record.TokensGenerated = ReadInt(Turn, TEXT("tokens_generated"), 0);
		// Recomputed with the same counter the runner uses rather than trusted from the file.
		Record.Words           = FTryllDialogLabChecks::CountWords(Record.AnswerText);
		if (Turn->HasField(TEXT("error")))
		{
			Record.ErrorMessage = ReadString(Turn, TEXT("error"));
		}

		Record.CheckOutcomes = bLegacy
			? MapV3Outcomes(Script, TurnIndex, Record, ParseV3Failures(Turn))
			: FTryllDialogCheckJson::ReadOutcomes(ArrayField(Turn, TEXT("check_outcomes")));

		const TSharedPtr<FJsonObject>* DebugObject = nullptr;
		if (Turn->TryGetObjectField(TEXT("debug_info"), DebugObject) && DebugObject != nullptr)
		{
			Record.DebugInfo    = *DebugObject;
			Record.DebugInfoRaw = RawDebugInfo(Lines[LineIndex]);
			if (Record.DebugInfoRaw.IsEmpty())
			{
				Record.DebugInfoRaw = CompactJson(*DebugObject);
			}
		}

		SamplesById[SampleId].Turns[FlatIndex] = MoveTemp(Record);
	}

	// Rectangular completeness: every sample must have every turn of every dialogue.
	for (const FString& SampleId : SampleOrder)
	{
		FTryllLabSampleResult& Sample = SamplesById[SampleId];
		for (int32 D = 0; D < Dialogues.Num(); ++D)
		{
			const int32 Offset = Snapshot.TurnOffset(D);
			for (int32 TurnIndex = 0; TurnIndex < Dialogues[D].Steps.Num(); ++TurnIndex)
			{
				if (Sample.Turns[Offset + TurnIndex].TurnIndex == INDEX_NONE)
				{
					OutError = Dialogues.Num() > 1
						? FString::Printf(TEXT("Sample %s is missing dialogue_index %d turn_index %d."),
							*SampleId, D, TurnIndex)
						: FString::Printf(TEXT("Sample %s is missing turn_index %d."), *SampleId, TurnIndex);
					return false;
				}
			}
		}
		OutRun.Samples.Add(Sample);
	}

	if (OutRun.Samples.Num() == 0)
	{
		OutError = TEXT("File has a header but no turn records.");
		return false;
	}

	// Stable display order regardless of line order in the file.
	OutRun.Samples.Sort([](const FTryllLabSampleResult& A, const FTryllLabSampleResult& B)
	{
		return A.VariantIndex != B.VariantIndex
			? A.VariantIndex < B.VariantIndex
			: A.Repeat < B.Repeat;
	});

	return true;
}
