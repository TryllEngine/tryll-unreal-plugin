// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllWorkflowModelRefs.h"

#include "Generated/Nodes/TryllNodeParamsBase.h"
#include "TryllVoiceProfile.h"
#include "UObject/UnrealType.h"

namespace
{
	void CollectFromStruct(
		const UStruct* Struct,
		const void* Data,
		const FString& NodeName,
		TArray<FTryllWorkflowModelRef>& Out)
	{
		if (!Struct || !Data)
		{
			return;
		}

#if WITH_METADATA
		// UPROPERTY meta= is editor/cooker-only (WITH_METADATA). Game targets still
		// compile this TU; Collect is only invoked from WITH_EDITOR validation paths.
		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			FProperty* Prop = *It;
			const FStructProperty* StructProp = CastField<FStructProperty>(Prop);
			if (!StructProp || StructProp->Struct != FTryllModelName::StaticStruct())
			{
				continue;
			}
			if (!Prop->HasMetaData(TEXT("TryllModelType")))
			{
				continue;
			}

			// Codegen wraps optional model fields in bOverrideX; only count when active.
			const FString OverrideName = FString::Printf(TEXT("bOverride%s"), *Prop->GetName());
			if (const FBoolProperty* OverrideProp = FindFProperty<FBoolProperty>(Struct, *OverrideName))
			{
				if (!OverrideProp->GetPropertyValue_InContainer(Data))
				{
					continue;
				}
			}

			const FTryllModelName* Model = StructProp->ContainerPtrToValuePtr<FTryllModelName>(Data);
			if (!Model || Model->IsEmpty())
			{
				continue;
			}

			FTryllWorkflowModelRef Ref;
			Ref.NodeName  = NodeName;
			Ref.FieldName = Prop->GetName();
			Ref.ModelName = Model->Name;
			Ref.ModelType = Prop->GetMetaData(TEXT("TryllModelType"));
			Out.Add(MoveTemp(Ref));
		}
#else
		(void)Struct;
		(void)Data;
		(void)NodeName;
		(void)Out;
#endif
	}
}

FString FTryllWorkflowModelViolation::ToString(const FString& AssetLabel) const
{
	const FString Where = Ref.bFromVoiceProfile
		? FString::Printf(TEXT("%s · Voice.%s"), *AssetLabel, *Ref.FieldName)
		: Ref.NodeName.IsEmpty()
		? FString::Printf(TEXT("%s · Graph.%s"), *AssetLabel, *Ref.FieldName)
		: FString::Printf(TEXT("%s · node '%s' · %s"), *AssetLabel, *Ref.NodeName, *Ref.FieldName);
	const TCHAR* Tier =
		Registration == ETryllModelRegistration::Production   ? TEXT("Production") :
		Registration == ETryllModelRegistration::Experimental ? TEXT("Experimental") :
		                                                        TEXT("Unregistered");
	return FString::Printf(TEXT("%s → '%s' (%s)"), *Where, *Ref.ModelName, Tier);
}

TArray<FTryllWorkflowModelRef> FTryllWorkflowModelRefs::Collect(const FTryllGraphDescription& Graph)
{
	TArray<FTryllWorkflowModelRef> Out;
	CollectFromStruct(FTryllGraphDescription::StaticStruct(), &Graph, FString(), Out);

	for (const FTryllNodeDescription& Node : Graph.Nodes)
	{
		if (!Node.Params)
		{
			continue;
		}
		CollectFromStruct(Node.Params->GetClass(), Node.Params.Get(), Node.Name, Out);
	}
	return Out;
}

TArray<FTryllWorkflowModelRef> FTryllWorkflowModelRefs::CollectVoice(const FTryllVoiceProfileData& Voice)
{
	TArray<FTryllWorkflowModelRef> Out;
	CollectFromStruct(FTryllVoiceProfileData::StaticStruct(), &Voice, FString(), Out);
	for (FTryllWorkflowModelRef& Ref : Out)
	{
		Ref.bFromVoiceProfile = true;
	}
	return Out;
}

TArray<FTryllWorkflowModelViolation> FTryllWorkflowModelRefs::FindNonProductionVoiceRefs(
	const FTryllVoiceProfileData& Voice,
	const UTryllModelManifest* Manifest)
{
	TArray<FTryllWorkflowModelViolation> Violations;
	for (const FTryllWorkflowModelRef& Ref : CollectVoice(Voice))
	{
		const ETryllModelRegistration Reg = Manifest
			? Manifest->Get(Ref.ModelName)
			: ETryllModelRegistration::None;
		if (Reg != ETryllModelRegistration::Production)
		{
			FTryllWorkflowModelViolation V;
			V.Ref = Ref;
			V.Registration = Reg;
			Violations.Add(MoveTemp(V));
		}
	}
	return Violations;
}

TArray<FTryllWorkflowModelViolation> FTryllWorkflowModelRefs::FindNonProductionRefs(
	const FTryllGraphDescription& Graph,
	const UTryllModelManifest* Manifest)
{
	TArray<FTryllWorkflowModelViolation> Violations;
	for (const FTryllWorkflowModelRef& Ref : Collect(Graph))
	{
		const ETryllModelRegistration Reg = Manifest
			? Manifest->Get(Ref.ModelName)
			: ETryllModelRegistration::None;
		if (Reg != ETryllModelRegistration::Production)
		{
			FTryllWorkflowModelViolation V;
			V.Ref = Ref;
			V.Registration = Reg;
			Violations.Add(MoveTemp(V));
		}
	}
	return Violations;
}

bool FTryllWorkflowModelRefs::NeedsProductionVad(
	const TArray<FTryllWorkflowModelRef>& Refs,
	const UTryllModelManifest* Manifest,
	FString& OutReason)
{
	OutReason.Reset();
	bool bHasStt = false;
	for (const FTryllWorkflowModelRef& Ref : Refs)
	{
		if (Ref.ModelType.Equals(TEXT("stt"), ESearchCase::IgnoreCase))
		{
			bHasStt = true;
			break;
		}
	}
	if (!bHasStt)
	{
		return false;
	}

	if (Manifest)
	{
		for (const FTryllModelManifestEntry& E : Manifest->Entries)
		{
			if (E.Registration == ETryllModelRegistration::Production
				&& E.ModelType.Equals(TEXT("vad"), ESearchCase::IgnoreCase))
			{
				return false;
			}
		}
	}

	OutReason =
		TEXT("A workflow references an STT model, but no VAD model is marked Production ")
		TEXT("in the Tryll Model Manifest. Offline STT needs the server's default_vad_model ")
		TEXT("(typically Silero VAD) staged into the build — mark a VAD model as Production ")
		TEXT("in the Model Manager, or remove the STT reference.");
	return true;
}
