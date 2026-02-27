// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * Generic key-value pair template structure
 * Can be used with any type combination (FString/FString, FString/int, uint8/uint8, etc.)
 * Not exposed to Blueprints - for C++ use only
 */
template<typename TKey, typename TValue>
struct TKVP
{
	TKey Key;
	TValue Value;
	
	TKVP() = default;
	TKVP(const TKey& InKey, const TValue& InValue) : Key(InKey), Value(InValue) {}
};

// Common type aliases for frequently used key-value pair combinations
using FStringKVP = TKVP<FString, FString>;           // String to String mapping
using FStringIntKVP = TKVP<FString, int32>;          // String to Integer mapping
using FStringByteKVP = TKVP<FString, uint8>;         // String to Byte mapping
using FStringFloatKVP = TKVP<FString, float>;        // String to Float mapping
using FIntKVP = TKVP<int32, int32>;                  // Integer to Integer mapping
using FByteKVP = TKVP<uint8, uint8>;                 // Byte to Byte mapping

/**
 * Character creation restriction combinations
 * Used to define valid combinations of race/gender/class/skincolor
 */
struct FRaceGenderCombination
{
	uint8 Race;
	uint8 Gender;
	
	FRaceGenderCombination() : Race(0), Gender(0) {}
	FRaceGenderCombination(uint8 InRace, uint8 InGender) : Race(InRace), Gender(InGender) {}
};

struct FRaceGenderSkinColorCombination
{
	uint8 Race;
	uint8 Gender;
	uint8 SkinColor;
	
	FRaceGenderSkinColorCombination() : Race(0), Gender(0), SkinColor(0) {}
	FRaceGenderSkinColorCombination(uint8 InRace, uint8 InGender, uint8 InSkinColor) 
		: Race(InRace), Gender(InGender), SkinColor(InSkinColor) {}
};

struct FRaceGenderClassCombination
{
	uint8 Race;
	uint8 Gender;
	uint8 CharacterClass;
	
	FRaceGenderClassCombination() : Race(0), Gender(0), CharacterClass(0) {}
	FRaceGenderClassCombination(uint8 InRace, uint8 InGender, uint8 InClass) 
		: Race(InRace), Gender(InGender), CharacterClass(InClass) {}
};
