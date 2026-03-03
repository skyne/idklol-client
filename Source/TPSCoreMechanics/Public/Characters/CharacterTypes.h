// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CharacterTypes.generated.h"

/**
 * Native character enums and data structures — no gRPC dependency.
 * These mirror the proto-defined types in CharactersMessage.h but are
 * transport-agnostic so both the game server and the client can use them.
 * The TPSCoreMechanicsClient module owns the mappers between these and gRPC types.
 */

UENUM(BlueprintType)
enum class ECharacterRace : uint8
{
	Unknown = 0,
	Human   = 1,
	Elf     = 2,
	Dwarf   = 3,
	Orc     = 4,
};

UENUM(BlueprintType)
enum class ECharacterGender : uint8
{
	Unknown = 0,
	Male    = 1,
	Female  = 2,
};

UENUM(BlueprintType)
enum class ECharacterSkinColor : uint8
{
	Unknown = 0,
	Pale    = 1,
	Fair    = 2,
	Tan     = 3,
	Brown   = 4,
	Dark    = 5,
	Green   = 6,
	Gray    = 7,
};

UENUM(BlueprintType)
enum class ECharacterClass : uint8
{
	Unknown = 0,
	Warrior = 1,
	Mage    = 2,
	Rogue   = 3,
	Cleric  = 4,
	Ranger  = 5,
};

/**
 * Transport-agnostic character data struct.
 * Populated from gRPC on the client side (via FCharacterGrpcMapper),
 * or from NATS/JSON on the server side (via FCharacterNatsMapper, future).
 */
USTRUCT(BlueprintType)
struct TPSCOREMECHANICS_API FCharacterData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Character")
	FString CharacterId = TEXT("");

	UPROPERTY(BlueprintReadWrite, Category = "Character")
	FString Name = TEXT("");

	UPROPERTY(BlueprintReadWrite, Category = "Character")
	ECharacterRace Race = ECharacterRace::Unknown;

	UPROPERTY(BlueprintReadWrite, Category = "Character")
	ECharacterGender Gender = ECharacterGender::Unknown;

	UPROPERTY(BlueprintReadWrite, Category = "Character")
	ECharacterSkinColor SkinColor = ECharacterSkinColor::Unknown;

	UPROPERTY(BlueprintReadWrite, Category = "Character")
	ECharacterClass CharacterClass = ECharacterClass::Unknown;

	UPROPERTY(BlueprintReadWrite, Category = "Character")
	FString CreatedAt = TEXT("");
};
