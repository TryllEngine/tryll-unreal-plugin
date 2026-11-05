// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllVoiceProfile.h"

#include "TryllClientModule.h"
#include "TryllModelInfo.h"
#include "TryllModelManifest.h"
#include "TryllRuntimeSettings.h"
#include "TryllWorkflowModelRefs.h"
#include "TryllWorkflowValidator.h"

#include "Misc/Paths.h"

#include "Win32Undef.h"
#include "voice_generated.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#include "UObject/ObjectSaveContext.h"
#endif

const TCHAR* FTryllVoiceProfileData::NeutralMode = TEXT("neutral");

namespace
{
	std::string VoiceToUtf8(const FString& Value)
	{
		return std::string(TCHAR_TO_UTF8(*Value));
	}

	/** Same normalization the generated node Pack code applies to file pickers. */
	std::string StoragePathVoiceToUtf8(const FTryllStoragePath& Path)
	{
		return VoiceToUtf8(UTryllRuntimeSettings::NormalizeStorageRelativePath(Path.Path));
	}

	/** Mirrors the server's description/instruction rule: <= 1024 bytes, no '"', '<', '>' and no control characters except tab, CR and LF. */
	void CheckVoiceFreeText(const FString& Where, const TCHAR* Field, const FString& Value, TArray<FTryllWorkflowIssue>& Out)
	{
		if (Value.IsEmpty())
		{
			return;
		}
		const int32 Bytes = FTCHARToUTF8(*Value).Length();
		if (Bytes > 1024)
		{
			Out.Add({ FTryllWorkflowIssue::ESeverity::Error,
				FString::Printf(TEXT("%s field '%s' is %d bytes; the limit is 1024."), *Where, Field, Bytes) });
		}
		for (const TCHAR C : Value)
		{
			const bool bControl = C < 0x20 && C != TEXT('\t') && C != TEXT('\r') && C != TEXT('\n');
			if (C == TEXT('"') || C == TEXT('<') || C == TEXT('>') || bControl || C == 0x7F)
			{
				Out.Add({ FTryllWorkflowIssue::ESeverity::Error,
					FString::Printf(TEXT("%s field '%s' contains a character the server rejects ('\"', '<', '>' or a control character)."),
						*Where, Field) });
				return;
			}
		}
	}

	void CheckVoiceClip(const FString& Where, const TCHAR* Field, const FTryllStoragePath& Clip, bool bCheckExists,
		TArray<FTryllWorkflowIssue>& Out)
	{
		if (Clip.Path.IsEmpty())
		{
			return;
		}
		if (!FTryllWorkflowValidator::ValidateStoragePathShape(Where, Field, Clip.Path, Out) || !bCheckExists)
		{
			return;
		}
		const FString Root = UTryllRuntimeSettings::ResolveStorageDataFolderForServer(
			UTryllRuntimeSettings::GetConfiguredStorageDataFolder());
		if (Root.IsEmpty() || !FPaths::DirectoryExists(Root))
		{
			return;   // no storage folder to check against; the server still checks at runtime
		}
		const FString Relative = UTryllRuntimeSettings::NormalizeStorageRelativePath(Clip.Path);
		if (!FPaths::FileExists(FPaths::Combine(Root, Relative)))
		{
			Out.Add({ FTryllWorkflowIssue::ESeverity::Error,
				FString::Printf(TEXT("%s field '%s': clip '%s' does not exist under the storage folder '%s'."),
					*Where, Field, *Relative, *Root) });
		}
	}
}

void FTryllVoiceProfileData::ToWire(::Tryll::VoiceProfileT& Out) const
{
	Out = ::Tryll::VoiceProfileT{};
	Out.tts_model_name    = VoiceToUtf8(TtsModelName.Name);
	Out.speaker_id        = SpeakerId;
	Out.reference_voice   = StoragePathVoiceToUtf8(ReferenceVoice);
	Out.voice_description = VoiceToUtf8(VoiceDescription);
	Out.speed             = Speed;
	Out.lang              = VoiceToUtf8(Lang);
	Out.expressiveness    = Expressiveness;
	Out.prepare           = Prepare == ETryllVoicePreparation::Lazy
		? ::Tryll::VoicePreparation_Lazy
		: ::Tryll::VoicePreparation_Eager;
	Out.emit_visemes      = bEmitVisemes;
	Out.delivery_modes.reserve(static_cast<size_t>(DeliveryModes.Num()));
	for (const FTryllDeliveryMode& Mode : DeliveryModes)
	{
		auto Wire = std::make_unique<::Tryll::DeliveryModeT>();
		Wire->name            = VoiceToUtf8(Mode.Name);
		Wire->reference_voice = StoragePathVoiceToUtf8(Mode.ReferenceVoice);
		Wire->instruction     = VoiceToUtf8(Mode.Instruction);
		Wire->expressiveness  = Mode.Expressiveness;
		Wire->speed_scale     = Mode.SpeedScale;
		Out.delivery_modes.push_back(MoveTemp(Wire));
	}
	Out.default_delivery_mode = VoiceToUtf8(DefaultDeliveryMode);
}

