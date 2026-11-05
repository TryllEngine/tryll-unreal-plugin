// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "TryllObservationTypes.h"

struct FTryllConnEvent;

class TRYLLCLIENT_API ITryllClientObserver
{
public:
	virtual ~ITryllClientObserver() = default;

	// No-throw contract. Unreal: do not use exceptions; implementations must not fail.
	virtual void OnObservation(const FTryllObservationEnvelope& Envelope) = 0;
};

class TRYLLCLIENT_API FTryllNullClientObserver final : public ITryllClientObserver
{
public:
	static TSharedRef<FTryllNullClientObserver> Get();
	void OnObservation(const FTryllObservationEnvelope& Envelope) override;
};

class TRYLLCLIENT_API FTryllCompositeClientObserver final : public ITryllClientObserver
{
public:
	void Add(TSharedRef<ITryllClientObserver> Observer);
	void OnObservation(const FTryllObservationEnvelope& Envelope) override;

private:
	TArray<TSharedRef<ITryllClientObserver>> Observers;
};

class TRYLLCLIENT_API FTryllClientObserverHub
{
public:
	void BeginConnectionEpoch();
	uint64 GetConnectionEpoch() const { return ConnectionEpoch; }
	bool HasSubscribers() const;
	FDelegateHandle Subscribe(TSharedRef<ITryllClientObserver> Observer);
	void Unsubscribe(FDelegateHandle Handle);
	void Observe(const FTryllObservationNote& Note);
	void ObserveEvent(const FTryllConnEvent& Evt);
	/** Journal one finished wire frame. No-op unless someone is subscribed. */
	void ObserveWire(const uint8* Data, int32 Size, ETryllObservationDirection Direction);
	/** Journal one local LogLocalEnvelope. */
	void ObserveLocal(TArray<uint8> LocalFrame, uint64 RequestId, uint64 AgentId, bool bIsError, const FString& Message, uint32 Code);
	/** Remember a carrier id. Does not journal the note; the frame tap does. */
	void AnnotateNote(const FTryllObservationNote& Note);
	void AnnotateCarrier(uint64 RequestId, uint64 CarrierRequestId);

	[[nodiscard]] static TArray<uint8> MakeConnection(::Tryll::LogConnState State, const FString& Reason);
	[[nodiscard]] static TArray<uint8> MakeError(uint32 Code, const FString& CodeName, const FString& Message, ::Tryll::LogErrorOrigin Origin);
	[[nodiscard]] static TArray<uint8> MakeGap(::Tryll::LogGapReason Reason);
	int32 Pump(int32 MaxCount = 64);
	uint64 AllocateEventId();

private:
	struct FSubscriberEntry
	{
		FDelegateHandle Handle;
		TSharedPtr<ITryllClientObserver> Observer;
	};

	uint64 ConnectionEpoch = 0;
	std::atomic<uint64> NextEventId{1};
	TQueue<FTryllObservationEnvelope> Queue;
	TArray<FSubscriberEntry> Subscribers;
	TMap<uint64, uint64> Carriers;
	mutable FCriticalSection Gate;
};
