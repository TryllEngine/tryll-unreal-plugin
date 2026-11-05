// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "ClientWire.h"

#include "Sockets.h"
#include "tryll/Protocol.h"

#include <array>

namespace Tryll::Client {

namespace {

[[nodiscard]] bool RecvExact(FSocket* Socket, uint8* Buffer, int32 Size)
{
	int32 Total = 0;
	while (Total < Size)
	{
		int32 Read = 0;
		if (!Socket->Recv(Buffer + Total, Size - Total, Read))
		{
			return false;
		}
		if (Read <= 0)
		{
			return false;
		}
		Total += Read;
	}
	return true;
}

[[nodiscard]] bool SendExact(FSocket* Socket, const uint8* Buffer, int32 Size)
{
	int32 Total = 0;
	while (Total < Size)
	{
		int32 Sent = 0;
		if (!Socket->Send(Buffer + Total, Size - Total, Sent))
		{
			return false;
		}
		if (Sent <= 0)
		{
			return false;
		}
		Total += Sent;
	}
	return true;
}

} // namespace

bool ReadFrame(FSocket* Socket, TArray<uint8>& OutPayload)
{
	std::array<uint8, Tryll::LengthPrefixBytes> LenBuf{};
	if (!RecvExact(Socket, LenBuf.data(), static_cast<int32>(LenBuf.size())))
	{
		return false;
	}

	const uint32 Len = Tryll::ReadLengthPrefixLE(LenBuf);
	if (!Tryll::IsValidPayloadLength(Len))
	{
		return false;
	}

	OutPayload.SetNumUninitialized(static_cast<int32>(Len));
	if (!RecvExact(Socket, OutPayload.GetData(), OutPayload.Num()))
	{
		return false;
	}
	return true;
}

ETryllWriteFrameResult WriteFrame(FSocket* Socket, const TArray<uint8>& Payload)
{
	if (Payload.Num() <= 0 ||
		static_cast<uint32>(Payload.Num()) > static_cast<uint32>(Tryll::MaxFramePayloadBytes))
	{
		return ETryllWriteFrameResult::TooLarge;
	}

	std::array<uint8, Tryll::LengthPrefixBytes> LenBuf{};
	Tryll::WriteLengthPrefixLE(static_cast<uint32>(Payload.Num()), LenBuf);

	if (!SendExact(Socket, LenBuf.data(), static_cast<int32>(LenBuf.size())))
	{
		return ETryllWriteFrameResult::IoError;
	}
	if (!SendExact(Socket, Payload.GetData(), Payload.Num()))
	{
		return ETryllWriteFrameResult::IoError;
	}
	return ETryllWriteFrameResult::Ok;
}

} // namespace Tryll::Client
