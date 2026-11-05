// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "ClientCodec.h"

#include "Win32Undef.h"
#include "messages_generated.h"
#include "Generated/Nodes/TryllNodeParamsBase.h"

#include <flatbuffers/flatbuffers.h>

namespace Tryll::Client {

// WrapMessage / FStringToUtf8 are shared with the generated ClientCodec.gen.cpp
// via ClientCodec.gen.h (included through ClientCodec.h). Defining private copies
// here would collide with the generated unit under UE unity builds (C2995 / C2084).

namespace {

/**
 * Build the wire VariableValue union member for one typed value, filling
 * OutWireType with the matching union tag. Shared by AgentVariable (CreateAgent
 * declarations) and VariableUpdate (UpdateAgentVariablesRequest mutations).
 */
[[nodiscard]] flatbuffers::Offset<void> BuildVariableValueOffset(
	flatbuffers::FlatBufferBuilder& Fbb,
	ETryllVariableType              Type,
	int64                           IntValue,
	double                          FloatValue,
	const FString&                  StringValue,
	bool                            bBoolValue,
	const TArray<FString>&          StringSetValue,
	::Tryll::VariableValue&         OutWireType)
{
	switch (Type)
	{
	case ETryllVariableType::Int:
		OutWireType = ::Tryll::VariableValue_VarInt;
		return ::Tryll::CreateVarInt(Fbb, IntValue).Union();
	case ETryllVariableType::Float:
		OutWireType = ::Tryll::VariableValue_VarFloat;
		return ::Tryll::CreateVarFloat(Fbb, FloatValue).Union();
	case ETryllVariableType::String:
		OutWireType = ::Tryll::VariableValue_VarString;
		return ::Tryll::CreateVarString(Fbb, Fbb.CreateString(FStringToUtf8(StringValue))).Union();
	case ETryllVariableType::Bool:
		OutWireType = ::Tryll::VariableValue_VarBool;
		return ::Tryll::CreateVarBool(Fbb, bBoolValue).Union();
	case ETryllVariableType::StringSet:
	{
		std::vector<std::string> Utf8Strings;
		Utf8Strings.reserve(StringSetValue.Num());
		for (const FString& S : StringSetValue)
			Utf8Strings.push_back(FStringToUtf8(S));
		OutWireType = ::Tryll::VariableValue_VarStringSet;
		return ::Tryll::CreateVarStringSet(Fbb, Fbb.CreateVectorOfStrings(Utf8Strings)).Union();
	}
	default:
		OutWireType = ::Tryll::VariableValue_NONE;
		return flatbuffers::Offset<void>();
	}
}

} // namespace

// ── Manual carve-outs ─────────────────────────────────────────────────────────

TArray<uint8> BuildCreateAgentRequest(
	uint64 RequestId,
	const FTryllGraphDescription& Graph,
	bool bEnableDiagnostics,
	bool bMaintainDialogueHistory,
	const TArray<FTryllVariableDecl>& Variables,
	::Tryll::AgentKvCacheInitialization KvCacheInitialization,
	::Tryll::AgentWorkload Workload,
	const ::Tryll::VoiceProfileT* Voice)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// Build nodes vector.
	TArray<flatbuffers::Offset<::Tryll::NodeDescription>> NodeOffsets;
	NodeOffsets.Reserve(Graph.Nodes.Num());
	for (const FTryllNodeDescription& N : Graph.Nodes)
	{
		checkf(N.Params != nullptr,
			TEXT("Node '%s' has null Params. Every node must carry typed params."),
			*N.Name);

		const std::string NameUtf8 = FStringToUtf8(N.Name);
		auto NameOff = Fbb.CreateString(NameUtf8);

		::Tryll::NodeParams::NodeParams ParamsType = ::Tryll::NodeParams::NodeParams_NONE;
		flatbuffers::Offset<void>       ParamsOffset = N.Params->Pack(Fbb, ParamsType);

		::Tryll::NodeDescriptionBuilder NdBuilder{Fbb};
		NdBuilder.add_name(NameOff);
		NdBuilder.add_params_type(ParamsType);
		NdBuilder.add_params(ParamsOffset);
		NodeOffsets.Add(NdBuilder.Finish());
	}

	auto NodesVec = Fbb.CreateVector(NodeOffsets.GetData(), static_cast<size_t>(NodeOffsets.Num()));
	auto StartOff = Fbb.CreateString(FStringToUtf8(Graph.StartNode));
	auto GraphOff = ::Tryll::CreateGraphDescription(Fbb, NodesVec, StartOff);

	flatbuffers::Offset<flatbuffers::String> DefaultModelOff;
	if (!Graph.DefaultModelName.IsEmpty())
		DefaultModelOff = Fbb.CreateString(FStringToUtf8(Graph.DefaultModelName));

