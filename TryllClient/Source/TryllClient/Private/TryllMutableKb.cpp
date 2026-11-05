// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllMutableKb.h"
#include "ClientCodec.h"
#include "TryllConnection.h"
#include "TryllObservationTypes.h"
#include "Generated/TryllCalls.gen.h"
#include "TryllWeakCapture.h"

#include <memory>

namespace
{
	void NoteMutableKb(FTryllConnection& Conn, uint64 RequestId, ::Tryll::MessageBody Body)
	{
		FTryllObservationNote Note;
		Note.Direction = ETryllObservationDirection::Outbound;
		Note.Body = Body;
		Note.RequestBody = Body;
		Note.RequestId = RequestId;
		Conn.GetObserverHub().AnnotateNote(Note);
	}
}

FTryllMutableKb::FTryllMutableKb(
	TSharedPtr<FTryllConnection> InConnection, FString InName, FString InSchema)
	: Connection(MoveTemp(InConnection))
	, Name(MoveTemp(InName))
	, Schema(MoveTemp(InSchema))
{
}

bool FTryllMutableKb::IsValid() const
{
	return Connection.IsValid() && !Name.IsEmpty();
}

void FTryllMutableKb::Write(
	const TArray<FTryllMutableKbUpsert>& Records,
	TFunction<void(const TArray<FTryllMutableKbWriteResult>&, const FTryllError&)> OnComplete)
{
	if (!IsValid())
	{
		if (OnComplete)
		{
			OnComplete({}, FTryllError(ETryllErrorCode::UnknownMutableKb, TEXT("MutableKb handle is not bound to a session")));
		}
		return;
	}
	TArray<::Tryll::MutableKbUpsertT> Wire;
	Wire.Reserve(Records.Num());
	for (const FTryllMutableKbUpsert& Record : Records)
	{
		::Tryll::MutableKbUpsertT Upsert;
		Upsert.id = TCHAR_TO_UTF8(*Record.Id);
		Upsert.text = TCHAR_TO_UTF8(*Record.Text);
		Wire.Add(MoveTemp(Upsert));
	}
	const auto Weak = CaptureTryllLive(this);
	Tryll::Client::Calls::MutableKbWrite(
		*Connection, this, Name, Schema, Wire,
		[OnComplete = MoveTemp(OnComplete)](TUniquePtr<::Tryll::MutableKbWriteResponseT> Resp, const FTryllError& Error)
		{
			TArray<FTryllMutableKbWriteResult> Results;
			if (Resp)
			{
				Results.Reserve(static_cast<int32>(Resp->results.size()));
				for (const std::unique_ptr<::Tryll::MutableKbWriteResultT>& Item : Resp->results)
				{
					if (!Item)
					{
						continue;
					}
					FTryllMutableKbWriteResult Result;
					Result.Id = UTF8_TO_TCHAR(Item->id.c_str());
					Result.bInserted = Item->inserted;
					Result.bAccepted = Item->accepted;
					Result.Error = UTF8_TO_TCHAR(Item->error.c_str());
					Results.Add(MoveTemp(Result));
				}
			}
			if (OnComplete)
			{
				OnComplete(Results, Error);
			}
		},
		[Weak](uint64 RequestId)
		{
			if (auto Live = Weak.Pin(); Live && Live->Connection.IsValid())
			{
				NoteMutableKb(*Live->Connection, RequestId, ::Tryll::MessageBody_MutableKbWriteRequest);
			}
		});
}

void FTryllMutableKb::Delete(
	const TArray<FString>& Ids,
	TFunction<void(uint32 DeletedCount, const FTryllError&)> OnComplete)
{
	if (!IsValid())
	{
		if (OnComplete)
		{
			OnComplete(0, FTryllError(ETryllErrorCode::UnknownMutableKb, TEXT("MutableKb handle is not bound to a session")));
		}
		return;
	}
	const auto Weak = CaptureTryllLive(this);
	Tryll::Client::Calls::MutableKbDelete(
		*Connection, this, Name, Ids,
		[OnComplete = MoveTemp(OnComplete)](TUniquePtr<::Tryll::MutableKbDeleteResponseT> Resp, const FTryllError& Error)
		{
			if (OnComplete)
			{
				OnComplete(Resp ? Resp->deleted_count : 0, Error);
			}
		},
		[Weak](uint64 RequestId)
		{
			if (auto Live = Weak.Pin(); Live && Live->Connection.IsValid())
			{
				NoteMutableKb(*Live->Connection, RequestId, ::Tryll::MessageBody_MutableKbDeleteRequest);
			}
		});
}

