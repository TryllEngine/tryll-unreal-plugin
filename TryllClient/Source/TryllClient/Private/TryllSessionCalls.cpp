// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllSessionCalls.h"

#include "ClientCodec.h"
#include "TryllConnection.h"
#include "TryllConnectionEvents.h"
#include "TryllMutableKb.h"
#include "Generated/TryllCalls.gen.h"

namespace Tryll::Client {
namespace {

FTryllError NoConnection()
{
	return FTryllError(ETryllErrorCode::ConnectionFailed, TEXT("No connection"));
}

FTryllError NoConnectionCode0()
{
	return FTryllError{0u, TEXT("No connection")};
}

bool IsTransportError(const FTryllError& Error)
{
	return Error.Code == static_cast<int32>(ETryllErrorCode::ConnectionDropped)
		|| Error.Code == static_cast<int32>(ETryllErrorCode::ConnectionFailed);
}

// DownloadComplete can fail with code 0 (success=false in the payload). That is
// not IsOk for the caller, and it is not a transport drain.
void ReportUnlessTransport(
	bool bOk,
	const FTryllError& Error,
	FTryllConnectionEvents* Events,
	const TFunction<void(const FString&)>& LocalError)
{
	if (bOk || IsTransportError(Error))
	{
		return;
	}
	if (Events)
	{
		Events->OnError.Broadcast(Error.Message);
	}
	else if (LocalError)
	{
		LocalError(Error.Message);
	}
}

void RunOnGameThread(FTryllConnection* Connection, TUniqueFunction<void()> Fn)
{
	if (!Connection)
	{
		return;
	}
	if (IsInGameThread())
	{
		Fn();
	}
	else
	{
		Connection->PostToGameThread(MoveTemp(Fn));
	}
}

void ReportNoConnection(
	FTryllConnectionEvents* Events,
	const TFunction<void(const FString&)>& LocalError)
{
	if (Events)
	{
		Events->OnError.Broadcast(TEXT("No connection"));
	}
	else if (LocalError)
	{
		LocalError(TEXT("No connection"));
	}
}

void ReportAfter(
	const FTryllError& Error,
	FTryllConnectionEvents* Events,
	const TFunction<void(const FString&)>& LocalError)
{
	if (Error.IsOk() || IsTransportError(Error))
	{
		return;
	}
	if (Events)
	{
		Events->OnError.Broadcast(Error.Message);
	}
	else if (LocalError)
	{
		LocalError(Error.Message);
	}
}

TArray<FTryllModelInfo> ModelsFrom(const ::Tryll::ListModelsResponseT* Resp)
{
	TArray<FTryllModelInfo> Models;
	if (!Resp)
	{
		return Models;
	}
	Models.Reserve(static_cast<int32>(Resp->models.size()));
	for (const std::unique_ptr<::Tryll::ModelInfoT>& Model : Resp->models)
	{
		if (!Model)
		{
			continue;
		}
		FTryllModelInfo Info;
		Info.Name = UTF8_TO_TCHAR(Model->name.c_str());
		Info.Status = static_cast<ETryllModelStatus>(Model->status);
		Info.HuggingFaceRepo = UTF8_TO_TCHAR(Model->huggingface_repo.c_str());
		Info.SizeBytes = static_cast<int64>(Model->size_bytes);
		Info.ModelType = UTF8_TO_TCHAR(Model->model_type.c_str());
		Info.Engine = UTF8_TO_TCHAR(Model->engine.c_str());
		Info.Audience = UTF8_TO_TCHAR(Model->audience.c_str());
		Info.ToolCallFormat = UTF8_TO_TCHAR(Model->tool_call_format.c_str());
		Info.ToolCallSupport = UTF8_TO_TCHAR(Model->tool_call_support.c_str());
		Info.ContextSize = Model->context_size;
		Info.LocalPath = UTF8_TO_TCHAR(Model->local_path.c_str());
		for (const std::string& File : Model->files)
		{
			Info.Files.Add(UTF8_TO_TCHAR(File.c_str()));
		}
		for (const std::string& Lang : Model->languages)
		{
			Info.Languages.Add(UTF8_TO_TCHAR(Lang.c_str()));
		}
		if (const ::Tryll::ModelSamplingT* S = Model->default_sampling.get())
		{
			Info.bHasDefaultSampling = true;
			Info.DefaultSampling.Temperature      = S->temperature;
			Info.DefaultSampling.TopP             = S->top_p;
			Info.DefaultSampling.TopK             = S->top_k;
			Info.DefaultSampling.MinP             = S->min_p;
			Info.DefaultSampling.MaxTokens        = S->max_tokens;
			Info.DefaultSampling.Seed             = S->seed;
			Info.DefaultSampling.RepeatPenalty    = S->repeat_penalty;
			Info.DefaultSampling.PresencePenalty  = S->presence_penalty;
			Info.DefaultSampling.FrequencyPenalty = S->frequency_penalty;
		}
		for (const std::unique_ptr<::Tryll::ModelDependencyT>& D : Model->dependencies)
		{
			if (!D)
			{
				continue;
			}
			FTryllModelDependency Dep;
			Dep.Id            = UTF8_TO_TCHAR(D->id.c_str());
			Dep.Role          = UTF8_TO_TCHAR(D->role.c_str());
			Dep.Status        = static_cast<ETryllModelStatus>(D->status);
			Dep.SizeBytes     = static_cast<int64>(D->size_bytes);
			Dep.Folder        = UTF8_TO_TCHAR(D->folder.c_str());
			Dep.LocalPath     = UTF8_TO_TCHAR(D->local_path.c_str());
			Dep.FailureReason = UTF8_TO_TCHAR(D->failure_reason.c_str());
			Info.Dependencies.Add(MoveTemp(Dep));
		}
		if (const ::Tryll::TtsCapabilitiesInfoT* Caps = Model->tts_capabilities.get())
		{
			Info.bHasTtsCapabilities = true;
			FTryllTtsCapabilities& Out = Info.TtsCapabilities;
			Out.bVoiceDescription    = Caps->voice_description;
			Out.bDeliveryInstruction = Caps->delivery_instruction;
			Out.bReferenceVoice      = Caps->reference_voice;
			Out.bSpeed               = Caps->speed;
			Out.bLanguagePerCall     = Caps->language_per_call;
			Out.bExpressiveness      = Caps->expressiveness;
			Out.NumSpeakers          = Caps->num_speakers;
			for (const std::unique_ptr<::Tryll::TtsVoicePresetInfoT>& P : Caps->voice_presets)
			{
				if (!P)
				{
					continue;
				}
				FTryllTtsVoicePreset Preset;
				Preset.Id          = UTF8_TO_TCHAR(P->id.c_str());
				Preset.Label       = UTF8_TO_TCHAR(P->label.c_str());
				Preset.Description = UTF8_TO_TCHAR(P->description.c_str());
				Out.VoicePresets.Add(MoveTemp(Preset));
			}
			for (const std::unique_ptr<::Tryll::TtsInlineTagInfoT>& T : Caps->inline_tags)
			{
				if (!T)
				{
					continue;
				}
				FTryllTtsInlineTag Tag;
				Tag.Token        = UTF8_TO_TCHAR(T->token.c_str());
				Tag.Kind         = UTF8_TO_TCHAR(T->kind.c_str());
				Tag.Verification = UTF8_TO_TCHAR(T->verification.c_str());
				Out.InlineTags.Add(MoveTemp(Tag));
			}
		}
		Models.Add(MoveTemp(Info));
	}
	return Models;
}

TArray<FTryllMutableKbInfo> StoresFrom(const ::Tryll::EnumerateMutableKbsResponseT* Resp)
{
	TArray<FTryllMutableKbInfo> Stores;
	if (!Resp)
	{
		return Stores;
	}
	Stores.Reserve(static_cast<int32>(Resp->stores.size()));
	for (const std::unique_ptr<::Tryll::MutableKbInfoT>& Store : Resp->stores)
	{
		if (!Store)
		{
			continue;
		}
		FTryllMutableKbInfo Info;
		Info.Name = UTF8_TO_TCHAR(Store->name.c_str());
		Info.Schema = UTF8_TO_TCHAR(Store->schema.c_str());
		Info.LiveCount = Store->live_count;
		Info.WriteGeneration = Store->write_generation;
		Info.bMaterialized = Store->materialized;
		Info.BytesEstimate = Store->bytes_estimate;
		Stores.Add(MoveTemp(Info));
	}
	return Stores;
}

void FillEmbedded(
	UTryllSubsystem::FEmbeddedStorageInfo& Info,
	const ::Tryll::CreateEmbeddedStringStorageResponseT* Resp,
	const FString& Name)
{
	if (!Resp)
	{
		return;
	}
	Info.Name = Resp->name.empty() ? Name : UTF8_TO_TCHAR(Resp->name.c_str());
	Info.RecordCount = Resp->record_count;
	Info.EmbeddingDim = Resp->embedding_dim;
}

} // namespace

void FTryllSessionCalls::SetInferenceThrottle(FTryllConnection* Connection, float Level)
{
	if (Connection)
	{
		Calls::SetInferenceThrottle(*Connection, Level);
	}
}

void FTryllSessionCalls::SetSessionLocale(
	FTryllConnection* Connection,
	const void* Owner,
	const FString& Locale,
	TFunction<FTryllConnectionEvents*()> Events,
	TFunction<void(const FString&, bool)> LocalComplete,
	TFunction<void(const FString&)> LocalError)
{
	if (!Connection)
	{
		if (LocalComplete)
		{
			LocalComplete(Locale, false);
		}
		return;
	}
	Calls::SetSessionLocale(
		*Connection,
		Owner,
		Locale,
		[Locale, Events = MoveTemp(Events), LocalComplete = MoveTemp(LocalComplete), LocalError = MoveTemp(LocalError)](
			const FTryllError& Error)
		{
			FTryllConnectionEvents* Shared = Events ? Events() : nullptr;
			if (Shared)
			{
				Shared->OnSetSessionLocaleComplete.Broadcast(Locale, Error.IsOk());
			}
			else if (LocalComplete)
			{
				LocalComplete(Locale, Error.IsOk());
			}
			ReportAfter(Error, Shared, LocalError);
		},
		{});
}

void FTryllSessionCalls::ListModels(
	FTryllConnection* Connection,
	const void* Owner,
	TFunction<FTryllConnectionEvents*()> Events,
	TFunction<FTryllOnListModels> OnComplete,
	TFunction<void(const TArray<FTryllModelInfo>&, bool)> LocalComplete)
{
	if (!Connection)
	{
		if (OnComplete)
		{
			OnComplete({}, NoConnection());
		}
		return;
	}
	Calls::ListModels(
		*Connection,
		Owner,
		[OnComplete = MoveTemp(OnComplete), Events = MoveTemp(Events), LocalComplete = MoveTemp(LocalComplete)](
			TUniquePtr<::Tryll::ListModelsResponseT> Resp,
			const FTryllError& Error)
		{
			TArray<FTryllModelInfo> Models = ModelsFrom(Resp.Get());
			if (OnComplete)
			{
				OnComplete(Models, Error);
			}
			if (FTryllConnectionEvents* Shared = Events ? Events() : nullptr)
			{
				Shared->OnListModelsComplete.Broadcast(Models, Error.IsOk());
			}
			else if (LocalComplete)
			{
				LocalComplete(Models, Error.IsOk());
			}
		},
		{});
}

void FTryllSessionCalls::GetMemoryConsumption(
	FTryllConnection* Connection,
	const void* Owner,
	TFunction<FTryllOnGetMemoryConsumption> OnComplete,
	TFunction<void(const FTryllMemorySnapshot&, bool)> LocalComplete)
{
	if (!Connection)
	{
		if (OnComplete)
		{
			OnComplete({}, NoConnection());
		}
		return;
	}
	Calls::GetMemoryConsumption(
		*Connection,
		Owner,
		[OnComplete = MoveTemp(OnComplete), LocalComplete = MoveTemp(LocalComplete)](
			TUniquePtr<::Tryll::GetMemoryConsumptionResponseT> Resp,
			const FTryllError& Error)
		{
			const FTryllMemorySnapshot Snap = (Error.IsOk() && Resp)
				? FTryllMemorySnapshot::FromWire(*Resp)
				: FTryllMemorySnapshot{};
			if (OnComplete)
			{
				OnComplete(Snap, Error);
			}
			if (LocalComplete)
			{
				LocalComplete(Error.IsOk() ? Snap : FTryllMemorySnapshot{}, Error.IsOk());
			}
		},
		{});
}

void FTryllSessionCalls::EnumerateMutableKbs(
	FTryllConnection* Connection,
	const void* Owner,
	TFunction<FTryllOnEnumerateMutableKbs> OnComplete,
	TFunction<void(int32, bool)> LocalCount)
{
	if (!Connection)
	{
		if (OnComplete)
		{
			OnComplete({}, NoConnection());
		}
		return;
	}
	Calls::EnumerateMutableKbs(
		*Connection,
		Owner,
		[OnComplete = MoveTemp(OnComplete), LocalCount = MoveTemp(LocalCount)](
			TUniquePtr<::Tryll::EnumerateMutableKbsResponseT> Resp,
			const FTryllError& Error)
		{
			TArray<FTryllMutableKbInfo> Stores = Error.IsOk() ? StoresFrom(Resp.Get()) : TArray<FTryllMutableKbInfo>{};
			if (OnComplete)
			{
				OnComplete(Stores, Error);
			}
			if (LocalCount)
			{
				LocalCount(Stores.Num(), Error.IsOk());
			}
		},
		{});
}

void FTryllSessionCalls::SaveGameMemory(
	FTryllConnection* Connection,
	const void* Owner,
	const FString& Path,
	const FString& SaveId,
	TFunction<FTryllOnSaveGameMemory> OnComplete,
	TFunction<void(int32, bool)> LocalCount)
{
	if (!Connection)
	{
		if (OnComplete)
		{
			OnComplete({}, NoConnection());
		}
		return;
	}
	Calls::SaveGameMemory(
		*Connection,
		Owner,
		Path,
		SaveId,
		[OnComplete = MoveTemp(OnComplete), LocalCount = MoveTemp(LocalCount)](
			TUniquePtr<::Tryll::SaveGameMemoryResponseT> Resp,
			const FTryllError& Error)
		{
			FTryllGameMemorySaveResult Result;
			if (Error.IsOk() && Resp)
			{
				Result.SavedAtMs = Resp->saved_at_ms;
				Result.StoreCount = Resp->store_count;
			}
			if (OnComplete)
			{
				OnComplete(Result, Error);
			}
			if (LocalCount)
			{
				LocalCount(static_cast<int32>(Result.StoreCount), Error.IsOk());
			}
		},
		{});
}

void FTryllSessionCalls::LoadGameMemory(
	FTryllConnection* Connection,
	const void* Owner,
	const FString& Path,
	ETryllMutableKbEmbedTiming EmbedTiming,
	ETryllMutableKbCompatibility Compatibility,
	TFunction<FTryllOnLoadGameMemory> OnComplete,
	TFunction<void(int32, bool)> LocalCount)
{
	if (EmbedTiming != ETryllMutableKbEmbedTiming::Now && EmbedTiming != ETryllMutableKbEmbedTiming::Lazy)
	{
		if (OnComplete)
		{
			OnComplete({}, FTryllError(
				ETryllErrorCode::InvalidEmbedTiming,
				TEXT("LoadGameMemory requires embed_timing Now or Lazy")));
		}
		if (LocalCount)
		{
			LocalCount(0, false);
		}
		return;
	}
	if (!Connection)
	{
		if (OnComplete)
		{
			OnComplete({}, NoConnection());
		}
		return;
	}
	Calls::LoadGameMemory(
		*Connection,
		Owner,
		Path,
		static_cast<::Tryll::MutableKbCompatibility>(Compatibility),
		static_cast<::Tryll::MutableKbEmbedTiming>(EmbedTiming),
		[OnComplete = MoveTemp(OnComplete), LocalCount = MoveTemp(LocalCount)](
			TUniquePtr<::Tryll::LoadGameMemoryResponseT> Resp,
			const FTryllError& Error)
		{
			FTryllGameMemoryLoadResult Result;
			if (Error.IsOk() && Resp)
			{
				Result.StoreCount = Resp->store_count;
				Result.Warnings.Reserve(static_cast<int32>(Resp->warnings.size()));
				for (const std::string& Warning : Resp->warnings)
				{
					Result.Warnings.Add(UTF8_TO_TCHAR(Warning.c_str()));
				}
			}
			if (OnComplete)
			{
				OnComplete(Result, Error);
			}
			if (LocalCount)
			{
				LocalCount(static_cast<int32>(Result.StoreCount), Error.IsOk());
			}
		},
		{});
}

void FTryllSessionCalls::LoadModel(
	FTryllConnection* Connection,
	const void* Owner,
	const FString& ModelName,
	ETryllMemoryPlacement Placement,
	bool bIsOverride,
	TFunction<FTryllConnectionEvents*()> Events,
	TFunction<void(const FString&, bool)> LocalComplete,
	TFunction<void(const FString&)> LocalError)
{
	if (!Connection)
	{
		if (LocalComplete)
		{
			LocalComplete(ModelName, false);
		}
		if (LocalError)
		{
			ReportNoConnection(nullptr, LocalError);
		}
		return;
	}
	Calls::LoadModel(
		*Connection,
		Owner,
		ModelName,
		static_cast<::Tryll::MemoryPlacement>(Placement),
		bIsOverride,
		[ModelName, Events = MoveTemp(Events), LocalComplete = MoveTemp(LocalComplete), LocalError = MoveTemp(LocalError)](
			TUniquePtr<::Tryll::LoadModelResponseT>,
			const FTryllError& Error)
		{
			FTryllConnectionEvents* Shared = Events ? Events() : nullptr;
			if (Shared)
			{
				Shared->OnLoadModelComplete.Broadcast(ModelName, Error.IsOk());
			}
			else if (LocalComplete)
			{
				LocalComplete(ModelName, Error.IsOk());
			}
			ReportAfter(Error, Shared, LocalError);
		},
		{});
}

void FTryllSessionCalls::UnloadModel(
	FTryllConnection* Connection,
	const void* Owner,
	const FString& ModelName,
	TFunction<FTryllConnectionEvents*()> Events,
	TFunction<void(const FString&, bool)> LocalComplete,
	TFunction<void(const FString&)> LocalError)
{
	if (!Connection)
	{
		if (LocalComplete)
		{
			LocalComplete(ModelName, false);
		}
		if (LocalError)
		{
			ReportNoConnection(nullptr, LocalError);
		}
		return;
	}
	Calls::UnloadModel(
		*Connection,
		Owner,
		ModelName,
		[ModelName, Events = MoveTemp(Events), LocalComplete = MoveTemp(LocalComplete), LocalError = MoveTemp(LocalError)](
			const FTryllError& Error)
		{
			FTryllConnectionEvents* Shared = Events ? Events() : nullptr;
			if (Shared)
			{
				Shared->OnUnloadModelComplete.Broadcast(ModelName, Error.IsOk());
			}
			else if (LocalComplete)
			{
				LocalComplete(ModelName, Error.IsOk());
			}
			ReportAfter(Error, Shared, LocalError);
		},
		{});
}

void FTryllSessionCalls::DeleteModel(
	FTryllConnection* Connection,
	const void* Owner,
	const FString& ModelName,
	TFunction<void(const FString&, bool)> LocalComplete,
	TFunction<void(const FString&)> LocalError)
{
	if (!Connection)
	{
		if (LocalComplete)
		{
			LocalComplete(ModelName, false);
		}
		if (LocalError)
		{
			ReportNoConnection(nullptr, LocalError);
		}
		return;
	}
	Calls::DeleteModel(
		*Connection,
		Owner,
		ModelName,
		[ModelName, LocalComplete = MoveTemp(LocalComplete), LocalError = MoveTemp(LocalError)](const FTryllError& Error)
		{
			if (LocalComplete)
			{
				LocalComplete(ModelName, Error.IsOk());
			}
			ReportAfter(Error, nullptr, LocalError);
		},
		{});
}

void FTryllSessionCalls::CreateStringStorage(
	FTryllConnection* Connection,
	const void* Owner,
	const FString& Name,
	uint8 Kind,
	const TArray<FString>& Values,
	const TArray<FString>& Keys,
	const FString& FilePath,
	TFunction<UTryllSubsystem::FTryllOnStringStorage> OnComplete,
	TFunction<FTryllConnectionEvents*()> Events,
	TFunction<void(const FString&, bool)> LocalComplete,
	TFunction<void(const FString&)> LocalError)
{
	if (!Connection)
	{
		if (OnComplete)
		{
			OnComplete(NoConnectionCode0());
		}
		return;
	}
	Calls::CreateStringStorage(
		*Connection,
		Owner,
		Name,
		static_cast<::Tryll::StringStorageKind>(Kind),
		Values,
		Keys,
		FilePath,
		[Name, OnComplete = MoveTemp(OnComplete), Events = MoveTemp(Events), LocalComplete = MoveTemp(LocalComplete),
			LocalError = MoveTemp(LocalError)](TUniquePtr<::Tryll::CreateStringStorageResponseT>, const FTryllError& Error)
		{
			if (OnComplete)
			{
				OnComplete(Error);
			}
			FTryllConnectionEvents* Shared = Events ? Events() : nullptr;
			if (Shared)
			{
				Shared->OnCreateStringStorageComplete.Broadcast(Name, Error.IsOk());
			}
			else if (LocalComplete)
			{
				LocalComplete(Name, Error.IsOk());
			}
			ReportAfter(Error, Shared, LocalError);
		},
		{});
}

void FTryllSessionCalls::DestroyStringStorage(
	FTryllConnection* Connection,
	const void* Owner,
	const FString& Name,
	TFunction<UTryllSubsystem::FTryllOnStringStorage> OnComplete,
	TFunction<FTryllConnectionEvents*()> Events,
	TFunction<void(const FString&, bool)> LocalComplete,
	TFunction<void(const FString&)> LocalError)
{
	if (!Connection)
	{
		if (OnComplete)
		{
			OnComplete(NoConnectionCode0());
		}
		return;
	}
	Calls::DestroyStringStorage(
		*Connection,
		Owner,
		Name,
		[Name, OnComplete = MoveTemp(OnComplete), Events = MoveTemp(Events), LocalComplete = MoveTemp(LocalComplete),
			LocalError = MoveTemp(LocalError)](const FTryllError& Error)
		{
			if (OnComplete)
			{
				OnComplete(Error);
			}
			FTryllConnectionEvents* Shared = Events ? Events() : nullptr;
			if (Shared)
			{
				Shared->OnDestroyStringStorageComplete.Broadcast(Name, Error.IsOk());
			}
			else if (LocalComplete)
			{
				LocalComplete(Name, Error.IsOk());
			}
			ReportAfter(Error, Shared, LocalError);
		},
		{});
}

void FTryllSessionCalls::CreateEmbeddedStringStorage(
	FTryllConnection* Connection,
	const void* Owner,
	const FString& Name,
	const FString& ConfigPath,
	const TArray<FString>& Strings,
	const FString& EmbeddingModel,
	TFunction<UTryllSubsystem::FTryllOnEmbeddedStorageCreated> OnComplete,
	TFunction<FTryllConnectionEvents*()> Events,
	TFunction<void(const FString&, int32, bool)> LocalComplete,
	TFunction<void(const FString&)> LocalError)
{
	if (!Connection)
	{
		if (OnComplete)
		{
			OnComplete({}, NoConnectionCode0());
		}
		return;
	}
	Calls::CreateEmbeddedStringStorage(
		*Connection,
		Owner,
		Name,
		ConfigPath,
		Strings,
		EmbeddingModel,
		[Name, OnComplete = MoveTemp(OnComplete), Events = MoveTemp(Events), LocalComplete = MoveTemp(LocalComplete),
			LocalError = MoveTemp(LocalError)](
			TUniquePtr<::Tryll::CreateEmbeddedStringStorageResponseT> Resp,
			const FTryllError& Error)
		{
			UTryllSubsystem::FEmbeddedStorageInfo Info;
			if (Error.IsOk())
			{
				FillEmbedded(Info, Resp.Get(), Name);
			}
			if (OnComplete)
			{
				OnComplete(Info, Error);
			}
			const FString StorageName = Info.Name.IsEmpty() ? Name : Info.Name;
			FTryllConnectionEvents* Shared = Events ? Events() : nullptr;
			if (Shared)
			{
				Shared->OnCreateEmbeddedStringStorageComplete.Broadcast(
					StorageName, static_cast<int32>(Info.RecordCount), Error.IsOk());
			}
			else if (LocalComplete)
			{
				LocalComplete(StorageName, static_cast<int32>(Info.RecordCount), Error.IsOk());
			}
			ReportAfter(Error, Shared, LocalError);
		},
		{});
}

void FTryllSessionCalls::DestroyEmbeddedStringStorage(
	FTryllConnection* Connection,
	const void* Owner,
	const FString& Name,
	TFunction<UTryllSubsystem::FTryllOnEmbeddedStorageDestroyed> OnComplete,
	TFunction<FTryllConnectionEvents*()> Events,
	TFunction<void(const FString&, bool)> LocalComplete,
	TFunction<void(const FString&)> LocalError)
{
	if (!Connection)
	{
		if (OnComplete)
		{
			OnComplete(NoConnectionCode0());
		}
		return;
	}
	Calls::DestroyEmbeddedStringStorage(
		*Connection,
		Owner,
		Name,
		[Name, OnComplete = MoveTemp(OnComplete), Events = MoveTemp(Events), LocalComplete = MoveTemp(LocalComplete),
			LocalError = MoveTemp(LocalError)](const FTryllError& Error)
		{
			if (OnComplete)
			{
				OnComplete(Error);
			}
			FTryllConnectionEvents* Shared = Events ? Events() : nullptr;
			if (Shared)
			{
				Shared->OnDestroyEmbeddedStringStorageComplete.Broadcast(Name, Error.IsOk());
			}
			else if (LocalComplete)
			{
				LocalComplete(Name, Error.IsOk());
			}
			ReportAfter(Error, Shared, LocalError);
		},
		{});
}

void FTryllSessionCalls::DestroyAgent(
	FTryllConnection* Connection,
	const void* Owner,
	uint64 AgentId,
	TFunction<FTryllConnectionEvents*()> Events,
	TFunction<void(uint64)> OnDestroyed,
	TFunction<void(int64)> LocalDestroyed,
	TFunction<void(const FString&)> LocalError)
{
	if (!Connection || AgentId == 0)
	{
		return;
	}
	Calls::DestroyAgent(
		*Connection,
		Owner,
		AgentId,
		[AgentId, Events = MoveTemp(Events), OnDestroyed = MoveTemp(OnDestroyed),
			LocalDestroyed = MoveTemp(LocalDestroyed), LocalError = MoveTemp(LocalError)](
			const FTryllError& Error)
		{
			FTryllConnectionEvents* Shared = Events ? Events() : nullptr;
			if (Error.IsOk())
			{
				if (OnDestroyed)
				{
					OnDestroyed(AgentId);
				}
				if (Shared)
				{
					Shared->OnAgentDestroyed.Broadcast(AgentId);
				}
				else if (LocalDestroyed)
				{
					LocalDestroyed(static_cast<int64>(AgentId));
				}
			}
			ReportAfter(Error, Shared, LocalError);
		},
		{});
}

void FTryllSessionCalls::DestroyVoiceInput(
	FTryllConnection* Connection,
	const void* Owner,
	uint64 VoiceInputId,
	TFunction<FTryllConnectionEvents*()> Events,
	TFunction<void(uint64)> OnDestroyed,
	TFunction<void(const FString&)> LocalError)
{
	if (!Connection || VoiceInputId == 0)
	{
		return;
	}
	Calls::DestroyVoiceInput(
		*Connection,
		Owner,
		VoiceInputId,
		[VoiceInputId, Events = MoveTemp(Events), OnDestroyed = MoveTemp(OnDestroyed),
			LocalError = MoveTemp(LocalError)](const FTryllError& Error)
		{
			FTryllConnectionEvents* Shared = Events ? Events() : nullptr;
			if (Error.IsOk() && OnDestroyed)
			{
				OnDestroyed(VoiceInputId);
			}
			ReportAfter(Error, Shared, LocalError);
		},
		{});
}

void FTryllSessionCalls::DownloadModel(
	FTryllConnection* Connection,
	const void* Owner,
	const FString& ModelName,
	TFunction<FTryllConnectionEvents*()> Events,
	TFunction<void(const FString&, uint64, uint64, float)> LocalProgress,
	TFunction<void(const FString&, bool, const FString&)> LocalComplete,
	TFunction<void(const FString&)> LocalError)
{
	if (!Connection)
	{
		if (LocalError)
		{
			ReportNoConnection(nullptr, LocalError);
		}
		return;
	}
	const uint64 RequestId = Connection->AllocateRequestId();
	TWeakPtr<FTryllConnection> WeakConn = Connection->AsShared();
	Connection->Call(
		RequestId,
		BuildDownloadModelRequest(RequestId, ModelName),
		Owner,
		[WeakConn, ModelName, Events = MoveTemp(Events), LocalProgress = MoveTemp(LocalProgress),
			LocalComplete = MoveTemp(LocalComplete), LocalError = MoveTemp(LocalError)](
			const ::Tryll::Message* Msg, const FTryllErrorContext& Err) -> bool
		{
			const auto Body = Msg ? Msg->body_type() : ::Tryll::MessageBody_NONE;
			if (Body == ::Tryll::MessageBody_DownloadProgress)
			{
				FString Name = ModelName;
				uint64 Bytes = 0;
				uint64 Total = 0;
				float Percent = 0.f;
				if (const ::Tryll::DownloadProgress* Prog = Msg->body_as_DownloadProgress())
				{
					if (Prog->model_name())
					{
						Name = UTF8_TO_TCHAR(Prog->model_name()->c_str());
					}
					Bytes = Prog->bytes_downloaded();
					Total = Prog->total_bytes();
					Percent = Prog->percent();
				}
				TSharedPtr<FTryllConnection> Live = WeakConn.Pin();
				RunOnGameThread(Live.Get(), [Events, LocalProgress, Name, Bytes, Total, Percent]()
				{
					FTryllConnectionEvents* Shared = Events ? Events() : nullptr;
					if (Shared)
					{
						Shared->OnDownloadProgress.Broadcast(Name, Bytes, Total, Percent);
					}
					else if (LocalProgress)
					{
						LocalProgress(Name, Bytes, Total, Percent);
					}
				});
				return false;
			}

			bool bOk = Err.IsEmpty() && Body == ::Tryll::MessageBody_DownloadComplete;
			FString Name = ModelName;
			FString Message = Err.Message;
			uint32 Code = Err.Code;
			if (const ::Tryll::DownloadComplete* Done = (Msg && Body == ::Tryll::MessageBody_DownloadComplete)
				? Msg->body_as_DownloadComplete() : nullptr)
			{
				if (Done->model_name())
				{
					Name = UTF8_TO_TCHAR(Done->model_name()->c_str());
				}
				if (!Done->success())
				{
					bOk = false;
					if (Done->error())
					{
						Message = UTF8_TO_TCHAR(Done->error()->c_str());
					}
				}
			}
			const FTryllError Reported(Code, Message);
			TSharedPtr<FTryllConnection> Live = WeakConn.Pin();
			RunOnGameThread(Live.Get(), [Events, LocalComplete, LocalError, Name, bOk, Reported]()
			{
				FTryllConnectionEvents* Shared = Events ? Events() : nullptr;
				if (Shared)
				{
					Shared->OnDownloadComplete.Broadcast(Name, bOk, Reported.Message);
				}
				else if (LocalComplete)
				{
					LocalComplete(Name, bOk, Reported.Message);
				}
				ReportUnlessTransport(bOk, Reported, Shared, LocalError);
			});
			return true;
		},
		::Tryll::MessageBody_DownloadModelRequest,
		{});
}

void FTryllSessionCalls::EnsureMutableKb(
	FTryllConnection* Connection,
	const void* Owner,
	const FString& Name,
	const FString& Schema,
	TFunction<FTryllOnEnsureMutableKb> OnComplete,
	TFunction<TSharedPtr<FTryllConnection>()> LiveConnection,
	TFunction<void(const TSharedPtr<FTryllMutableKb>&)> OnStore,
	TFunction<void(const FString&, bool)> LocalComplete)
{
	if (!Connection)
	{
		if (OnComplete)
		{
			OnComplete(nullptr, NoConnection());
		}
		return;
	}
	Calls::EnsureMutableKb(
		*Connection,
		Owner,
		Name,
		Schema,
		[Name, Schema, OnComplete = MoveTemp(OnComplete), LiveConnection = MoveTemp(LiveConnection),
			OnStore = MoveTemp(OnStore), LocalComplete = MoveTemp(LocalComplete)](
			TUniquePtr<::Tryll::EnsureMutableKbResponseT> Resp,
			const FTryllError& Error)
		{
			TSharedPtr<FTryllConnection> Live = LiveConnection ? LiveConnection() : nullptr;
			if (!Error.IsOk() || !Resp || !Live)
			{
				if (OnComplete)
				{
					OnComplete(nullptr, Error);
				}
				if (LocalComplete)
				{
					LocalComplete(Name, false);
				}
				return;
			}
			const FString StoreName = Resp->name.empty() ? Name : FString(UTF8_TO_TCHAR(Resp->name.c_str()));
			const FString StoreSchema = Resp->schema.empty() ? Schema : FString(UTF8_TO_TCHAR(Resp->schema.c_str()));
			TSharedPtr<FTryllMutableKb> Store = MakeShared<FTryllMutableKb>(Live, StoreName, StoreSchema);
			if (OnStore)
			{
				OnStore(Store);
			}
			if (OnComplete)
			{
				OnComplete(Store, Error);
			}
			if (LocalComplete)
			{
				LocalComplete(StoreName, true);
			}
		},
		{});
}

}
