// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllMemoryPresentation.h"

FString FTryllMemoryPresentation::FmtBinaryBytes(int64 Bytes)
{
	constexpr double Kib = 1024.0;
	constexpr double Mib = Kib * 1024.0;
	constexpr double Gib = Mib * 1024.0;
	if (Bytes >= static_cast<int64>(Gib))
	{
		return FString::Printf(TEXT("%.2f GiB"), Bytes / Gib);
	}
	if (Bytes >= static_cast<int64>(Mib))
	{
		return FString::Printf(TEXT("%.1f MiB"), Bytes / Mib);
	}
	if (Bytes >= static_cast<int64>(Kib))
	{
		return FString::Printf(TEXT("%.1f KiB"), Bytes / Kib);
	}
	return FString::Printf(TEXT("%lld B"), Bytes);
}

FString FTryllMemoryPresentation::FmtClock(int64 UnixMs)
{
	if (UnixMs <= 0)
	{
		return TEXT("—");
	}
	const FTimespan LocalOffset = FDateTime::Now() - FDateTime::UtcNow();
	const FDateTime Local = FDateTime::FromUnixTimestamp(UnixMs / 1000) + LocalOffset;
	return Local.ToString(TEXT("%H:%M:%S"));
}

FString FTryllMemoryPresentation::FmtIdle(int64 LastTurnAtMs, int64 NowMs)
{
	if (LastTurnAtMs <= 0)
	{
		return TEXT("never run");
	}
	const double S = FMath::Max(0.0, (NowMs - LastTurnAtMs) / 1000.0);
	if (S < 1.0)
	{
		return TEXT("now");
	}
	if (S < 60.0)
	{
		return FString::Printf(TEXT("%.0fs ago"), S);
	}
	if (S < 3600.0)
	{
		return FString::Printf(TEXT("%.0fm ago"), S / 60.0);
	}
	return FString::Printf(TEXT("%.1fh ago"), S / 3600.0);
}

int32 FTryllMemoryPresentation::ClassOrder(ETryllInferenceConsumerClass Cls)
{
	switch (Cls)
	{
	case ETryllInferenceConsumerClass::Background:          return 0;
	case ETryllInferenceConsumerClass::InteractiveBuffered: return 1;
	case ETryllInferenceConsumerClass::InteractiveVisible:  return 2;
	default:                                                return 9;
	}
}

FString FTryllMemoryPresentation::ConsumerClassLabel(
	ETryllInferenceConsumerClass Cls, const bool bResident)
{
	FString Label;
	switch (Cls)
	{
	case ETryllInferenceConsumerClass::InteractiveVisible:  Label = TEXT("visible"); break;
	case ETryllInferenceConsumerClass::InteractiveBuffered: Label = TEXT("buffered"); break;
	case ETryllInferenceConsumerClass::Background:          Label = TEXT("background"); break;
	default: break;
	}
	if (!bResident)
	{
		Label = Label.IsEmpty() ? FString(TEXT("evicted")) : Label + TEXT(" · evicted");
	}
	return Label;
}

FString FTryllMemoryPresentation::FormatRamRange(
	const int64 RamBytes, const int64 RamBytesHigh, const ETryllMemoryTier Tier)
{
	FString Ram = RamBytesHigh > 0
		? FString::Printf(TEXT("~%s – %s"), *FmtBinaryBytes(RamBytes), *FmtBinaryBytes(RamBytesHigh))
		: FmtBinaryBytes(RamBytes);
	if (Tier == ETryllMemoryTier::Estimated && RamBytesHigh == 0)
	{
		Ram = TEXT("~") + Ram;
	}
	return Ram;
}

FString FTryllMemoryPresentation::ContextCaption(
	const int32 UsedTokens, const int32 CapacityTokens, const bool bOccupancyStale)
{
	if (CapacityTokens <= 0)
	{
		return TEXT("—");
	}
	const float Frac = FMath::Clamp(UsedTokens / static_cast<float>(CapacityTokens), 0.f, 1.f);
	FString Caption = FString::Printf(
		TEXT("%d / %d (%.0f%%)"), UsedTokens, CapacityTokens, Frac * 100.f);
	if (bOccupancyStale)
	{
		Caption += TEXT("  stale");
	}
	return Caption;
}

FString FTryllMemoryPresentation::StaleBannerText(
	const TOptional<FTryllMemorySnapshot>& Snapshot, const FString& StaleError)
{
	if (StaleError.IsEmpty())
	{
		return FString();
	}
	if (Snapshot.IsSet())
	{
		return FString::Printf(
			TEXT("Showing a stale snapshot from %s.\n%s"),
			*FmtClock(Snapshot->CapturedAtMs),
			*StaleError);
	}
	return StaleError;
}