void FTryllMutableKb::Clear(TFunction<void(uint32 ClearedCount, const FTryllError&)> OnComplete)
{
	if (!IsValid())
	{
		if (OnComplete)
		{
			OnComplete(0, FTryllError(ETryllErrorCode::UnknownMutableKb, TEXT("MutableKb handle is not bound to a session")));
		}
		return;
	}
	const auto Weak = CaptureTryllLive(this);
	Tryll::Client::Calls::MutableKbClear(
		*Connection, this, Name,
		[OnComplete = MoveTemp(OnComplete)](TUniquePtr<::Tryll::MutableKbClearResponseT> Resp, const FTryllError& Error)
		{
			if (OnComplete)
			{
				OnComplete(Resp ? Resp->cleared_count : 0, Error);
			}
		},
		[Weak](uint64 RequestId)
		{
			if (auto Live = Weak.Pin(); Live && Live->Connection.IsValid())
			{
				NoteMutableKb(*Live->Connection, RequestId, ::Tryll::MessageBody_MutableKbClearRequest);
			}
		});
}

void FTryllMutableKb::Query(
	const FString& QueryText,
	uint32 TopK,
	TFunction<void(const TArray<FTryllMutableKbHit>&, const FTryllError&)> OnComplete)
{
	Query(QueryText, TopK, ETryllRetrievalMode::Dense, MoveTemp(OnComplete));
}

void FTryllMutableKb::Query(
	const FString& QueryText,
	uint32 TopK,
	ETryllRetrievalMode Mode,
	TFunction<void(const TArray<FTryllMutableKbHit>&, const FTryllError&)> OnComplete)
{
	if (!IsValid())
	{
		if (OnComplete)
		{
			OnComplete({}, FTryllError(ETryllErrorCode::UnknownMutableKb, TEXT("MutableKb handle is not bound to a session")));
		}
		return;
	}
	const auto Weak = CaptureTryllLive(this);
	Tryll::Client::Calls::MutableKbQuery(
		*Connection, this, Name, QueryText, TopK, static_cast<::Tryll::RetrievalMode>(Mode),
		[OnComplete = MoveTemp(OnComplete)](TUniquePtr<::Tryll::MutableKbQueryResponseT> Resp, const FTryllError& Error)
		{
			TArray<FTryllMutableKbHit> Hits;
			if (Resp)
			{
				Hits.Reserve(static_cast<int32>(Resp->hits.size()));
				for (const std::unique_ptr<::Tryll::MutableKbHitT>& Item : Resp->hits)
				{
					if (!Item)
					{
						continue;
					}
					FTryllMutableKbHit Hit;
					Hit.Id = UTF8_TO_TCHAR(Item->id.c_str());
					Hit.Text = UTF8_TO_TCHAR(Item->text.c_str());
					Hit.Distance = Item->distance;
					Hits.Add(MoveTemp(Hit));
				}
			}
			if (OnComplete)
			{
				OnComplete(Hits, Error);
			}
		},
		[Weak](uint64 RequestId)
		{
			if (auto Live = Weak.Pin(); Live && Live->Connection.IsValid())
			{
				NoteMutableKb(*Live->Connection, RequestId, ::Tryll::MessageBody_MutableKbQueryRequest);
			}
		});
}

void FTryllMutableKb::Get(
	const FString& Id,
	TFunction<void(bool bFound, const FTryllMutableKbRecord&, const FTryllError&)> OnComplete)
{
	if (!IsValid())
	{
		if (OnComplete)
		{
			OnComplete(false, {}, FTryllError(ETryllErrorCode::UnknownMutableKb, TEXT("MutableKb handle is not bound to a session")));
		}
		return;
	}
	const auto Weak = CaptureTryllLive(this);
	Tryll::Client::Calls::MutableKbGet(
		*Connection, this, Name, Id,
		[OnComplete = MoveTemp(OnComplete)](TUniquePtr<::Tryll::MutableKbGetResponseT> Resp, const FTryllError& Error)
		{
			FTryllMutableKbRecord Record;
			const bool bFound = Resp && Resp->found;
			if (bFound)
			{
				Record.Id = UTF8_TO_TCHAR(Resp->id.c_str());
				Record.Text = UTF8_TO_TCHAR(Resp->text.c_str());
				Record.CreatedAtMs = Resp->created_at_ms;
				Record.UpdatedAtMs = Resp->updated_at_ms;
			}
			if (OnComplete)
			{
				OnComplete(bFound, Record, Error);
			}
		},
		[Weak](uint64 RequestId)
		{
			if (auto Live = Weak.Pin(); Live && Live->Connection.IsValid())
			{
				NoteMutableKb(*Live->Connection, RequestId, ::Tryll::MessageBody_MutableKbGetRequest);
			}
		});
}

