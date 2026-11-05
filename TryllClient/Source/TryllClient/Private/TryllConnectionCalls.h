// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.
//
// Hand-written unary/ack helpers the generated stubs call. A handler copies
// the reply out of the receive buffer (UnPack) and posts the completion.
// On the game thread the completion runs before the handler returns, so
// FailOwnedBy's callers see it before FailOwnedBy returns.

#pragma once

#include "TryllConnection.h"
#include "TryllObservationTypes.h"
#include "TryllUtf8.h"

#include "messages_generated.h"

namespace Tryll::Client {

template<typename TTable, typename TNative>
void CallUnary(
	FTryllConnection& Conn,
	const void* Owner,
	uint64 RequestId,
	TArray<uint8> Frame,
	::Tryll::MessageBody RequestBody,
	::Tryll::MessageBody Expected,
	TUniqueFunction<void(TUniquePtr<TNative>, const FTryllError&)> Done,
	TUniqueFunction<void(uint64)> Note,
	bool bBusyIsExpected = false)
{
	// The inbound frame is journaled by the tap. bBusyIsExpected used to hide a
	// second slot row; that row is no longer written.
	(void)bBusyIsExpected;
	if (Frame.Num() == 0)
	{
		Conn.PostToGameThread([Done = MoveTemp(Done)]() mutable
		{
			if (Done)
			{
				Done(nullptr, FTryllError(ETryllErrorCode::MalformedMessage, TEXT("empty frame")));
			}
		});
		return;
	}

	FTryllConnection* ConnPtr = &Conn;
	const uint64 NotedRequestId = RequestId;
	const ::Tryll::MessageBody NotedRequest = RequestBody;
	Conn.Call(
		RequestId,
		MoveTemp(Frame),
		Owner,
		[ConnPtr, Expected, NotedRequestId, NotedRequest, Done = MoveTemp(Done)](const ::Tryll::Message* Msg, const FTryllErrorContext& Err) mutable -> bool
		{
			TUniquePtr<TNative> Value;
			FTryllError Error;
			if (!Err.IsEmpty())
			{
				Error = FTryllError(Err.Code, Err.Message);
			}
			else if (Msg && Msg->body_type() == ::Tryll::MessageBody_ErrorResponse)
			{
				const auto* Body = Msg->body_as<::Tryll::ErrorResponse>();
				const uint32 Code = Body ? Body->code() : static_cast<uint32>(ETryllErrorCode::MalformedMessage);
				Error = FTryllError(Code, Body ? Utf8ToFString(Body->message()) : FString(TEXT("unexpected reply")));
			}
			else if (Msg && Msg->body_type() == Expected)
			{
				const auto* Body = Msg->body_as<TTable>();
				if (Body)
				{
					Value.Reset(Body->UnPack());
				}
				if (!Value)
				{
					Error = FTryllError(ETryllErrorCode::MalformedMessage, TEXT("unexpected reply"));
				}
			}
			else
			{
				Error = FTryllError(ETryllErrorCode::MalformedMessage, TEXT("unexpected reply"));
				if (ConnPtr)
				{
					ConnPtr->GetObserverHub().ObserveLocal(
						FTryllClientObserverHub::MakeError(
							static_cast<uint32>(ETryllErrorCode::MalformedMessage),
							TEXT("MalformedMessage"),
							TEXT("unexpected reply"),
							::Tryll::LogErrorOrigin_unexpected_reply),
						NotedRequestId,
						0,
						true,
						TEXT("unexpected reply"),
						static_cast<uint32>(ETryllErrorCode::MalformedMessage));
				}
			}

			auto Run = [Done = MoveTemp(Done), Value = MoveTemp(Value), Error]() mutable
			{
				// The inbound tap already journaled this frame. A second Observe
				// would export an empty payload beside it.
				if (Done)
				{
					Done(MoveTemp(Value), Error);
				}
			};
			if (IsInGameThread())
			{
				Run();
			}
			else if (ConnPtr)
			{
				ConnPtr->PostToGameThread(MoveTemp(Run));
			}
			return true;
		},
		RequestBody,
		MoveTemp(Note));
}

inline void CallAck(
	FTryllConnection& Conn,
	const void* Owner,
	uint64 RequestId,
	TArray<uint8> Frame,
	::Tryll::MessageBody RequestBody,
	TUniqueFunction<void(const FTryllError&)> Done,
	TUniqueFunction<void(uint64)> Note,
	bool bBusyIsExpected = false)
{
	CallUnary<::Tryll::Ack, ::Tryll::AckT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		RequestBody,
		::Tryll::MessageBody_Ack,
		[Done = MoveTemp(Done)](TUniquePtr<::Tryll::AckT> Ack, const FTryllError& Error) mutable
		{
			if (Done)
			{
				// A success can still carry a notice (SetAgentVoice mode fallback).
				if (Error.IsOk() && Ack && !Ack->warning.empty())
				{
					Done(FTryllError(
						static_cast<uint32>(0),
						Tryll::Client::Utf8ToFString(Ack->warning.c_str(), Ack->warning.size())));
				}
				else
				{
					Done(Error);
				}
			}
		},
		MoveTemp(Note),
		bBusyIsExpected);
}

} // namespace Tryll::Client
