// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllMemorySnapshot.h"

#include "Win32Undef.h"
#include "TryllUtf8.h"
#include "messages_generated.h"

using Tryll::Client::Utf8ToFString;

FTryllMemorySnapshot FTryllMemorySnapshot::FromWire(const ::Tryll::GetMemoryConsumptionResponse* Resp)
{
	FTryllMemorySnapshot Snap;
	if (!Resp)
	{
		return Snap;
	}

	const ::Tryll::MemorySnapshot* S = Resp->snapshot();
	if (!S)
	{
		return Snap;
	}

	Snap.CapturedAtMs           = S->captured_at_ms();
	Snap.ProcessWorkingSetBytes = static_cast<int64>(S->process_working_set_bytes());
	Snap.ProcessPrivateBytes    = static_cast<int64>(S->process_private_bytes());
	Snap.bProcessAvailable      = S->process_available();
	Snap.bBackendInitialized    = S->backend_initialized();
	Snap.UnattributedRamBytes   = static_cast<int64>(S->unattributed_ram_bytes());

	if (const auto* Devices = S->devices())
	{
		Snap.Devices.Reserve(static_cast<int32>(Devices->size()));
		for (const auto* D : *Devices)
		{
			if (!D)
			{
				continue;
			}
			FTryllMemoryDevice Dev;
			Dev.Index            = D->index();
			Dev.Name             = Utf8ToFString(D->name());
			Dev.Description      = Utf8ToFString(D->description());
			Dev.Kind             = Utf8ToFString(D->kind());
			Dev.TotalBytes       = static_cast<int64>(D->total_bytes());
			Dev.FreeBytes        = static_cast<int64>(D->free_bytes());
			Dev.SelfModelBytes   = static_cast<int64>(D->self_model_bytes());
			Dev.SelfContextBytes = static_cast<int64>(D->self_context_bytes());
			Dev.SelfComputeBytes = static_cast<int64>(D->self_compute_bytes());
			Dev.OtherBytes       = static_cast<int64>(D->other_bytes());
			Snap.Devices.Add(MoveTemp(Dev));
		}
	}

	if (const auto* Components = S->components())
	{
		Snap.Components.Reserve(static_cast<int32>(Components->size()));
		for (const auto* C : *Components)
		{
			if (!C)
			{
				continue;
			}
			FTryllMemoryComponent Row;
			Row.Kind            = Utf8ToFString(C->kind());
			Row.Name            = Utf8ToFString(C->name());
			Row.Owner           = static_cast<ETryllMemoryOwnerKind>(C->owner());
			Row.Tier            = static_cast<ETryllMemoryTier>(C->tier());
			Row.EstimateBasis   = Utf8ToFString(C->estimate_basis());
			Row.RamBytes        = static_cast<int64>(C->ram_bytes());
			Row.VramBytes       = static_cast<int64>(C->vram_bytes());
			Row.RamBytesHigh    = static_cast<int64>(C->ram_bytes_high());
			Row.DeviceIndex     = C->device_index();
			Row.SessionId       = static_cast<int64>(C->session_id());
			Row.AgentId         = static_cast<int64>(C->agent_id());
			Row.NodeName        = Utf8ToFString(C->node_name());
			Row.UsedTokens      = static_cast<int32>(C->used_tokens());
			Row.CapacityTokens  = static_cast<int32>(C->capacity_tokens());
			Row.bResident       = C->resident();
			Row.bOccupancyStale = C->occupancy_stale();
			Row.ComputeBytes    = static_cast<int64>(C->compute_bytes());
			Row.ConsumerClass   = static_cast<ETryllInferenceConsumerClass>(C->consumer_class());
			Row.LastTurnAtMs    = C->last_turn_at_ms();
			if (const auto* PerDevice = C->per_device())
			{
				Row.PerDevice.Reserve(static_cast<int32>(PerDevice->size()));
				for (const auto* P : *PerDevice)
				{
					if (!P)
					{
						continue;
					}
					FTryllMemoryComponentDevice Slot;
					Slot.DeviceIndex  = P->device_index();
					Slot.Bytes        = static_cast<int64>(P->bytes());
					Slot.ComputeBytes = static_cast<int64>(P->compute_bytes());
					Row.PerDevice.Add(Slot);
				}
			}
			Snap.Components.Add(MoveTemp(Row));
		}
	}

	return Snap;
}