void FTryllMutableKb::List(
	uint32 Offset,
	uint32 Limit,
	TFunction<void(const FTryllMutableKbListPage&, const FTryllError&)> OnComplete)
{
	if (!IsValid())
	{
		if (OnComplete)
		{
			OnComplete({}, FTryllError(ETryllErrorCode::UnknownMutableKb, TEXT("MutableKb handle is not bound to a session")));
		}
		return;
	}
	const auto Weak = CaptureTryllLive(this);
	Tryll::Client::Calls::MutableKbList(
		*Connection, this, Name, Offset, Limit,
		[OnComplete = MoveTemp(OnComplete)](TUniquePtr<::Tryll::MutableKbListResponseT> Resp, const FTryllError& Error)
		{
			FTryllMutableKbListPage Page;
			if (Resp)
			{
				Page.NextOffset = Resp->next_offset;
				Page.bHasMore = Resp->has_more;
				Page.Records.Reserve(static_cast<int32>(Resp->records.size()));
				for (const std::unique_ptr<::Tryll::MutableKbRecordT>& Item : Resp->records)
				{
					if (!Item)
					{
						continue;
					}
					FTryllMutableKbRecord Record;
					Record.Id = UTF8_TO_TCHAR(Item->id.c_str());
					Record.Text = UTF8_TO_TCHAR(Item->text.c_str());
					Record.CreatedAtMs = Item->created_at_ms;
					Record.UpdatedAtMs = Item->updated_at_ms;
					Page.Records.Add(MoveTemp(Record));
				}
			}
			if (OnComplete)
			{
				OnComplete(Page, Error);
			}
		},
		[Weak](uint64 RequestId)
		{
			if (auto Live = Weak.Pin(); Live && Live->Connection.IsValid())
			{
				NoteMutableKb(*Live->Connection, RequestId, ::Tryll::MessageBody_MutableKbListRequest);
			}
		});
}

void FTryllMutableKb::Stats(TFunction<void(const FTryllMutableKbStats&, const FTryllError&)> OnComplete)
{
	if (!IsValid())
	{
		if (OnComplete)
		{
			OnComplete({}, FTryllError(ETryllErrorCode::UnknownMutableKb, TEXT("MutableKb handle is not bound to a session")));
		}
		return;
	}
	const auto Weak = CaptureTryllLive(this);
	Tryll::Client::Calls::MutableKbStats(
		*Connection, this, Name,
		[OnComplete = MoveTemp(OnComplete)](TUniquePtr<::Tryll::MutableKbStatsResponseT> Resp, const FTryllError& Error)
		{
			FTryllMutableKbStats StatsResult;
			if (Resp)
			{
				StatsResult.LiveCount = Resp->live_count;
				StatsResult.TombstonedCount = Resp->tombstoned_count;
				StatsResult.SlotCount = Resp->slot_count;
				StatsResult.WriteGeneration = Resp->write_generation;
				StatsResult.BytesEstimate = Resp->bytes_estimate;
				StatsResult.bMaterialized = Resp->materialized;
			}
			if (OnComplete)
			{
				OnComplete(StatsResult, Error);
			}
		},
		[Weak](uint64 RequestId)
		{
			if (auto Live = Weak.Pin(); Live && Live->Connection.IsValid())
			{
				NoteMutableKb(*Live->Connection, RequestId, ::Tryll::MessageBody_MutableKbStatsRequest);
			}
		});
}