	// Declared per-agent variables — complete declaration, sent once.
	TArray<flatbuffers::Offset<::Tryll::AgentVariable>> VariableOffsets;
	VariableOffsets.Reserve(Variables.Num());
	for (const FTryllVariableDecl& V : Variables)
	{
		if (V.Name.IsEmpty())
			continue;
		auto NameOff = Fbb.CreateString(FStringToUtf8(V.Name));
		::Tryll::VariableValue WireType = ::Tryll::VariableValue_NONE;
		flatbuffers::Offset<void> ValueOff = BuildVariableValueOffset(
			Fbb, V.Type, V.IntValue, V.FloatValue, V.StringValue, V.bBoolValue, V.StringSetValue, WireType);
		VariableOffsets.Add(::Tryll::CreateAgentVariable(
			Fbb, NameOff, WireType, ValueOff, V.bAllowOutputSubstitution));
	}
	auto VariablesVec = Fbb.CreateVector(VariableOffsets.GetData(), static_cast<size_t>(VariableOffsets.Num()));

	const flatbuffers::Offset<::Tryll::VoiceProfile> VoiceOff = Voice
		? ::Tryll::VoiceProfile::Pack(Fbb, Voice)
		: flatbuffers::Offset<::Tryll::VoiceProfile>{};

	::Tryll::CreateAgentRequestBuilder ReqBuilder{Fbb};
	ReqBuilder.add_request_id(RequestId);
	ReqBuilder.add_graph(GraphOff);
	if (!DefaultModelOff.IsNull())
		ReqBuilder.add_default_model_name(DefaultModelOff);
	ReqBuilder.add_enable_diagnostics(bEnableDiagnostics);
	ReqBuilder.add_maintain_dialogue_history(bMaintainDialogueHistory);
	ReqBuilder.add_variables(VariablesVec);
	ReqBuilder.add_kv_cache_initialization(KvCacheInitialization);
	ReqBuilder.add_workload(Workload);
	if (!VoiceOff.IsNull())
		ReqBuilder.add_voice(VoiceOff);
	auto Req = ReqBuilder.Finish();
	return WrapMessage(Fbb, Req, ::Tryll::MessageBody_CreateAgentRequest);
}

TArray<uint8> BuildUpdateAgentVariablesRequest(
	uint64 RequestId, uint64 AgentId, const TArray<FTryllVariableUpdate>& Updates)
{
	flatbuffers::FlatBufferBuilder Fbb;

	TArray<flatbuffers::Offset<::Tryll::VariableUpdate>> UpdateOffsets;
	UpdateOffsets.Reserve(Updates.Num());
	for (const FTryllVariableUpdate& U : Updates)
	{
		auto NameOff = Fbb.CreateString(FStringToUtf8(U.Name));
		::Tryll::VariableValue WireType = ::Tryll::VariableValue_NONE;
		flatbuffers::Offset<void> ValueOff;
		if (U.Op != ETryllVariableOp::Reset)
		{
			ValueOff = BuildVariableValueOffset(
				Fbb, U.ValueType, U.IntValue, U.FloatValue, U.StringValue, U.bBoolValue, U.StringSetValue, WireType);
		}
		UpdateOffsets.Add(::Tryll::CreateVariableUpdate(
			Fbb, NameOff, static_cast<::Tryll::VariableOp>(U.Op), WireType, ValueOff));
	}
	auto UpdatesVec = Fbb.CreateVector(UpdateOffsets.GetData(), static_cast<size_t>(UpdateOffsets.Num()));

	::Tryll::UpdateAgentVariablesRequestBuilder ReqBuilder{Fbb};
	ReqBuilder.add_request_id(RequestId);
	ReqBuilder.add_agent_id(AgentId);
	ReqBuilder.add_updates(UpdatesVec);
	auto Req = ReqBuilder.Finish();
	return WrapMessage(Fbb, Req, ::Tryll::MessageBody_UpdateAgentVariablesRequest);
}

