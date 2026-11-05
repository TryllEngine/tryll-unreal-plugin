// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TryllModelInfo.h"

#include "TryllLanguages.generated.h"

/**
 * Works out which languages your game can actually offer, from the models it
 * actually uses.
 *
 * The rule is an intersection: a language is offerable only if *every* model in
 * the pipeline covers it. A game with a 31-language voice but an English-only
 * recognizer can offer exactly one language for hands-free play - and the point
 * of this library is that you find that out from the catalog instead of from a
 * bug report.
 *
 * Blueprint-callable, so a settings menu can populate itself without C++.
 */
UCLASS()
class TRYLLCLIENT_API UTryllLanguages : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Languages every model in Models declares, sorted.
	 *
	 * A model that declares nothing is **skipped, not treated as restrictive** -
	 * an empty list means "the catalog didn't say", and letting unknown shrink
	 * the result would hide languages that work.
	 *
	 * An empty return therefore means one of two things: the declared sets
	 * genuinely do not overlap, or nothing declared anything. Call
	 * HasLanguageInfo to tell those apart before telling a player there are no
	 * languages available.
	 */
	UFUNCTION(BlueprintPure, Category = "Tryll|Languages")
	static TArray<FString> OfferableLanguages(const TArray<FTryllModelInfo>& Models);

	/**
	 * Same, but picks the models out of a ListModels result by name - the usual
	 * shape, since you know your TTS/STT/language model names. Names that are
	 * not present are ignored, so a model the player has not downloaded yet does
	 * not silently empty the menu.
	 */
	UFUNCTION(BlueprintPure, Category = "Tryll|Languages")
	static TArray<FString> OfferableLanguagesForModels(
		const TArray<FTryllModelInfo>& AllModels,
		const TArray<FString>&         ModelNames);

	/**
	 * True when at least one model declared its languages. Distinguishes "these
	 * models share no language" from "we have no idea", which are very different
	 * things to show a player.
	 */
	UFUNCTION(BlueprintPure, Category = "Tryll|Languages")
	static bool HasLanguageInfo(const TArray<FTryllModelInfo>& Models);

	/**
	 * Whether Model covers Locale (a tag like "de-DE" or a bare code like "de").
	 *
	 * Returns true for a model that declares nothing: without a catalog claim we
	 * have no evidence either way, and the server applies the same rule - it
	 * withholds the language hint rather than refusing to load the model.
	 */
	UFUNCTION(BlueprintPure, Category = "Tryll|Languages")
	static bool ModelSupportsLanguage(const FTryllModelInfo& Model, const FString& Locale);

	/**
	 * Primary language subtag of a locale, lowercase: "pt-BR" -> "pt". This is
	 * the granularity models work at - none of the shipped voice or speech
	 * models distinguish pt-BR from pt-PT.
	 */
	UFUNCTION(BlueprintPure, Category = "Tryll|Languages")
	static FString LanguageOf(const FString& Locale);

	/**
	 * English name for a language code - "de" -> "German" - for labelling a
	 * menu. Unknown codes come back unchanged, so an entry is never blank.
	 *
	 * Deliberately a fixed table rather than FInternationalization: it must
	 * match what the server renders into the {{language}} prompt key exactly
	 * (see SessionLocale.cpp). Keep the two lists in sync when adding a language.
	 */
	UFUNCTION(BlueprintPure, Category = "Tryll|Languages")
	static FString LanguageDisplayName(const FString& Code);
};
