// Fill out your copyright notice in the Description page of Project Settings.

#include "CharacterCreation/CharacterCreationMapper.h"

#include "SCharacters/CharactersMessage.h"

FString FCharacterCreationMapper::RaceEnumToString(EGrpcCharactersRace Race)
{
	switch (Race)
	{
	case EGrpcCharactersRace::HUMAN:
		return TEXT("Human");
	case EGrpcCharactersRace::ELF:
		return TEXT("Elf");
	case EGrpcCharactersRace::DWARF:
		return TEXT("Dwarf");
	case EGrpcCharactersRace::ORC:
		return TEXT("Orc");
	// case EGrpcCharactersRace::HALFLING: // Not defined in current enum
	// 	return TEXT("Halfling");
	default:
		return TEXT("Unknown");
	}
}

FString FCharacterCreationMapper::GenderEnumToString(EGrpcCharactersGender Gender)
{
	switch (Gender)
	{
	case EGrpcCharactersGender::MALE:
		return TEXT("Male");
	case EGrpcCharactersGender::FEMALE:
		return TEXT("Female");
	default:
		return TEXT("Unknown");
	}
}

FString FCharacterCreationMapper::ClassEnumToString(EGrpcCharactersCharacterClass CharacterClass)
{
	switch (CharacterClass)
	{
	case EGrpcCharactersCharacterClass::WARRIOR:
		return TEXT("Warrior");
	case EGrpcCharactersCharacterClass::MAGE:
		return TEXT("Mage");
	case EGrpcCharactersCharacterClass::ROGUE:
		return TEXT("Rogue");
	case EGrpcCharactersCharacterClass::CLERIC:
		return TEXT("Cleric");
	case EGrpcCharactersCharacterClass::RANGER:
		return TEXT("Ranger");
	default:
		return TEXT("Unknown");
	}
}

FString FCharacterCreationMapper::SkinColorEnumToString(EGrpcCharactersSkinColor SkinColor)
{
	switch (SkinColor)
	{
	case EGrpcCharactersSkinColor::PALE:
		return TEXT("Pale");
	case EGrpcCharactersSkinColor::FAIR:
		return TEXT("Fair");
	case EGrpcCharactersSkinColor::TAN:
		return TEXT("Tan");
	case EGrpcCharactersSkinColor::BROWN:
		return TEXT("Brown");
	case EGrpcCharactersSkinColor::DARK:
		return TEXT("Dark");
	case EGrpcCharactersSkinColor::GREEN:
		return TEXT("Green");
	case EGrpcCharactersSkinColor::GRAY:
		return TEXT("Gray");
	default:
		return TEXT("Unknown");
	}
}
