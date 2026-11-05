// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllObservationTypes.h"

FTryllTurnKey FTryllTurnKey::FromRequest(uint64 Epoch, uint64 InAgentId, uint64 InRequestId)
{
	FTryllTurnKey Key;
	Key.ConnectionEpoch = Epoch;
	Key.AgentId         = InAgentId;
	Key.RequestId       = InRequestId;
	Key.bHasValue       = true;
	return Key;
}

FTryllTurnKey FTryllTurnKey::FromSynthetic(uint64 Epoch, uint64 InAgentId, uint64 Seq)
{
	FTryllTurnKey Key;
	Key.ConnectionEpoch   = Epoch;
	Key.AgentId           = InAgentId;
	Key.SyntheticSequence = Seq;
	Key.bHasValue         = true;
	return Key;
}

bool FTryllTurnKey::operator==(const FTryllTurnKey& Other) const
{
	return bHasValue == Other.bHasValue
		&& ConnectionEpoch == Other.ConnectionEpoch
		&& AgentId == Other.AgentId
		&& RequestId == Other.RequestId
		&& SyntheticSequence == Other.SyntheticSequence;
}

uint32 GetTypeHash(const FTryllTurnKey& Key)
{
	if (!Key.bHasValue)
	{
		return 0;
	}

	uint32 Hash = GetTypeHash(Key.ConnectionEpoch);
	Hash        = HashCombine(Hash, GetTypeHash(Key.AgentId));
	Hash        = HashCombine(Hash, GetTypeHash(Key.RequestId));
	Hash        = HashCombine(Hash, GetTypeHash(Key.SyntheticSequence));
	return Hash;
}
