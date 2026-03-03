// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonTypes.h"
#include "SCharacters/CharactersMessage.h"

/**
 * Helper class for mapping gRPC character creation catalog data to UI-friendly formats
 */
class TPSCOREMECHANICSCLIENT_API FCharacterCreationMapper
{
public:
	/**
	 * Generic mapper function to convert an array of source objects to an array of KVP
	 * @param Source The source array to map from
	 * @param KeyExtractor Function to extract the key from each source item
	 * @param ValueExtractor Function to extract the value from each source item
	 * @return Array of TKVP with extracted keys and values
	 */
	template<typename TSource, typename TKey, typename TValue, typename TKeyFunc, typename TValueFunc>
	static TArray<TKVP<TKey, TValue>> MapToKVP(
		const TArray<TSource>& Source,
		TKeyFunc KeyExtractor,
		TValueFunc ValueExtractor)
	{
		TArray<TKVP<TKey, TValue>> Result;
		Result.Reserve(Source.Num());
		
		for (const auto& Item : Source)
		{
			Result.Add(TKVP<TKey, TValue>(KeyExtractor(Item), ValueExtractor(Item)));
		}
		
		return Result;
	}
	
	/**
	 * Convenience wrapper for mapping to FStringKVP (most common case)
	 */
	template<typename TSource, typename TKeyFunc, typename TValueFunc>
	static TArray<FStringKVP> MapToStringKVP(
		const TArray<TSource>& Source,
		TKeyFunc KeyExtractor,
		TValueFunc ValueExtractor)
	{
		return MapToKVP<TSource, FString, FString>(Source, KeyExtractor, ValueExtractor);
	}
	
	/**
	 * Convenience wrapper for mapping to FStringByteKVP
	 */
	template<typename TSource, typename TKeyFunc, typename TValueFunc>
	static TArray<FStringByteKVP> MapToStringByteKVP(
		const TArray<TSource>& Source,
		TKeyFunc KeyExtractor,
		TValueFunc ValueExtractor)
	{
		return MapToKVP<TSource, FString, uint8>(Source, KeyExtractor, ValueExtractor);
	}
	
	/**
	 * Convenience wrapper for mapping to FStringIntKVP
	 */
	template<typename TSource, typename TKeyFunc, typename TValueFunc>
	static TArray<FStringIntKVP> MapToStringIntKVP(
		const TArray<TSource>& Source,
		TKeyFunc KeyExtractor,
		TValueFunc ValueExtractor)
	{
		return MapToKVP<TSource, FString, int32>(Source, KeyExtractor, ValueExtractor);
	}
	
	/**
	 * Map gRPC race-gender combinations to simple struct
	 */
	template<typename TSource>
	static TArray<FRaceGenderCombination> MapToRaceGenderCombinations(
		const TArray<TSource>& Source,
		auto RaceExtractor,
		auto GenderExtractor)
	{
		TArray<FRaceGenderCombination> Result;
		Result.Reserve(Source.Num());
		
		for (const auto& Item : Source)
		{
			Result.Add(FRaceGenderCombination(
				static_cast<uint8>(RaceExtractor(Item)),
				static_cast<uint8>(GenderExtractor(Item))
			));
		}
		
		return Result;
	}
	
	/**
	 * Map gRPC race-gender-skincolor combinations to simple struct
	 */
	template<typename TSource>
	static TArray<FRaceGenderSkinColorCombination> MapToRaceGenderSkinColorCombinations(
		const TArray<TSource>& Source,
		auto RaceExtractor,
		auto GenderExtractor,
		auto SkinColorExtractor)
	{
		TArray<FRaceGenderSkinColorCombination> Result;
		Result.Reserve(Source.Num());
		
		for (const auto& Item : Source)
		{
			Result.Add(FRaceGenderSkinColorCombination(
				static_cast<uint8>(RaceExtractor(Item)),
				static_cast<uint8>(GenderExtractor(Item)),
				static_cast<uint8>(SkinColorExtractor(Item))
			));
		}
		
		return Result;
	}
	
	/**
	 * Map gRPC race-gender-class combinations to simple struct
	 */
	template<typename TSource>
	static TArray<FRaceGenderClassCombination> MapToRaceGenderClassCombinations(
		const TArray<TSource>& Source,
		auto RaceExtractor,
		auto GenderExtractor,
		auto ClassExtractor)
	{
		TArray<FRaceGenderClassCombination> Result;
		Result.Reserve(Source.Num());
		
		for (const auto& Item : Source)
		{
			Result.Add(FRaceGenderClassCombination(
				static_cast<uint8>(RaceExtractor(Item)),
				static_cast<uint8>(GenderExtractor(Item)),
				static_cast<uint8>(ClassExtractor(Item))
			));
		}
		
		return Result;
	}
	
	// Enum to string conversion helpers for UI display
	static FString RaceEnumToString(EGrpcCharactersRace Race);
	static FString GenderEnumToString(EGrpcCharactersGender Gender);
	static FString ClassEnumToString(EGrpcCharactersCharacterClass CharacterClass);
	static FString SkinColorEnumToString(EGrpcCharactersSkinColor SkinColor);
};