TArray<uint8> BuildResumeAgentRequest(
	uint64 RequestId,
	uint64 AgentId,
	const FString& ResumeNode,
	const TArray<TPair<FString, FString>>& ToolResults)
{
	flatbuffers::FlatBufferBuilder Fbb;

	TArray<flatbuffers::Offset<::Tryll::ToolResult>> ResultOffsets;
	ResultOffsets.Reserve(ToolResults.Num());
	for (const auto& ToolResult : ToolResults)
	{
		ResultOffsets.Add(::Tryll::CreateToolResult(
			Fbb,
			Fbb.CreateString(FStringToUtf8(ToolResult.Key)),
			Fbb.CreateString(FStringToUtf8(ToolResult.Value))));
	}
	const auto ResultsVec = Fbb.CreateVector(
		ResultOffsets.GetData(), static_cast<size_t>(ResultOffsets.Num()));
	const auto ResumeNodeOff = ResumeNode.IsEmpty()
		? flatbuffers::Offset<flatbuffers::String>{}
		: Fbb.CreateString(FStringToUtf8(ResumeNode));

	const auto Req = ::Tryll::CreateResumeAgentRequest(
		Fbb, RequestId, AgentId, ResumeNodeOff, ResultsVec);
	return WrapMessage(Fbb, Req, ::Tryll::MessageBody_ResumeAgentRequest);
}

TArray<uint8> BuildAppendInteractionsRequest(
	uint64 RequestId, uint64 AgentId, const TArray<FTryllDialogInteraction>& Interactions)
{
	flatbuffers::FlatBufferBuilder Fbb;

	TArray<flatbuffers::Offset<::Tryll::DialogInteraction>> PairOffsets;
	PairOffsets.Reserve(Interactions.Num());
	for (const FTryllDialogInteraction& Interaction : Interactions)
	{
		flatbuffers::Offset<flatbuffers::String> UserOff;
		if (!Interaction.UserMessage.IsEmpty())
			UserOff = Fbb.CreateString(FStringToUtf8(Interaction.UserMessage));
		flatbuffers::Offset<flatbuffers::String> AssistantOff;
		if (!Interaction.AssistantMessage.IsEmpty())
			AssistantOff = Fbb.CreateString(FStringToUtf8(Interaction.AssistantMessage));
		PairOffsets.Add(::Tryll::CreateDialogInteraction(Fbb, UserOff, AssistantOff));
	}
	const auto InteractionsVec = Fbb.CreateVector(PairOffsets.GetData(), static_cast<size_t>(PairOffsets.Num()));

	::Tryll::AppendInteractionsRequestBuilder ReqBuilder{Fbb};
	ReqBuilder.add_request_id(RequestId);
	ReqBuilder.add_agent_id(AgentId);
	ReqBuilder.add_interactions(InteractionsVec);
	auto Req = ReqBuilder.Finish();
	return WrapMessage(Fbb, Req, ::Tryll::MessageBody_AppendInteractionsRequest);
}

TArray<uint8> BuildImportDialogRequest(
	uint64 RequestId, uint64 AgentId, const FTryllDialogSnapshot& Snapshot,
	ETryllDialogCompatibility Compatibility, FTryllError& OutError)
{
	flatbuffers::FlatBufferBuilder Fbb;
	flatbuffers::Offset<::Tryll::PersistedDialogSnapshot> SnapshotOff;
	if (!PackPersistedDialogSnapshot(Fbb, Snapshot, SnapshotOff, OutError))
		return {};
	const auto Req = ::Tryll::CreateImportDialogRequest(
		Fbb,
		RequestId,
		AgentId,
		SnapshotOff,
		static_cast<::Tryll::DialogCompatibility>(Compatibility));
	return WrapMessage(Fbb, Req, ::Tryll::MessageBody_ImportDialogRequest);
}

TArray<uint8> BuildChangeAgentParamsRequest(
	uint64                RequestId,
	uint64                AgentId,
	const FString&        NodeName,
	UTryllNodeParamsBase* Params)
{
	checkf(Params != nullptr,
		TEXT("ChangeParams '%s' called with null params."), *NodeName);

	flatbuffers::FlatBufferBuilder Fbb;
	auto NodeOff = Fbb.CreateString(FStringToUtf8(NodeName));

	::Tryll::NodeParams::NodeParams ParamsType = ::Tryll::NodeParams::NodeParams_NONE;
	flatbuffers::Offset<void>       ParamsOffset = Params->Pack(Fbb, ParamsType);

	::Tryll::ChangeAgentParamRequestBuilder ReqBuilder{Fbb};
	ReqBuilder.add_request_id(RequestId);
	ReqBuilder.add_agent_id(AgentId);
	ReqBuilder.add_node_name(NodeOff);
	ReqBuilder.add_params_type(ParamsType);
	ReqBuilder.add_params(ParamsOffset);
	auto Req = ReqBuilder.Finish();
	return WrapMessage(Fbb, Req, ::Tryll::MessageBody_ChangeAgentParamRequest);
}

TArray<uint8> BuildAudioBuffer(uint64 VoiceInputId, const TArray<uint8>& Pcm)
{
	flatbuffers::FlatBufferBuilder Fbb;
	auto PcmVec = Fbb.CreateVector(Pcm.GetData(), static_cast<size_t>(Pcm.Num()));
	auto Buf = ::Tryll::CreateAudioBuffer(Fbb, VoiceInputId, PcmVec);
	return WrapMessage(Fbb, Buf, ::Tryll::MessageBody_AudioBuffer);
}