FTryllMemoryPresentationState FTryllMemoryPresentation::ApplyRefreshStarted(
	FTryllMemoryPresentationState State)
{
	State.bRefreshInFlight = true;
	return State;
}

FTryllMemoryPresentationState FTryllMemoryPresentation::ApplyRefreshSucceeded(
	FTryllMemoryPresentationState State, FTryllMemorySnapshot Snapshot)
{
	State.bRefreshInFlight = false;
	State.StaleError.Empty();
	State.Snapshot = MoveTemp(Snapshot);
	return State;
}

FTryllMemoryPresentationState FTryllMemoryPresentation::ApplyRefreshFailed(
	FTryllMemoryPresentationState State, const FString& Error)
{
	State.bRefreshInFlight = false;
	State.StaleError = Error.IsEmpty() ? FString(TEXT("Refresh failed")) : Error;
	return State;
}

FTryllMemoryPresentationState FTryllMemoryPresentation::ApplyDisconnected(
	FTryllMemoryPresentationState State, const FString& Reason)
{
	if (State.Snapshot.IsSet() && State.StaleError.IsEmpty())
	{
		State.StaleError = Reason.IsEmpty() ? FString(TEXT("Disconnected")) : Reason;
	}
	return State;
}

const TCHAR* FTryllMemoryPresentation::UnattributedMappedNote =
	TEXT("Most of the working set is unattributed. Memory-mapped model files count as resident pages but are not allocations we make, so a VRAM-placed model shows up here rather than under its own row.");
const TCHAR* FTryllMemoryPresentation::DevicesUninitializedNote =
	TEXT("No backend initialised yet — devices register on the first model load. This is not an error.");
const TCHAR* FTryllMemoryPresentation::DevicesEmptyNote =
	TEXT("Backend is initialised but no devices were reported.");
const TCHAR* FTryllMemoryPresentation::DevicesFooterNote =
	TEXT("Other is everything on the device that is not Tryll — the editor, the compositor, other processes. The editor's own VRAM cannot be split out of that number.");
const TCHAR* FTryllMemoryPresentation::ContextHeadroomNote =
	TEXT("Context used is headroom to the limit, not memory growth: the KV buffer is allocated for the full context size when the agent is created and does not grow with the conversation.");
const TCHAR* FTryllMemoryPresentation::EmptyKvNote = TEXT("No language-model contexts yet.");

FTryllMemoryPresentation::ETone FTryllMemoryPresentation::ClassTone(
	const ETryllInferenceConsumerClass Cls, const bool bResident)
{
	if (!bResident)
	{
		return ETone::Err;
	}
	switch (Cls)
	{
	case ETryllInferenceConsumerClass::InteractiveVisible:  return ETone::Ok;
	case ETryllInferenceConsumerClass::InteractiveBuffered: return ETone::Accent;
	case ETryllInferenceConsumerClass::Background:          return ETone::Warn;
	default:                                                return ETone::Dim;
	}
}

void FTryllMemoryPresentation::TierPresentation(
	const ETryllMemoryTier Tier, FString& OutLabel, ETone& OutTone)
{
	switch (Tier)
	{
	case ETryllMemoryTier::Measured:
		OutLabel = TEXT("measured");
		OutTone = ETone::Ok;
		break;
	case ETryllMemoryTier::Derived:
		OutLabel = TEXT("derived");
		OutTone = ETone::Accent;
		break;
	case ETryllMemoryTier::Estimated:
		OutLabel = TEXT("estimated");
		OutTone = ETone::Warn;
		break;
	default:
		OutLabel = TEXT("measured");
		OutTone = ETone::Dim;
		break;
	}
}

FString FTryllMemoryPresentation::DisplayName(const FString& Name)
{
	return Name.IsEmpty() ? FString(TEXT("—")) : Name;
}

FString FTryllMemoryPresentation::DeviceLabel(const FString& Description, const FString& Name)
{
	if (!Description.IsEmpty())
	{
		return Description;
	}
	if (!Name.IsEmpty())
	{
		return Name;
	}
	return TEXT("—");
}

