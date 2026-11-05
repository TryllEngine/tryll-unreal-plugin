// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

class FSocket;

namespace Tryll::Client {

/** Distinguishes a locally detected size violation from a dead socket. */
enum class ETryllWriteFrameResult : uint8
{
	Ok,
	TooLarge,
	IoError,
};

/**
 * Read one length-prefixed FlatBuffers frame from the socket.
 * Blocks until all bytes have been received or an error occurs.
 * Returns false on socket error or if the frame size is invalid.
 */
// TRYLLCLIENT_API: also called from TryllClientEditor (UTryllEditorSubsystem port probe).
TRYLLCLIENT_API [[nodiscard]] bool ReadFrame(FSocket* Socket, TArray<uint8>& OutPayload);

/**
 * Write one length-prefixed FlatBuffers frame to the socket.
 * TooLarge is a local input error — fail that request and keep the socket.
 * IoError means the socket is gone.
 */
TRYLLCLIENT_API [[nodiscard]] ETryllWriteFrameResult WriteFrame(FSocket* Socket, const TArray<uint8>& Payload);

} // namespace Tryll::Client
