// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllAgentLogDocument.h"

#include "TryllJsonWriter.h"
#include "TryllObservationTypes.h"
#include "Generated/TryllAgentLogJson.gen.h"
#include "Generated/TryllMessageDescriptors.gen.h"
#include "CodegenFingerprint.gen.h"
#include "tryll/Protocol.h"
#include "messages_generated.h"
#include "agent_log_generated.h"

#include <cctype>
#include <cstdlib>
#include <map>
#include <string>

namespace TryllAgentLogExportDetail
{

struct FRun
{
	FTryllObservationEnvelope First;
	FTryllObservationEnvelope Last;
	std::string Type;
	std::string Payload;
	int32 Chunks = 1;
	FString NodeName;
	uint64 FirstSeq = 0;
	uint64 LastSeq = 0;
	bool bHasIsDelta = false;
	std::string Coalesce;
	std::string CoalesceField;
	bool bHasNodeName = false;
};

struct FTurn
{
	std::string Id;
	uint64 AgentId = 0;
	uint64 FirstSeq = 0;
	uint64 LastSeq = 0;
	double FirstMs = 0;
	double LastMs = 0;
	bool bSawAnswer = false;
	double FirstAnswerMs = 0;
	bool bComplete = false;
	std::string Status;
	bool bHasTokens = false;
	uint64 Tokens = 0;
	bool bFromDebug = false;
	double DurationMs = 0;
	double TtftMs = 0;
	bool bHasDuration = false;
	bool bHasTtft = false;
};

struct FOrigin
{
	bool bUseMono = false;
	double Mono = 0;
	FDateTime Utc;
};

std::string ToUtf8(const FString& Text)
{
	return std::string(TCHAR_TO_UTF8(*Text));
}

FString FromUtf8(const std::string& Text)
{
	const FUTF8ToTCHAR Convert(Text.c_str());
	return FString(Convert.Length(), Convert.Get());
}

void SkipWs(const std::string& Json, size_t& Index)
{
	while (Index < Json.size() && (Json[Index] == ' ' || Json[Index] == '\t' || Json[Index] == '\n' || Json[Index] == '\r'))
		++Index;
}

bool Locate(const std::string& Json, const std::string& Key, size_t& ValueAt, size_t From = 0)
{
	const std::string Needle = "\"" + Key + "\"";
	bool bString = false;
	bool bEscape = false;
	for (size_t i = From; i < Json.size(); ++i)
	{
		const char C = Json[i];
		if (bString)
		{
			if (bEscape) bEscape = false;
			else if (C == '\\') bEscape = true;
			else if (C == '"') bString = false;
			continue;
		}
		if (C != '"') continue;
		if (Json.compare(i, Needle.size(), Needle) != 0)
		{
			bString = true;
			continue;
		}
		size_t After = i + Needle.size();
		SkipWs(Json, After);
		if (After >= Json.size() || Json[After] != ':')
		{
			bString = true;
			continue;
		}
		ValueAt = After + 1;
		return true;
	}
	return false;
}

std::string ParseString(const std::string& Json, size_t& Index)
{
	if (Index >= Json.size() || Json[Index] != '"') return {};
	++Index;
	std::string Out;
	while (Index < Json.size())
	{
		const char C = Json[Index++];
		if (C == '"') break;
		if (C != '\\')
		{
			Out.push_back(C);
			continue;
		}
		if (Index >= Json.size()) break;
		const char Esc = Json[Index++];
		switch (Esc)
		{
		case 'n': Out.push_back('\n'); break;
		case 'r': Out.push_back('\r'); break;
		case 't': Out.push_back('\t'); break;
		default: Out.push_back(Esc); break;
		}
	}
	return Out;
}

bool ReadString(const std::string& Json, const std::string& Key, std::string& Out)
{
	size_t At = 0;
	if (!Locate(Json, Key, At)) return false;
	SkipWs(Json, At);
	if (At >= Json.size() || Json[At] != '"') return false;
	Out = ParseString(Json, At);
	return true;
}

std::string Quote(const std::string& Value)
{
	FTryllJsonWriter Writer(false);
	Writer.String(Value);
	return Writer.Utf8();
}

std::string ReplaceString(std::string Json, const std::string& Key, const std::string& Value)
{
	size_t At = 0;
	if (!Locate(Json, Key, At)) return Json;
	SkipWs(Json, At);
	if (At >= Json.size() || Json[At] != '"') return Json;
	const size_t Start = At;
	ParseString(Json, At);
	return Json.substr(0, Start) + Quote(Value) + Json.substr(At);
}

bool ReadBool(const std::string& Json, const std::string& Key, bool& Out)
{
	size_t At = 0;
	if (!Locate(Json, Key, At)) return false;
	SkipWs(Json, At);
	if (Json.compare(At, 4, "true") == 0) { Out = true; return true; }
	if (Json.compare(At, 5, "false") == 0) { Out = false; return true; }
	return false;
}

std::string ReplaceBool(std::string Json, const std::string& Key, bool bValue)
{
	size_t At = 0;
	if (!Locate(Json, Key, At)) return Json;
	SkipWs(Json, At);
	const std::string Token = bValue ? "true" : "false";
	if (Json.compare(At, 4, "true") == 0)
		return Json.substr(0, At) + Token + Json.substr(At + 4);
	if (Json.compare(At, 5, "false") == 0)
		return Json.substr(0, At) + Token + Json.substr(At + 5);
	return Json;
}

bool ReadDoubleAt(const std::string& Json, size_t& Index, double& Out)
{
	if (Index >= Json.size()) return false;
	char* End = nullptr;
	Out = std::strtod(Json.c_str() + Index, &End);
	if (End == nullptr || End == Json.c_str() + Index) return false;
	Index = static_cast<size_t>(End - Json.c_str());
	return true;
}

bool ReadDouble(const std::string& Json, const std::string& Key, double& Out)
{
	size_t At = 0;
	if (!Locate(Json, Key, At)) return false;
	SkipWs(Json, At);
	return ReadDoubleAt(Json, At, Out);
}

std::string SliceObject(const std::string& Json, size_t At)
{
	if (At >= Json.size() || Json[At] != '{') return {};
	int Depth = 0;
	bool bString = false;
	bool bEscape = false;
	for (size_t i = At; i < Json.size(); ++i)
	{
		const char C = Json[i];
		if (bString)
		{
			if (bEscape) bEscape = false;
			else if (C == '\\') bEscape = true;
			else if (C == '"') bString = false;
			continue;
		}
		if (C == '"') { bString = true; continue; }
		if (C == '{') ++Depth;
		else if (C == '}')
		{
			--Depth;
			if (Depth == 0) return Json.substr(At, i - At + 1);
		}
	}
	return {};
}

void ApplyDebugTiming(FTurn& Turn, const std::string& Payload)
{
	size_t At = 0;
	if (!Locate(Payload, "debug_info", At)) return;
	SkipWs(Payload, At);
	std::string Slice;
	if (At < Payload.size() && Payload[At] == '"')
		Slice = ParseString(Payload, At);
	else if (At < Payload.size() && Payload[At] == '{')
		Slice = SliceObject(Payload, At);
	if (Slice.empty()) return;
	double Duration = 0.0;
	if (ReadDouble(Slice, "duration_s", Duration))
	{
		Turn.DurationMs = Duration * 1000.0;
		Turn.bHasDuration = true;
		Turn.bFromDebug = true;
	}
	double Ttft = 0.0;
	if (ReadDouble(Slice, "time_to_first_token_ms", Ttft))
	{
		Turn.TtftMs = Ttft;
		Turn.bHasTtft = true;
		Turn.bFromDebug = true;
	}
}

bool ReadLong(const std::string& Json, size_t& Index, int64& Out)
{
	const size_t Start = Index;
	if (Index < Json.size() && Json[Index] == '-') ++Index;
	if (Index >= Json.size() || Json[Index] < '0' || Json[Index] > '9')
	{
		Index = Start;
		return false;
	}
	while (Index < Json.size() && Json[Index] >= '0' && Json[Index] <= '9') ++Index;
	const size_t End = Index;
	if (Index < Json.size() && (Json[Index] == '.' || Json[Index] == 'e' || Json[Index] == 'E'))
	{
		if (Json[Index] == '.')
		{
			++Index;
			while (Index < Json.size() && Json[Index] >= '0' && Json[Index] <= '9') ++Index;
		}
	}
	Out = FCStringAnsi::Atoi64(Json.substr(Start, End - Start).c_str());
	return true;
}

void ReadNumbers(const std::string& Json, const std::string& Key, TArray<int64>& Out)
{
	size_t From = 0;
	size_t At = 0;
	while (Locate(Json, Key, At, From))
	{
		SkipWs(Json, At);
		int64 Number = 0;
		if (ReadLong(Json, At, Number))
			Out.Add(Number);
		From = At;
	}
}

std::string ReplaceNthNumber(std::string Json, const std::string& Key, int32 Index, int64 Value)
{
	size_t From = 0;
	int32 Seen = 0;
	size_t At = 0;
	while (Locate(Json, Key, At, From))
	{
		SkipWs(Json, At);
		const size_t Start = At;
		int64 Number = 0;
		if (!ReadLong(Json, At, Number))
		{
			From = At + 1;
			continue;
		}
		if (Seen == Index)
		{
			return Json.substr(0, Start) + std::to_string(Value) + Json.substr(At);
		}
		++Seen;
		From = At;
	}
	return Json;
}

std::string SumKey(std::string Kept, const std::string& Incoming, const std::string& Key)
{
	TArray<int64> Left;
	TArray<int64> Right;
	ReadNumbers(Kept, Key, Left);
	ReadNumbers(Incoming, Key, Right);
	const int32 Count = FMath::Min(Left.Num(), Right.Num());
	for (int32 i = Count - 1; i >= 0; --i)
		Kept = ReplaceNthNumber(Kept, Key, i, Left[i] + Right[i]);
	return Kept;
}

std::string TypeName(const FTryllObservationEnvelope& Env)
{
	if (Env.Frame.Num() > 0)
	{
		if (Env.bIsLocal)
		{
			const auto* Root = ::Tryll::GetLogLocalEnvelope(Env.Frame.GetData());
			if (Root != nullptr)
			{
				switch (Root->body_type())
				{
				case ::Tryll::LogLocalBody_LogConnectionState: return "LogConnectionState";
				case ::Tryll::LogLocalBody_LogLocalError: return "LogLocalError";
				case ::Tryll::LogLocalBody_LogCaptureGap: return "LogCaptureGap";
				default: break;
				}
			}
		}
		else
		{
			const auto* Message = ::Tryll::GetMessage(Env.Frame.GetData());
			if (Message != nullptr)
			{
				const auto* Desc = ::Tryll::MessageDescriptor(Message->body_type());
				if (Desc != nullptr && Desc->Name != nullptr && Desc->Name[0] != '\0')
					return Desc->Name;
			}
		}
	}
	if (Env.bIsCaptureGap) return "LogCaptureGap";
	const auto* Desc = ::Tryll::MessageDescriptor(Env.Body);
	if (Desc != nullptr && Desc->Name != nullptr) return Desc->Name;
	return {};
}

std::string PayloadOf(const FTryllObservationEnvelope& Env)
{
	if (Env.Frame.Num() == 0) return "{}";
	std::vector<FTryllOmittedField> Omitted;
	Omitted.reserve(static_cast<size_t>(Env.Omitted.Num()));
	for (const FTryllObservationEnvelope::FOmittedField& Field : Env.Omitted)
	{
		FTryllOmittedField Item;
		Item.Path = ToUtf8(Field.Path);
		Item.Count = Field.Count;
		Item.Bytes = Field.Bytes;
		Omitted.push_back(Item);
	}
	FTryllJsonWriter Writer(false);
	if (Env.bIsLocal)
		::Tryll::AgentLog::WriteLocal(Writer, Env.Frame.GetData(), static_cast<size_t>(Env.Frame.Num()), Omitted);
	else
		::Tryll::AgentLog::WriteMessage(Writer, Env.Frame.GetData(), static_cast<size_t>(Env.Frame.Num()), Omitted);
	return Writer.Utf8().empty() ? "{}" : Writer.Utf8();
}

std::string TurnId(const FTryllObservationEnvelope& Env)
{
	if (!Env.TurnKey.bHasValue) return {};
	if (Env.TurnKey.RequestId != 0)
		return std::to_string(Env.TurnKey.ConnectionEpoch) + ":" + std::to_string(Env.TurnKey.AgentId) + ":" + std::to_string(Env.TurnKey.RequestId);
	return std::to_string(Env.TurnKey.ConnectionEpoch) + ":" + std::to_string(Env.TurnKey.AgentId) + ":s" + std::to_string(Env.TurnKey.SyntheticSequence);
}

std::string StreamKey(const FTryllObservationEnvelope& Env)
{
	const std::string Turn = TurnId(Env);
	if (!Turn.empty()) return "t:" + Turn;
	const std::string Epoch = std::to_string(Env.ConnectionEpoch);
	if (Env.RequestId != 0) return "r:" + Epoch + ":" + std::to_string(Env.RequestId);
	if (Env.VoiceInputId != 0) return "v:" + Epoch + ":" + std::to_string(Env.VoiceInputId);
	return "e:" + std::to_string(Env.EventId);
}

std::string KindOf(const FTryllObservationEnvelope& Env, const std::string& Type)
{
	if (Env.bIsCaptureGap || Type == "LogCaptureGap") return "marker";
	if (Env.bIsLocal && Type == "LogConnectionState") return "connection";
	if (Env.bIsLocal && Type == "LogLocalError") return "local_error";
	if (Env.Body == ::Tryll::MessageBody_ErrorResponse || (Env.bIsError && !Env.bIsLocal)) return "error";
	if (Env.bIsLocal && Env.bIsError) return "local_error";
	if (Env.Direction == ETryllObservationDirection::Outbound) return "request";
	const auto* Request = ::Tryll::MessageDescriptor(Env.RequestBody);
	if (Request != nullptr && Request->Terminal == Env.Body && Env.Body != ::Tryll::MessageBody_NONE)
		return "reply";
	if (Env.Body == ::Tryll::MessageBody_Ack) return "reply";
	return "push";
}

FString FormatUtc(const FDateTime& Time)
{
	return FString::Printf(TEXT("%04d-%02d-%02dT%02d:%02d:%02d.%03dZ"),
		Time.GetYear(), Time.GetMonth(), Time.GetDay(),
		Time.GetHour(), Time.GetMinute(), Time.GetSecond(), Time.GetMillisecond());
}

void WriteOptional(FTryllJsonWriter& Writer, const char* Key, const FString& Value)
{
	if (Value.IsEmpty()) return;
	Writer.Key(Key);
	Writer.String(ToUtf8(Value));
}

TArray<FRun> Coalesce(const TArray<FTryllObservationEnvelope>& Events)
{
	TArray<FRun> Runs;
	std::map<std::string, int32> Open;
	uint64 Fallback = 1;
	for (const FTryllObservationEnvelope& Env : Events)
	{
		const std::string Type = TypeName(Env);
		const uint64 Seq = Env.EventId != 0 ? Env.EventId : Fallback;
		Fallback = Seq + 1;
		const auto* Desc = ::Tryll::MessageDescriptor(Env.Body);
		const std::string Rule = (Desc != nullptr && Desc->Coalesce != nullptr) ? Desc->Coalesce : "";
		const std::string Field = (Desc != nullptr && Desc->CoalesceField != nullptr) ? Desc->CoalesceField : "";
		const bool bDelta = Type == "AnswerText";
		const bool bNode = bDelta || Type == "TtsAudioFrame" || Type == "TtsVisemeFrame" || Type == "NodeEvent";
		const std::string Key = StreamKey(Env);
		const auto Found = Open.find(Key);
		if (!Rule.empty() && Found != Open.end())
		{
			FRun& Run = Runs[Found->second];
			const bool bSameType = Run.Type == Type;
			const bool bSameNode = !bNode || Run.NodeName == Env.NodeName;
			if (bSameType && bSameNode)
			{
				const std::string Incoming = PayloadOf(Env);
				if (Rule == "last")
					Run.Payload = Incoming;
				else if (Rule == "concat" && !Field.empty())
				{
					std::string Left;
					std::string Right;
					ReadString(Run.Payload, Field, Left);
					ReadString(Incoming, Field, Right);
					const bool bReplace = bDelta && !Env.bFlag;
					Run.Payload = ReplaceString(Run.Payload, Field, bReplace ? Right : Left + Right);
					bool bValue = false;
					if (ReadBool(Incoming, "is_delta", bValue))
						Run.Payload = ReplaceBool(Run.Payload, "is_delta", bValue);
					if (ReadBool(Incoming, "is_final", bValue))
						Run.Payload = ReplaceBool(Run.Payload, "is_final", bValue);
				}
				else if (Rule == "sum")
				{
					if (!Field.empty())
						Run.Payload = SumKey(Run.Payload, Incoming, Field);
					else
					{
						Run.Payload = SumKey(Run.Payload, Incoming, "count");
						Run.Payload = SumKey(Run.Payload, Incoming, "bytes");
					}
				}
				Run.Last = Env;
				Run.Chunks += 1;
				Run.LastSeq = Seq;
				Run.NodeName = Env.NodeName;
				continue;
			}
		}

		FRun Started;
		Started.First = Env;
		Started.Last = Env;
		Started.Type = Type;
		Started.Payload = PayloadOf(Env);
		Started.NodeName = Env.NodeName;
		Started.FirstSeq = Seq;
		Started.LastSeq = Seq;
		Started.bHasIsDelta = bDelta;
		Started.Coalesce = Rule;
		Started.CoalesceField = Field;
		Started.bHasNodeName = bNode;
		Runs.Add(Started);
		if (!Rule.empty())
			Open[Key] = Runs.Num() - 1;
		else
			Open.erase(Key);
	}
	return Runs;
}

} // namespace TryllAgentLogExportDetail

