// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/CharacterGrpcMapper.h"

// ── gRPC → Native ─────────────────────────────────────────────────────────────

ECharacterRace FCharacterGrpcMapper::ToNative(EGrpcCharactersRace Race)
{
	switch (Race)
	{
	case EGrpcCharactersRace::HUMAN: return ECharacterRace::Human;
	case EGrpcCharactersRace::ELF:   return ECharacterRace::Elf;
	case EGrpcCharactersRace::DWARF: return ECharacterRace::Dwarf;
	case EGrpcCharactersRace::ORC:   return ECharacterRace::Orc;
	default:                         return ECharacterRace::Unknown;
	}
}

ECharacterGender FCharacterGrpcMapper::ToNative(EGrpcCharactersGender Gender)
{
	switch (Gender)
	{
	case EGrpcCharactersGender::MALE:   return ECharacterGender::Male;
	case EGrpcCharactersGender::FEMALE: return ECharacterGender::Female;
	default:                            return ECharacterGender::Unknown;
	}
}

ECharacterSkinColor FCharacterGrpcMapper::ToNative(EGrpcCharactersSkinColor SkinColor)
{
	switch (SkinColor)
	{
	case EGrpcCharactersSkinColor::PALE:  return ECharacterSkinColor::Pale;
	case EGrpcCharactersSkinColor::FAIR:  return ECharacterSkinColor::Fair;
	case EGrpcCharactersSkinColor::TAN:   return ECharacterSkinColor::Tan;
	case EGrpcCharactersSkinColor::BROWN: return ECharacterSkinColor::Brown;
	case EGrpcCharactersSkinColor::DARK:  return ECharacterSkinColor::Dark;
	case EGrpcCharactersSkinColor::GREEN: return ECharacterSkinColor::Green;
	case EGrpcCharactersSkinColor::GRAY:  return ECharacterSkinColor::Gray;
	default:                              return ECharacterSkinColor::Unknown;
	}
}

ECharacterClass FCharacterGrpcMapper::ToNative(EGrpcCharactersCharacterClass Class)
{
	switch (Class)
	{
	case EGrpcCharactersCharacterClass::WARRIOR: return ECharacterClass::Warrior;
	case EGrpcCharactersCharacterClass::MAGE:    return ECharacterClass::Mage;
	case EGrpcCharactersCharacterClass::ROGUE:   return ECharacterClass::Rogue;
	case EGrpcCharactersCharacterClass::CLERIC:  return ECharacterClass::Cleric;
	case EGrpcCharactersCharacterClass::RANGER:  return ECharacterClass::Ranger;
	default:                                     return ECharacterClass::Unknown;
	}
}

FCharacterData FCharacterGrpcMapper::ToNative(const FGrpcCharactersCharacter& GrpcCharacter)
{
	FCharacterData Out;
	Out.CharacterId  = GrpcCharacter.CharacterId;
	Out.Name         = GrpcCharacter.Name;
	Out.Race         = ToNative(GrpcCharacter.Race);
	Out.Gender       = ToNative(GrpcCharacter.Gender);
	Out.SkinColor    = ToNative(GrpcCharacter.SkinColor);
	Out.CharacterClass = ToNative(GrpcCharacter.CharacterClass);
	Out.CreatedAt    = GrpcCharacter.CreatedAt;
	return Out;
}

// ── Native → gRPC ─────────────────────────────────────────────────────────────

EGrpcCharactersRace FCharacterGrpcMapper::ToGrpc(ECharacterRace Race)
{
	switch (Race)
	{
	case ECharacterRace::Human: return EGrpcCharactersRace::HUMAN;
	case ECharacterRace::Elf:   return EGrpcCharactersRace::ELF;
	case ECharacterRace::Dwarf: return EGrpcCharactersRace::DWARF;
	case ECharacterRace::Orc:   return EGrpcCharactersRace::ORC;
	default:                    return EGrpcCharactersRace::RACE_UNSPECIFIED;
	}
}

EGrpcCharactersGender FCharacterGrpcMapper::ToGrpc(ECharacterGender Gender)
{
	switch (Gender)
	{
	case ECharacterGender::Male:   return EGrpcCharactersGender::MALE;
	case ECharacterGender::Female: return EGrpcCharactersGender::FEMALE;
	default:                       return EGrpcCharactersGender::GENDER_UNSPECIFIED;
	}
}

EGrpcCharactersSkinColor FCharacterGrpcMapper::ToGrpc(ECharacterSkinColor SkinColor)
{
	switch (SkinColor)
	{
	case ECharacterSkinColor::Pale:  return EGrpcCharactersSkinColor::PALE;
	case ECharacterSkinColor::Fair:  return EGrpcCharactersSkinColor::FAIR;
	case ECharacterSkinColor::Tan:   return EGrpcCharactersSkinColor::TAN;
	case ECharacterSkinColor::Brown: return EGrpcCharactersSkinColor::BROWN;
	case ECharacterSkinColor::Dark:  return EGrpcCharactersSkinColor::DARK;
	case ECharacterSkinColor::Green: return EGrpcCharactersSkinColor::GREEN;
	case ECharacterSkinColor::Gray:  return EGrpcCharactersSkinColor::GRAY;
	default:                         return EGrpcCharactersSkinColor::SKIN_COLOR_UNSPECIFIED;
	}
}

EGrpcCharactersCharacterClass FCharacterGrpcMapper::ToGrpc(ECharacterClass Class)
{
	switch (Class)
	{
	case ECharacterClass::Warrior: return EGrpcCharactersCharacterClass::WARRIOR;
	case ECharacterClass::Mage:    return EGrpcCharactersCharacterClass::MAGE;
	case ECharacterClass::Rogue:   return EGrpcCharactersCharacterClass::ROGUE;
	case ECharacterClass::Cleric:  return EGrpcCharactersCharacterClass::CLERIC;
	case ECharacterClass::Ranger:  return EGrpcCharactersCharacterClass::RANGER;
	default:                       return EGrpcCharactersCharacterClass::CLASS_UNSPECIFIED;
	}
}

FGrpcCharactersCreateCharacterRequest FCharacterGrpcMapper::ToCreateRequest(const FCharacterData& CharacterData)
{
	FGrpcCharactersCreateCharacterRequest Request;
	Request.Name           = CharacterData.Name;
	Request.Race           = ToGrpc(CharacterData.Race);
	Request.Gender         = ToGrpc(CharacterData.Gender);
	Request.SkinColor      = ToGrpc(CharacterData.SkinColor);
	Request.CharacterClass = ToGrpc(CharacterData.CharacterClass);
	return Request;
}