FTryllMemoryPresentation::FTables FTryllMemoryPresentation::Project(
	const FTryllMemorySnapshot& Snapshot, const int64 NowMs)
{
	FTables Tables;
	const FTryllMemoryComponent* Residual = nullptr;
	TArray<FTryllMemoryComponent> Kv;
	bool bAnyModelResident = false;
	for (const FTryllMemoryComponent& C : Snapshot.Components)
	{
		if (C.Owner == ETryllMemoryOwnerKind::AgentNode)
		{
			Kv.Add(C);
		}
		else
		{
			FComponentRow Row;
			FString TierLabel;
			ETone TierTone = ETone::Dim;
			TierPresentation(C.Tier, TierLabel, TierTone);
			Row.Kind = DisplayName(C.Kind);
			Row.Name = DisplayName(C.Name);
			Row.TierLabel = TierLabel;
			Row.TierTone = TierTone;
			Row.Ram = FormatRamRange(C.RamBytes, C.RamBytesHigh, C.Tier);
			Row.bRamWarn = C.Tier == ETryllMemoryTier::Estimated;
			Row.Vram = FmtBinaryBytes(C.VramBytes);
			Row.Tooltip = C.EstimateBasis;
			Tables.ComponentRows.Add(Row);
		}
		if (C.Owner == ETryllMemoryOwnerKind::Model && (C.RamBytes > 0 || C.VramBytes > 0))
		{
			bAnyModelResident = true;
		}
		if (C.Owner == ETryllMemoryOwnerKind::Unattributed && Residual == nullptr)
		{
			Residual = &C;
		}
	}

	const double ResidPct = Snapshot.ProcessWorkingSetBytes > 0
		? (static_cast<double>(Snapshot.UnattributedRamBytes)
			/ static_cast<double>(Snapshot.ProcessWorkingSetBytes)) * 100.0
		: 0.0;
	if (ResidPct > 50.0 && bAnyModelResident)
	{
		Tables.SystemNotes.Add(UnattributedMappedNote);
	}
	if (Residual != nullptr && Residual->Tier == ETryllMemoryTier::Estimated)
	{
		Tables.SystemNotes.Add(FString::Printf(
			TEXT("Unattributed inherits Estimated because some components are estimates. %s"),
			*Residual->EstimateBasis));
	}

	if (!Snapshot.bBackendInitialized)
	{
		Tables.DevicesKind = EDeviceSection::Uninitialized;
	}
	else if (Snapshot.Devices.Num() == 0)
	{
		Tables.DevicesKind = EDeviceSection::Empty;
	}
	else
	{
		Tables.DevicesKind = EDeviceSection::Table;
		for (const FTryllMemoryDevice& D : Snapshot.Devices)
		{
			FDeviceRow Row;
			Row.Label = DeviceLabel(D.Description, D.Name);
			Row.Kind = D.Kind.IsEmpty() ? FString(TEXT("—")) : D.Kind;
			Row.Self = FmtBinaryBytes(D.GetSelfBytes());
			Row.Other = FmtBinaryBytes(D.OtherBytes);
			Row.Free = FmtBinaryBytes(D.FreeBytes);
			Row.Total = FmtBinaryBytes(D.TotalBytes);
			Tables.Devices.Add(Row);
		}
	}

	Kv.Sort([](const FTryllMemoryComponent& A, const FTryllMemoryComponent& B)
	{
		const int32 Oa = ClassOrder(A.ConsumerClass);
		const int32 Ob = ClassOrder(B.ConsumerClass);
		if (Oa != Ob)
		{
			return Oa < Ob;
		}
		return (A.RamBytes + A.VramBytes) > (B.RamBytes + B.VramBytes);
	});

	int64 KvTotal = 0;
	int64 BgTotal = 0;
	for (const FTryllMemoryComponent& C : Kv)
	{
		const int64 Bytes = C.RamBytes + C.VramBytes;
		KvTotal += Bytes;
		if (C.ConsumerClass == ETryllInferenceConsumerClass::Background)
		{
			BgTotal += Bytes;
		}
		FKvRow Row;
		const int32 Cap = static_cast<int32>(C.CapacityTokens);
		Row.Name = DisplayName(C.Name);
		Row.ClassLabel = ConsumerClassLabel(C.ConsumerClass, C.bResident);
		Row.ClassTone = ClassTone(C.ConsumerClass, C.bResident);
		Row.bBackground = C.ConsumerClass == ETryllInferenceConsumerClass::Background;
		Row.bOccupancyStale = C.bOccupancyStale;
		Row.UsedTokens = static_cast<int32>(C.UsedTokens);
		Row.CapacityTokens = Cap;
		Row.OccupancyFrac = Cap > 0
			? FMath::Clamp(C.UsedTokens / static_cast<float>(Cap), 0.f, 1.f)
			: 0.f;
		Row.ContextCaption = ContextCaption(Row.UsedTokens, Cap, C.bOccupancyStale);
		Row.When = FmtIdle(C.LastTurnAtMs, NowMs);
		Row.Reserved = FmtBinaryBytes(Bytes);
		Tables.KvRows.Add(Row);
	}
	Tables.KvTotalLabel = FmtBinaryBytes(KvTotal);
	if (BgTotal > 0)
	{
		Tables.BackgroundKvNote = FString::Printf(
			TEXT("%s of KV belongs to background work — nobody is waiting on it."),
			*FmtBinaryBytes(BgTotal));
	}
	return Tables;
}