FTryllMemorySnapshot FTryllMemorySnapshot::FromWire(const ::Tryll::GetMemoryConsumptionResponseT& Resp)
{
	FTryllMemorySnapshot Snap;
	const ::Tryll::MemorySnapshotT* S = Resp.snapshot.get();
	if (!S)
	{
		return Snap;
	}

	Snap.CapturedAtMs           = S->captured_at_ms;
	Snap.ProcessWorkingSetBytes = static_cast<int64>(S->process_working_set_bytes);
	Snap.ProcessPrivateBytes    = static_cast<int64>(S->process_private_bytes);
	Snap.bProcessAvailable      = S->process_available;
	Snap.bBackendInitialized    = S->backend_initialized;
	Snap.UnattributedRamBytes   = static_cast<int64>(S->unattributed_ram_bytes);

	Snap.Devices.Reserve(static_cast<int32>(S->devices.size()));
	for (const std::unique_ptr<::Tryll::MemoryDeviceT>& D : S->devices)
	{
		if (!D)
		{
			continue;
		}
		FTryllMemoryDevice Dev;
		Dev.Index            = D->index;
		Dev.Name             = UTF8_TO_TCHAR(D->name.c_str());
		Dev.Description      = UTF8_TO_TCHAR(D->description.c_str());
		Dev.Kind             = UTF8_TO_TCHAR(D->kind.c_str());
		Dev.TotalBytes       = static_cast<int64>(D->total_bytes);
		Dev.FreeBytes        = static_cast<int64>(D->free_bytes);
		Dev.SelfModelBytes   = static_cast<int64>(D->self_model_bytes);
		Dev.SelfContextBytes = static_cast<int64>(D->self_context_bytes);
		Dev.SelfComputeBytes = static_cast<int64>(D->self_compute_bytes);
		Dev.OtherBytes       = static_cast<int64>(D->other_bytes);
		Snap.Devices.Add(MoveTemp(Dev));
	}

	Snap.Components.Reserve(static_cast<int32>(S->components.size()));
	for (const std::unique_ptr<::Tryll::MemoryComponentT>& C : S->components)
	{
		if (!C)
		{
			continue;
		}
		FTryllMemoryComponent Row;
		Row.Kind            = UTF8_TO_TCHAR(C->kind.c_str());
		Row.Name            = UTF8_TO_TCHAR(C->name.c_str());
		Row.Owner           = static_cast<ETryllMemoryOwnerKind>(C->owner);
		Row.Tier            = static_cast<ETryllMemoryTier>(C->tier);
		Row.EstimateBasis   = UTF8_TO_TCHAR(C->estimate_basis.c_str());
		Row.RamBytes        = static_cast<int64>(C->ram_bytes);
		Row.VramBytes       = static_cast<int64>(C->vram_bytes);
		Row.RamBytesHigh    = static_cast<int64>(C->ram_bytes_high);
		Row.DeviceIndex     = C->device_index;
		Row.SessionId       = static_cast<int64>(C->session_id);
		Row.AgentId         = static_cast<int64>(C->agent_id);
		Row.NodeName        = UTF8_TO_TCHAR(C->node_name.c_str());
		Row.UsedTokens      = static_cast<int32>(C->used_tokens);
		Row.CapacityTokens  = static_cast<int32>(C->capacity_tokens);
		Row.bResident       = C->resident;
		Row.bOccupancyStale = C->occupancy_stale;
		Row.ComputeBytes    = static_cast<int64>(C->compute_bytes);
		Row.ConsumerClass   = static_cast<ETryllInferenceConsumerClass>(C->consumer_class);
		Row.LastTurnAtMs    = C->last_turn_at_ms;
		Row.PerDevice.Reserve(static_cast<int32>(C->per_device.size()));
		for (const std::unique_ptr<::Tryll::MemoryComponentDeviceT>& P : C->per_device)
		{
			if (!P)
			{
				continue;
			}
			FTryllMemoryComponentDevice Slot;
			Slot.DeviceIndex  = P->device_index;
			Slot.Bytes        = static_cast<int64>(P->bytes);
			Slot.ComputeBytes = static_cast<int64>(P->compute_bytes);
			Row.PerDevice.Add(Slot);
		}
		Snap.Components.Add(MoveTemp(Row));
	}

	return Snap;
}
