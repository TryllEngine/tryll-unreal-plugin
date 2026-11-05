// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllTextProvider.h"

#include "TryllRuntimeSettings.h"

bool UTryllTextProvider::GetText_Implementation(FName /*Table*/, const FString& /*Id*/, FString& OutText) const
{
	OutText.Reset();
	return false;
}

FString UTryllTextProvider::GetCurrentLocale_Implementation() const
{
	return FString();
}

UTryllTextProvider* UTryllTextProvider::Get()
{
	static TWeakObjectPtr<UTryllTextProvider> Cached;
	const UTryllRuntimeSettings* Settings = GetDefault<UTryllRuntimeSettings>();
	UClass* Class = Settings ? Settings->TextProviderClass.Get() : nullptr;
	if (Class == nullptr)
	{
		if (UTryllTextProvider* Existing = Cached.Get())
		{
			Existing->RemoveFromRoot();
			Cached.Reset();
		}
		return nullptr;
	}

	if (UTryllTextProvider* Existing = Cached.Get())
	{
		if (Existing->GetClass() == Class)
		{
			return Existing;
		}
		Existing->RemoveFromRoot();
		Cached.Reset();
	}

	UTryllTextProvider* Created = NewObject<UTryllTextProvider>(GetTransientPackage(), Class);
	Created->AddToRoot();
	Cached = Created;
	return Created;
}

FSimpleMulticastDelegate& UTryllTextProvider::OnLocaleChanged()
{
	static FSimpleMulticastDelegate Delegate;
	return Delegate;
}

void UTryllTextProvider::BroadcastLocaleChanged()
{
	OnLocaleChanged().Broadcast();
}

FString UTryllTextProvider::CurrentProviderLocale()
{
	UTryllTextProvider* Provider = Get();
	return Provider ? Provider->GetCurrentLocale() : FString();
}

bool UTryllTextProvider::LocaleForLanguageChange(bool bSessionLocaleExplicit, FString& OutLocale)
{
	OutLocale.Reset();
	if (bSessionLocaleExplicit || Get() == nullptr)
	{
		return false;
	}
	OutLocale = ResolveSessionLocale();
	return !OutLocale.IsEmpty();
}

FString UTryllTextProvider::ResolveSessionLocale()
{
	if (UTryllTextProvider* Provider = Get())
	{
		const FString FromProvider = Provider->GetCurrentLocale();
		if (!FromProvider.IsEmpty())
		{
			return FromProvider;
		}
	}
	const UTryllRuntimeSettings* Settings = GetDefault<UTryllRuntimeSettings>();
	return Settings ? Settings->Locale : FString();
}

FTryllTextPreview FTryllTextPreview::Evaluate(
	const UTryllTextProvider* Provider, FName Table, const FString& Id, const FString& PreviewLocale)
{
	FTryllTextPreview Result;
	if (Provider == nullptr || !Provider->SupportsEditorPreview())
	{
		const FString Name = Provider ? Provider->GetClass()->GetName() : TEXT("none");
		Result.Kind = EKind::NoEditorSupport;
		Result.Message = FString::Printf(TEXT("No preview — %s has no editor support."), *Name);
		return Result;
	}

	if (Id.IsEmpty())
	{
		Result.Kind = EKind::EmptyId;
		Result.Message = TEXT("Enter a Text ID");
		return Result;
	}

	FString Fallback;
	for (const FString& Locale : Provider->ListLocales())
	{
		FString Text;
		if (!Provider->GetTextForPreview(Table, Id, Locale, Text))
		{
			Result.MissingLocales.Add(Locale);
		}
		else if (Fallback.IsEmpty() && !Locale.Equals(PreviewLocale, ESearchCase::IgnoreCase))
		{
			Fallback = Text;
		}
	}

	FString Exact;
	if (!Provider->GetTextForPreview(Table, Id, PreviewLocale, Exact))
	{
		Result.Kind = EKind::MissingHere;
		Result.Text = Fallback;
		Result.bIsFallback = !Fallback.IsEmpty();
		Result.Message = FString::Printf(
			TEXT("`%s` not found in table `%s` (%s)"), *Id, *Table.ToString(), *PreviewLocale);
		return Result;
	}

	Result.Kind = EKind::Found;
	Result.Text = Exact;
	return Result;
}