FString FTryllAgentLogDocument::FileName(const FString& ProjectName, const FDateTime& LocalNow)
{
	FString Project = ProjectName.IsEmpty() ? TEXT("project") : ProjectName;
	Project.ReplaceInline(TEXT(" "), TEXT("_"));
	return FString::Printf(TEXT("TryllAgentLog_%s_%s.json"), *Project, *LocalNow.ToString(TEXT("%Y%m%d-%H%M%S")));
}

FString FTryllAgentLogDocument::Write(const TArray<FTryllObservationEnvelope>& Events, const FTryllAgentLogExportInfo& Info)
{
	using namespace TryllAgentLogExportDetail;
	const TArray<FRun> Runs = Coalesce(Events);
	FOrigin Origin;
	Origin.Utc = FDateTime::UtcNow();
	if (Events.Num() > 0)
	{
		Origin.Utc = Events[0].WallTimeUtc;
		Origin.bUseMono = Events[0].MonotonicSeconds != 0.0;
		Origin.Mono = Events[0].MonotonicSeconds;
	}
	auto Millis = [&Origin](const FTryllObservationEnvelope& Env) -> double
	{
		if (Origin.bUseMono && Env.MonotonicSeconds != 0.0)
			return (Env.MonotonicSeconds - Origin.Mono) * 1000.0;
		return (Env.WallTimeUtc - Origin.Utc).GetTotalMilliseconds();
	};

	FTryllJsonWriter Writer(true);
	Writer.BeginObject();
	Writer.Key("format"); Writer.String("tryll.agent_log");
	Writer.Key("format_version"); Writer.Int64(1);
	Writer.Key("exported_at"); Writer.String(ToUtf8(FormatUtc(FDateTime::UtcNow())));
	Writer.Key("exporter");
	Writer.BeginObject();
	Writer.Key("host"); Writer.String(ToUtf8(Info.Host));
	Writer.Key("host_version"); Writer.String(ToUtf8(Info.HostVersion));
	Writer.Key("plugin_version"); Writer.String(ToUtf8(Info.PluginVersion));
	Writer.Key("project_name"); Writer.String(ToUtf8(Info.ProjectName));
	Writer.Key("os"); Writer.String(ToUtf8(Info.Os));
	Writer.EndObject();
	Writer.Key("protocol");
	Writer.BeginObject();
	Writer.Key("version"); Writer.UInt64(::Tryll::ProtocolVersion);
	Writer.Key("codegen_fingerprint"); Writer.String(::Tryll::Generated::kCodegenFingerprint);
	Writer.EndObject();
	Writer.Key("server");
	Writer.BeginObject();
	WriteOptional(Writer, "version", Info.ServerVersion);
	WriteOptional(Writer, "run_id", Info.ServerRunId);
	WriteOptional(Writer, "build_config", Info.ServerBuildConfig);
	WriteOptional(Writer, "mode", Info.ServerMode);
	Writer.EndObject();
	Writer.Key("capture");
	Writer.BeginObject();
	Writer.Key("state"); Writer.String(ToUtf8(Info.CaptureState.IsEmpty() ? TEXT("idle") : Info.CaptureState));
	Writer.Key("history_discarded_by_reload"); Writer.Bool(Info.bHistoryDiscarded);
	if (Info.SelectedAgentId != 0)
	{
		Writer.Key("selected_agent_id"); Writer.UInt64(Info.SelectedAgentId);
	}
	Writer.Key("retention");
	Writer.BeginObject();
	Writer.Key("max_completed_turns_per_agent"); Writer.Int64(Info.MaxTurns);
	Writer.Key("max_observations_per_agent"); Writer.Int64(Info.MaxObservations);
	Writer.Key("max_raw_session_observations"); Writer.Int64(Info.MaxRaw);
	Writer.EndObject();
	Writer.Key("evicted");
	Writer.BeginObject();
	Writer.Key("turns"); Writer.Int64(Info.EvictedTurns);
	Writer.Key("observations"); Writer.Int64(Info.EvictedObservations);
	Writer.EndObject();
	if (Events.Num() > 0)
	{
		Writer.Key("oldest_retained_utc");
		Writer.String(ToUtf8(FormatUtc(Events[0].WallTimeUtc)));
	}
	Writer.Key("coalesced"); Writer.Bool(true);
	Writer.EndObject();
	Writer.Key("agents");
	Writer.BeginArray();
	for (const FTryllAgentLogExportAgent& Agent : Info.Agents)
	{
		Writer.BeginObject();
		Writer.Key("agent_id"); Writer.UInt64(Agent.AgentId);
		if (Agent.Epoch != 0) { Writer.Key("epoch"); Writer.UInt64(Agent.Epoch); }
		Writer.Key("label"); Writer.String(ToUtf8(Agent.Label));
		Writer.Key("created_seq");
		if (Agent.bHasCreatedSeq) Writer.UInt64(Agent.CreatedSeq); else Writer.Null();
		Writer.Key("destroyed_seq");
		if (Agent.bHasDestroyedSeq) Writer.UInt64(Agent.DestroyedSeq); else Writer.Null();
		Writer.Key("diagnostics"); Writer.Bool(Agent.bDiagnostics);
		Writer.Key("created_before_diagnostics_capture"); Writer.Bool(Agent.bCreatedBeforeDiagnosticsCapture);
		Writer.Key("completed_turns"); Writer.Int64(Agent.CompletedTurns);
		Writer.EndObject();
	}
	Writer.EndArray();
	Writer.Key("events");
	Writer.BeginArray();
	for (const FRun& Run : Runs)
	{
		const std::string Kind = KindOf(Run.First, Run.Type);
		Writer.BeginObject();
		Writer.Key("seq"); Writer.UInt64(Run.FirstSeq);
		Writer.Key("t_ms"); Writer.Double(Millis(Run.First));
		if (Run.Chunks > 1)
		{
			Writer.Key("t_end_ms"); Writer.Double(Millis(Run.Last));
			Writer.Key("chunks"); Writer.Int64(Run.Chunks);
		}
		Writer.Key("utc"); Writer.String(ToUtf8(FormatUtc(Run.First.WallTimeUtc)));
		if (Run.First.ConnectionEpoch != 0)
		{
			Writer.Key("epoch"); Writer.UInt64(Run.First.ConnectionEpoch);
		}
		if (!Run.First.bIsLocal && !Run.First.bIsCaptureGap)
		{
			Writer.Key("dir");
			Writer.String(Run.First.Direction == ETryllObservationDirection::Outbound ? "out" : "in");
		}
		Writer.Key("kind"); Writer.String(Kind);
		const char* Scope = "session";
		if (Run.First.Scope == ETryllObservationScope::CuratedAgent) Scope = "agent";
		else if (Run.First.Scope == ETryllObservationScope::MetadataOnly) Scope = "metadata";
		Writer.Key("scope"); Writer.String(Scope);
		if (!Run.Type.empty())
		{
			Writer.Key("type"); Writer.String(Run.Type);
		}
		if (Run.First.RequestId != 0) { Writer.Key("request_id"); Writer.UInt64(Run.First.RequestId); }
		if (Run.First.AgentId != 0) { Writer.Key("agent_id"); Writer.UInt64(Run.First.AgentId); }
		if (Run.First.VoiceInputId != 0) { Writer.Key("voice_input_id"); Writer.UInt64(Run.First.VoiceInputId); }
		const std::string Turn = TurnId(Run.First);
		if (!Turn.empty()) { Writer.Key("turn"); Writer.String(Turn); }
		if (Run.First.CarrierRequestId != 0)
		{
			Writer.Key("carrier_request_id"); Writer.UInt64(Run.First.CarrierRequestId);
		}
		if (Kind == "error" || Kind == "local_error")
		{
			std::string ErrorOrigin;
			if (Kind == "error" && Run.First.Body == ::Tryll::MessageBody_ErrorResponse)
				ErrorOrigin = "server";
			else
				ReadString(Run.Payload, "origin", ErrorOrigin);
			if (!ErrorOrigin.empty())
			{
				Writer.Key("origin");
				Writer.String(ErrorOrigin);
			}
		}
		Writer.Key("payload");
		Writer.Raw(Run.Payload.empty() ? "{}" : Run.Payload);
		Writer.EndObject();
	}
	Writer.EndArray();
	Writer.Key("turns");
	Writer.BeginArray();
	TArray<FTurn> Turns;
	TMap<FString, int32> TurnIndex;
	for (const FRun& Run : Runs)
	{
		const std::string Id = TurnId(Run.First);
		if (Id.empty()) continue;
		const FString Key = FromUtf8(Id);
		int32* Slot = TurnIndex.Find(Key);
		if (Slot == nullptr)
		{
			FTurn Turn;
			Turn.Id = Id;
			Turn.AgentId = Run.First.AgentId;
			Turn.FirstSeq = Run.FirstSeq;
			Turn.FirstMs = Millis(Run.First);
			Turns.Add(Turn);
			TurnIndex.Add(Key, Turns.Num() - 1);
			Slot = TurnIndex.Find(Key);
		}
		FTurn& Turn = Turns[*Slot];
		Turn.LastSeq = Run.LastSeq;
		Turn.LastMs = Millis(Run.Last);
		if (Run.Type == "AnswerText" && !Turn.bSawAnswer)
		{
			Turn.bSawAnswer = true;
			Turn.FirstAnswerMs = Millis(Run.First);
		}
		if (Run.Type == "TurnComplete" || KindOf(Run.First, Run.Type) == "error")
			Turn.bComplete = true;
		if (Run.Type == "TurnComplete")
		{
			std::string Status;
			if (ReadString(Run.Payload, "status", Status))
			{
				for (char& Char : Status)
					Char = static_cast<char>(std::tolower(static_cast<unsigned char>(Char)));
				Turn.Status = Status;
			}
			if (Run.Last.TokensGenerated != 0)
			{
				Turn.bHasTokens = true;
				Turn.Tokens = Run.Last.TokensGenerated;
			}
			ApplyDebugTiming(Turn, Run.Payload);
		}
		if (KindOf(Run.First, Run.Type) == "error" || KindOf(Run.Last, Run.Type) == "error")
			Turn.Status = "error";
	}
	for (FTurn& Turn : Turns)
	{
		if (!Turn.bHasDuration)
		{
			Turn.DurationMs = Turn.LastMs - Turn.FirstMs;
			Turn.bHasDuration = true;
		}
		if (!Turn.bHasTtft && Turn.bSawAnswer)
		{
			Turn.TtftMs = Turn.FirstAnswerMs - Turn.FirstMs;
			Turn.bHasTtft = true;
		}
		if (Turn.bComplete && Turn.Status.empty())
			Turn.Status = "success";
		Writer.BeginObject();
		Writer.Key("turn"); Writer.String(Turn.Id);
		if (Turn.AgentId != 0) { Writer.Key("agent_id"); Writer.UInt64(Turn.AgentId); }
		Writer.Key("first_seq"); Writer.UInt64(Turn.FirstSeq);
		Writer.Key("last_seq"); Writer.UInt64(Turn.LastSeq);
		Writer.Key("complete"); Writer.Bool(Turn.bComplete);
		if (Turn.bComplete && !Turn.Status.empty())
		{
			Writer.Key("status"); Writer.String(Turn.Status);
		}
		if (Turn.bHasDuration) { Writer.Key("duration_ms"); Writer.Double(Turn.DurationMs); }
		if (Turn.bHasTtft) { Writer.Key("ttft_ms"); Writer.Double(Turn.TtftMs); }
		if (Turn.bHasTokens) { Writer.Key("tokens_generated"); Writer.UInt64(Turn.Tokens); }
		if (!Turn.bFromDebug) { Writer.Key("timing_source"); Writer.String("client"); }
		Writer.EndObject();
	}
	Writer.EndArray();
	Writer.EndObject();
	return FromUtf8(Writer.Utf8());
}
