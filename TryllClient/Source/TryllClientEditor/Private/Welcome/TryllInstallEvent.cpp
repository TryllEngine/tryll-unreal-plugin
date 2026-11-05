// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "Welcome/TryllInstallEvent.h"

#include "TryllLicence.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/DateTime.h"
#include "Misc/EngineVersion.h"
#include "Misc/Paths.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonWriter.h"

// Named, not anonymous: UBT unity builds merge this file with others in the module.
namespace TryllInstallEventPrivate
{
FDelegateHandle LicenceChangedHandle;

/** Seconds rounded to one decimal, written like Unity ("0", "12.5"). */
FString FormatSeconds(double Seconds)
{
	const double Rounded = FMath::RoundToDouble(Seconds * 10.0) / 10.0;
	const int64 Whole = static_cast<int64>(Rounded);
	if (static_cast<double>(Whole) == Rounded)
	{
		return FString::Printf(TEXT("%lld"), Whole);
	}
	return FString::Printf(TEXT("%.1f"), Rounded);
}

/** The stored ';'-joined list of sent versions. An older single version is a list of one. */
TArray<FString> SplitVersions(const FString& SentVersions)
{
	TArray<FString> Parts;
	SentVersions.ParseIntoArray(Parts, TEXT(";"), /*InCullEmpty=*/true);
	TArray<FString> Versions;
	for (FString& Part : Parts)
	{
		Part.TrimStartAndEndInline();
		if (!Part.IsEmpty())
		{
			Versions.Add(MoveTemp(Part));
		}
	}
	return Versions;
}

void Post(const FString& Body)
{
	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(FTryllInstallEvent::Endpoint);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(Body);
	Request->SetTimeout(5.0f);
	// No completion handler: telemetry never retries and never surfaces an error.
	Request->ProcessRequest();
}
} // namespace TryllInstallEventPrivate

const TCHAR* const FTryllInstallEvent::EventName = TEXT("tryll.server.install");
// Same PostHog project and write-only ingest key as the server's sink
// (tryll/data/server-config.json). An ingest key can only add events.
const TCHAR* const FTryllInstallEvent::Endpoint = TEXT("https://eu.i.posthog.com/capture/");
const TCHAR* const FTryllInstallEvent::ProjectKey = TEXT("phc_n5RuNgNFUfjtnXiExVTAAAgpsgZSEDS3a7xAxKWMdCEU");

TFunction<void(const FString& Body)> FTryllInstallEvent::DebugSenderForTests;

FString FTryllInstallEvent::BuildBody(const FInfo& Info, const FString& DistinctId, const FString& EngineVersion,
	const FDateTime& UtcNow)
{
	FString Body;
	const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Body);
	Writer->WriteObjectStart();
	Writer->WriteValue(TEXT("api_key"), FString(ProjectKey));
	Writer->WriteValue(TEXT("event"), FString(EventName));
	Writer->WriteValue(TEXT("distinct_id"), DistinctId);
	Writer->WriteValue(TEXT("timestamp"), UtcNow.ToIso8601());
	Writer->WriteObjectStart(TEXT("properties"));
	Writer->WriteValue(TEXT("$process_person_profile"), false);
	Writer->WriteValue(TEXT("result"), Info.Result);
	Writer->WriteValue(TEXT("version"), Info.Version);
	Writer->WriteValue(TEXT("variant"), Info.Variant);
	Writer->WriteValue(TEXT("source"), Info.Source);
	Writer->WriteValue(TEXT("channel"), Info.Channel);
	Writer->WriteValue(TEXT("engine"), FString(TEXT("unreal")));
	Writer->WriteValue(TEXT("engine_version"), EngineVersion);
	Writer->WriteValue(TEXT("bytes"), Info.Bytes);
	Writer->WriteRawJSONValue(TEXT("duration_s"), TryllInstallEventPrivate::FormatSeconds(Info.Seconds));
	Writer->WriteObjectEnd();
	Writer->WriteObjectEnd();
	Writer->Close();
	return Body;
}

bool FTryllInstallEvent::IsDue(bool bLicenceAccepted, const FString& PluginVersion, const FString& SentVersions)
{
	return bLicenceAccepted && !PluginVersion.IsEmpty()
		&& !TryllInstallEventPrivate::SplitVersions(SentVersions).Contains(PluginVersion);
}

FString FTryllInstallEvent::AddSentVersion(const FString& SentVersions, const FString& Version)
{
	TArray<FString> Versions = TryllInstallEventPrivate::SplitVersions(SentVersions);
	Versions.Remove(Version);
	Versions.Add(Version);
	if (Versions.Num() > MaxSentVersions)
	{
		Versions.RemoveAt(0, Versions.Num() - MaxSentVersions);
	}
	return FString::Join(Versions, TEXT(";"));
}

FTryllInstallEvent::FInfo FTryllInstallEvent::MakePluginInfo()
{
	FInfo Info;
	Info.Result = TEXT("ok");
	Info.Version = PluginVersion();
	Info.Variant = TEXT("full");
	Info.Source = TEXT("zip");
	Info.Channel = Channel();
	return Info;
}

FString FTryllInstallEvent::PluginVersion()
{
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("TryllClient"));
	return Plugin.IsValid() ? Plugin->GetDescriptor().VersionName : FString();
}

FString FTryllInstallEvent::Channel()
{
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("TryllClient"));
	const bool bDev = Plugin.IsValid()
		&& FPaths::FileExists(FPaths::Combine(Plugin->GetBaseDir(), TEXT("TryllDeveloperTools.marker")));
	return bDev ? TEXT("dev") : TEXT("release");
}

void FTryllInstallEvent::SendIfDue()
{
	const FInfo Info = MakePluginInfo();
	const FString SentVersions = FTryllLicence::GetLastInstallEventVersion();
	if (!IsDue(FTryllLicence::IsAccepted(), Info.Version, SentVersions))
	{
		return;
	}
	// Recorded before the send: one try per version, never a retry loop.
	FTryllLicence::SetLastInstallEventVersion(AddSentVersion(SentVersions, Info.Version));
	const FString Body = BuildBody(Info, FTryllLicence::GetInstallId(),
		FEngineVersion::Current().ToString(EVersionComponent::Patch), FDateTime::UtcNow());
	if (DebugSenderForTests)
	{
		DebugSenderForTests(Body);
		return;
	}
	TryllInstallEventPrivate::Post(Body);
}

void FTryllInstallEvent::Register()
{
	using namespace TryllInstallEventPrivate;
	if (LicenceChangedHandle.IsValid())
	{
		return;
	}
	LicenceChangedHandle = FTryllLicence::OnChanged().AddLambda([]()
	{
		// Tests flip the in-memory store; they must never send a real event.
		if (!FTryllLicence::IsUsingInMemoryStoreForTests())
		{
			SendIfDue();
		}
	});
}

void FTryllInstallEvent::Unregister()
{
	using namespace TryllInstallEventPrivate;
	if (LicenceChangedHandle.IsValid())
	{
		FTryllLicence::OnChanged().Remove(LicenceChangedHandle);
		LicenceChangedHandle.Reset();
	}
}