TArray<FString> FTryllVoiceProfileData::GetModeNames() const
{
	TArray<FString> Names;
	Names.Reserve(DeliveryModes.Num());
	for (const FTryllDeliveryMode& Mode : DeliveryModes)
	{
		Names.Add(Mode.Name);
	}
	return Names;
}

bool FTryllVoiceProfileData::HasMode(const FString& Name) const
{
	if (Name.Equals(NeutralMode, ESearchCase::CaseSensitive))
	{
		return true;
	}
	for (const FTryllDeliveryMode& Mode : DeliveryModes)
	{
		if (Mode.Name.Equals(Name, ESearchCase::CaseSensitive))
		{
			return true;
		}
	}
	return false;
}

bool FTryllVoiceProfileData::IsValidModeName(const FString& Name)
{
	if (Name.IsEmpty() || Name.Len() > 32)
	{
		return false;
	}
	for (const TCHAR C : Name)
	{
		const bool bOk = (C >= TEXT('a') && C <= TEXT('z')) || (C >= TEXT('0') && C <= TEXT('9')) || C == TEXT('_');
		if (!bOk)
		{
			return false;
		}
	}
	return true;
}

void FTryllVoiceProfileData::Validate(const FString& Context, TArray<FTryllWorkflowIssue>& OutIssues, bool bCheckFilesExist) const
{
	using ESeverity = FTryllWorkflowIssue::ESeverity;

	if (TtsModelName.Name.TrimStartAndEnd().IsEmpty())
	{
		OutIssues.Add({ ESeverity::Error,
			FString::Printf(TEXT("%s: Tts Model Name is empty. A voice must name its TTS model; there is no default."), *Context) });
	}
	if (SpeakerId < 0)
	{
		OutIssues.Add({ ESeverity::Error, FString::Printf(TEXT("%s: Speaker Id must be 0 or more."), *Context) });
	}
	if (Speed < 0.1f || Speed > 3.0f)
	{
		OutIssues.Add({ ESeverity::Error, FString::Printf(TEXT("%s: Speed %.2f is outside 0.1-3.0."), *Context, Speed) });
	}
	if (Expressiveness < 0.f || Expressiveness > 2.f)
	{
		OutIssues.Add({ ESeverity::Error,
			FString::Printf(TEXT("%s: Expressiveness %.2f is outside 0.0-2.0."), *Context, Expressiveness) });
	}
	CheckVoiceFreeText(Context, TEXT("VoiceDescription"), VoiceDescription, OutIssues);
	CheckVoiceClip(Context, TEXT("ReferenceVoice"), ReferenceVoice, bCheckFilesExist, OutIssues);

	TSet<FString> Seen;
	for (int32 Index = 0; Index < DeliveryModes.Num(); ++Index)
	{
		const FTryllDeliveryMode& Mode = DeliveryModes[Index];
		const FString Where = FString::Printf(TEXT("%s mode '%s'"), *Context,
			Mode.Name.IsEmpty() ? *FString::Printf(TEXT("#%d"), Index) : *Mode.Name);
		if (!IsValidModeName(Mode.Name))
		{
			OutIssues.Add({ ESeverity::Error, FString::Printf(
				TEXT("%s: the name must be 1-32 characters of lowercase letters, digits and '_'."), *Where) });
		}
		else if (Mode.Name.Equals(NeutralMode, ESearchCase::CaseSensitive))
		{
			OutIssues.Add({ ESeverity::Error, FString::Printf(
				TEXT("%s: \"neutral\" is built in and cannot be defined."), *Where) });
		}
		else if (Seen.Contains(Mode.Name))
		{
			OutIssues.Add({ ESeverity::Error, FString::Printf(TEXT("%s: the name is defined twice."), *Where) });
		}
		Seen.Add(Mode.Name);

		if (Mode.SpeedScale < 0.5f || Mode.SpeedScale > 2.0f)
		{
			OutIssues.Add({ ESeverity::Error,
				FString::Printf(TEXT("%s: Speed Scale %.2f is outside 0.5-2.0."), *Where, Mode.SpeedScale) });
		}
		if (Mode.Expressiveness > 2.f)
		{
			OutIssues.Add({ ESeverity::Error,
				FString::Printf(TEXT("%s: Expressiveness %.2f is above 2.0."), *Where, Mode.Expressiveness) });
		}
		CheckVoiceFreeText(Where, TEXT("Instruction"), Mode.Instruction, OutIssues);
		CheckVoiceClip(Where, TEXT("ReferenceVoice"), Mode.ReferenceVoice, bCheckFilesExist, OutIssues);
	}

	if (!DefaultDeliveryMode.IsEmpty() && !HasMode(DefaultDeliveryMode))
	{
		OutIssues.Add({ ESeverity::Error, FString::Printf(
			TEXT("%s: Default Delivery Mode '%s' is not defined in Delivery Modes (leave it empty for neutral)."),
			*Context, *DefaultDeliveryMode) });
	}
}