// ── Dual-mode StringStorage convenience wrappers ──────────────────────────────

TArray<uint8> BuildCreateStringStorageRequest(
	uint64 RequestId,
	const FString& Name,
	const TArray<FString>& Strings)
{
	// kind=List, no keys, no file_path
	return BuildCreateStringStorageRequest(
		RequestId, Name,
		::Tryll::StringStorageKind_List,
		Strings, /*Keys=*/TArray<FString>{}, /*FilePath=*/FString{});
}

TArray<uint8> BuildCreateKeyedStringStorageRequest(
	uint64                     RequestId,
	const FString&             Name,
	const TArray<FString>&     Keys,
	const TArray<FString>&     Values,
	::Tryll::StringStorageKind Kind)
{
	return BuildCreateStringStorageRequest(
		RequestId, Name,
		Kind,
		/*Strings=*/Values, Keys, /*FilePath=*/FString{});
}

TArray<uint8> BuildCreateStringStorageFromFileRequest(
	uint64 RequestId,
	const FString& Name,
	const FString& FilePath)
{
	// kind=List, no strings, no keys
	return BuildCreateStringStorageRequest(
		RequestId, Name,
		::Tryll::StringStorageKind_List,
		/*Strings=*/TArray<FString>{}, /*Keys=*/TArray<FString>{}, FilePath);
}

TArray<uint8> BuildCreateStringStorageFromFileRequest(
	uint64                     RequestId,
	const FString&             Name,
	const FString&             FilePath,
	::Tryll::StringStorageKind Kind)
{
	return BuildCreateStringStorageRequest(
		RequestId, Name,
		Kind,
		/*Strings=*/TArray<FString>{}, /*Keys=*/TArray<FString>{}, FilePath);
}

// ── Dual-mode EmbeddedStringStorage convenience wrappers ─────────────────────

TArray<uint8> BuildCreateEmbeddedStringStorageRequest(
	uint64         RequestId,
	const FString& Name,
	const FString& ConfigPath,
	const FString& EmbeddingModel)
{
	// Path A: config file; no inline strings.
	return BuildCreateEmbeddedStringStorageRequest(
		RequestId, Name, ConfigPath, /*Strings=*/TArray<FString>{}, EmbeddingModel);
}

TArray<uint8> BuildCreateEmbeddedStringStorageFromStringsRequest(
	uint64                 RequestId,
	const FString&         Name,
	const TArray<FString>& Strings,
	const FString&         EmbeddingModel)
{
	// Path B: inline strings; no config file.
	return BuildCreateEmbeddedStringStorageRequest(
		RequestId, Name, /*ConfigPath=*/FString{}, Strings, EmbeddingModel);
}

// ── VoiceInput flat-format convenience wrapper ────────────────────────────────

TArray<uint8> BuildCreateVoiceInputRequest(
	uint64         RequestId,
	const FString& ModelName,
	uint32         SampleRate,
	uint16         Channels,
	uint16         BitsPerSample,
	float          VadThreshold,
	uint32         VadMinSilenceMs,
	uint32         VadSpeechPadMs,
	const FString& HotwordsStoragePath,
	float          HotwordsScore)
{
	::Tryll::WireAudioFormatT InputFormat;
	InputFormat.sample_rate     = SampleRate;
	InputFormat.channels        = Channels;
	InputFormat.bits_per_sample = BitsPerSample;

	return BuildCreateVoiceInputRequest(
		RequestId, ModelName, InputFormat,
		VadThreshold, VadMinSilenceMs, VadSpeechPadMs,
		HotwordsStoragePath, HotwordsScore);
}

TArray<uint8>
BuildImportMutableKbRequest(
	uint64 RequestId,
	const FTryllMutableKbSnapshot& Snapshot,
	ETryllMutableKbCompatibility Compatibility,
	FTryllError& OutError)
{
	flatbuffers::FlatBufferBuilder Fbb;
	flatbuffers::Offset<::Tryll::PersistedMutableKb> SnapshotOff;
	if (!PackPersistedMutableKb(Fbb, Snapshot, SnapshotOff, OutError))
	{
		return {};
	}
	const auto Req = ::Tryll::CreateImportMutableKbRequest(
		Fbb,
		RequestId,
		SnapshotOff,
		static_cast<::Tryll::MutableKbCompatibility>(Compatibility));
	return WrapMessage(Fbb, Req, ::Tryll::MessageBody_ImportMutableKbRequest);
}

} // namespace Tryll::Client