void FTryllMutableKb::Export(TFunction<void(const FTryllMutableKbSnapshot&, const FTryllError&)> OnComplete)
{
	if (!IsValid())
	{
		if (OnComplete)
		{
			OnComplete({}, FTryllError(ETryllErrorCode::UnknownMutableKb, TEXT("MutableKb handle is not bound to a session")));
		}
		return;
	}
	const auto Weak = CaptureTryllLive(this);
	const uint64 Rid = Connection->AllocateRequestId();
	Connection->Call(
		Rid,
		Tryll::Client::BuildExportMutableKbRequest(Rid, Name),
		this,
		[Weak, OnComplete = MoveTemp(OnComplete)](const ::Tryll::Message* Msg, const FTryllErrorContext& Err) mutable -> bool
		{
			FTryllMutableKbSnapshot Snapshot;
			FTryllError Error;
			if (!Err.IsEmpty())
			{
				Error = FTryllError(Err.Code, Err.Message);
			}
			else if (Msg && Msg->body_type() == ::Tryll::MessageBody_ExportMutableKbResponse)
			{
				const auto* Resp = Msg->body_as<::Tryll::ExportMutableKbResponse>();
				if (!Resp || !Resp->snapshot()
					|| !Tryll::Client::UnpackPersistedMutableKb(*Resp->snapshot(), Snapshot, Error))
				{
					if (Error.IsOk())
					{
						Error = FTryllError(ETryllErrorCode::MalformedMessage, TEXT("ExportMutableKbResponse missing snapshot"));
					}
				}
			}
			else
			{
				Error = FTryllError(ETryllErrorCode::MalformedMessage, TEXT("unexpected ExportMutableKb body"));
			}
			auto Run = [OnComplete = MoveTemp(OnComplete), Snapshot = MoveTemp(Snapshot), Error]() mutable
			{
				if (OnComplete)
				{
					OnComplete(Snapshot, Error);
				}
			};
			if (IsInGameThread())
			{
				Run();
			}
			else if (auto Live = Weak.Pin(); Live && Live->Connection.IsValid())
			{
				Live->Connection->PostToGameThread(MoveTemp(Run));
			}
			return true;
		},
		::Tryll::MessageBody_ExportMutableKbRequest,
		[Weak](uint64 RequestId)
		{
			if (auto Live = Weak.Pin(); Live && Live->Connection.IsValid())
			{
				NoteMutableKb(*Live->Connection, RequestId, ::Tryll::MessageBody_ExportMutableKbRequest);
			}
		});
}

void FTryllMutableKb::Import(
	const FTryllMutableKbSnapshot& Snapshot,
	ETryllMutableKbCompatibility Compatibility,
	TFunction<void(const FTryllMutableKbImportResult&, const FTryllError&)> OnComplete)
{
	if (!IsValid())
	{
		if (OnComplete)
		{
			OnComplete({}, FTryllError(ETryllErrorCode::UnknownMutableKb, TEXT("MutableKb handle is not bound to a session")));
		}
		return;
	}
	if (!Snapshot.Name.Equals(Name, ESearchCase::CaseSensitive))
	{
		if (OnComplete)
		{
			OnComplete({}, FTryllError(
				ETryllErrorCode::MutableKbSnapshotInvalid,
				FString::Printf(TEXT("snapshot.name must match store name '%s'"), *Name)));
		}
		return;
	}
	const auto Weak = CaptureTryllLive(this);
	Tryll::Client::Calls::ImportMutableKb(
		*Connection,
		this,
		[Snapshot, Compatibility](uint64 RequestId)
		{
			FTryllError EncError;
			return Tryll::Client::BuildImportMutableKbRequest(RequestId, Snapshot, Compatibility, EncError);
		},
		[OnComplete = MoveTemp(OnComplete)](TUniquePtr<::Tryll::ImportMutableKbResponseT> Resp, const FTryllError& Error)
		{
			FTryllMutableKbImportResult Result;
			if (Resp)
			{
				Result.Name = UTF8_TO_TCHAR(Resp->name.c_str());
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
		},
		[Weak](uint64 RequestId)
		{
			if (auto Live = Weak.Pin(); Live && Live->Connection.IsValid())
			{
				NoteMutableKb(*Live->Connection, RequestId, ::Tryll::MessageBody_ImportMutableKbRequest);
			}
		});
}

bool FTryllMutableKb::TryHandleEvent(const FTryllConnEvent& /*Event*/)
{
	return false;
}

void FTryllMutableKb::DrainPendingCallbacks(const FTryllError& TerminalError)
{
	if (Connection.IsValid())
	{
		Connection->FailOwnedBy(this, TerminalError);
	}
	Connection.Reset();
}