void FTryllVoiceProfileData::ValidateAgainst(
	const FTryllTtsCapabilities& Caps, const FString& Context, TArray<FTryllWorkflowIssue>& OutIssues) const
{
	const FString Model = TtsModelName.Name;
	const auto Unsupported = [&](const FString& Where, const TCHAR* Field, const TCHAR* Why)
	{
		OutIssues.Add({ FTryllWorkflowIssue::ESeverity::Error,
			FString::Printf(TEXT("%s: '%s' cannot use %s (%s). Clear it, or pick a model that supports it."),
				*Where, *Model, Field, Why) });
	};

	if (!ReferenceVoice.Path.IsEmpty() && !Caps.bReferenceVoice)
	{
		Unsupported(Context, TEXT("Reference Voice"), TEXT("no voice cloning"));
	}
	if (!VoiceDescription.IsEmpty() && !Caps.bVoiceDescription)
	{
		Unsupported(Context, TEXT("Voice Description"), TEXT("no text-described voices"));
	}
	if (!FMath::IsNearlyEqual(Speed, 1.f) && !Caps.bSpeed)
	{
		Unsupported(Context, TEXT("Speed other than 1.0"), TEXT("no speed control"));
	}
	if (!Lang.IsEmpty() && !Caps.bLanguagePerCall)
	{
		Unsupported(Context, TEXT("Lang"), TEXT("its language is fixed"));
	}
	const int32 SpeakerCount = Caps.NumSpeakers > 0 ? Caps.NumSpeakers : Caps.VoicePresets.Num();
	if (SpeakerCount > 0 && SpeakerId >= SpeakerCount)
	{
		OutIssues.Add({ FTryllWorkflowIssue::ESeverity::Error,
			FString::Printf(TEXT("%s: Speaker Id %d is out of range; '%s' has %d speaker(s) (0-%d)."),
				*Context, SpeakerId, *Model, SpeakerCount, SpeakerCount - 1) });
	}

	for (const FTryllDeliveryMode& Mode : DeliveryModes)
	{
		const FString Where = FString::Printf(TEXT("%s mode '%s'"), *Context, *Mode.Name);
		if (!Mode.ReferenceVoice.Path.IsEmpty() && !Caps.bReferenceVoice)
		{
			Unsupported(Where, TEXT("Reference Voice"), TEXT("no voice cloning"));
		}
		if (!Mode.Instruction.IsEmpty() && !Caps.bDeliveryInstruction)
		{
			Unsupported(Where, TEXT("Instruction"), TEXT("no delivery instructions"));
		}
		if (Mode.Expressiveness >= 0.f && !Caps.bExpressiveness)
		{
			Unsupported(Where, TEXT("Expressiveness"), TEXT("no expressiveness control"));
		}
		if (!FMath::IsNearlyEqual(Mode.SpeedScale, 1.f) && !Caps.bSpeed)
		{
			Unsupported(Where, TEXT("Speed Scale other than 1.0"), TEXT("no speed control"));
		}
	}
}

#if WITH_EDITOR

EDataValidationResult UTryllVoiceProfile::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	TArray<FTryllWorkflowIssue> Issues;
	Voice.Validate(GetPathName(), Issues);
	for (const FTryllWorkflowIssue& Issue : Issues)
	{
		if (Issue.Severity == FTryllWorkflowIssue::ESeverity::Error)
		{
			Context.AddError(FText::FromString(Issue.Message));
			Result = EDataValidationResult::Invalid;
		}
		else
		{
			Context.AddWarning(FText::FromString(Issue.Message));
		}
	}

	for (const FTryllWorkflowModelViolation& V : FTryllWorkflowModelRefs::FindNonProductionVoiceRefs(Voice, UTryllModelManifest::GetActive()))
	{
		Context.AddError(FText::FromString(V.ToString(GetPathName())));
		Result = EDataValidationResult::Invalid;
	}

	if (Result == EDataValidationResult::NotValidated)
	{
		Result = EDataValidationResult::Valid;
	}
	return Result;
}

void UTryllVoiceProfile::PreSave(FObjectPreSaveContext ObjectSaveContext)
{
	Super::PreSave(ObjectSaveContext);
	if (!ObjectSaveContext.IsCooking())
	{
		return;
	}

	// Cook / package: Error-level logs fail the cook, like UTryllWorkflowAsset.
	for (const FTryllWorkflowModelViolation& V : FTryllWorkflowModelRefs::FindNonProductionVoiceRefs(Voice, UTryllModelManifest::GetActive()))
	{
		UE_LOG(LogTryll, Error,
			TEXT("[Tryll] Cook aborted — voice profile references a non-Production model: %s. ")
			TEXT("Mark it Production in the Model Manager, or change the voice's model."),
			*V.ToString(GetPathName()));
	}
	TArray<FTryllWorkflowIssue> Issues;
	Voice.Validate(GetPathName(), Issues);
	for (const FTryllWorkflowIssue& Issue : Issues)
	{
		if (Issue.Severity == FTryllWorkflowIssue::ESeverity::Error)
		{
			UE_LOG(LogTryll, Error, TEXT("[Tryll] Cook aborted — invalid voice profile: %s"), *Issue.Message);
		}
	}
}

#endif // WITH_EDITOR
