// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/CharacterTypes.h"
#include "SCharacters/CharactersMessage.h"

/**
 * Stateless mapper between proto-generated gRPC character types and the
 * transport-agnostic native types defined in CharacterTypes.h.
 *
 * Lives exclusively in TPSCoreMechanicsClient — the server never sees this file.
 */
struct TPSCOREMECHANICSCLIENT_API FCharacterGrpcMapper
{
	// ── gRPC → Native ────────────────────────────────────────────────────────

	static ECharacterRace      ToNative(EGrpcCharactersRace Race);
	static ECharacterGender    ToNative(EGrpcCharactersGender Gender);
	static ECharacterSkinColor ToNative(EGrpcCharactersSkinColor SkinColor);
	static ECharacterClass     ToNative(EGrpcCharactersCharacterClass Class);

	/** Convert a full gRPC character message to the native struct. */
	static FCharacterData ToNative(const FGrpcCharactersCharacter& GrpcCharacter);

	// ── Native → gRPC ────────────────────────────────────────────────────────

	static EGrpcCharactersRace           ToGrpc(ECharacterRace Race);
	static EGrpcCharactersGender         ToGrpc(ECharacterGender Gender);
	static EGrpcCharactersSkinColor      ToGrpc(ECharacterSkinColor SkinColor);
	static EGrpcCharactersCharacterClass ToGrpc(ECharacterClass Class);

	/**
	 * Build a CreateCharacterRequest from a native FCharacterData.
	 * The Name field on the request is taken from CharacterData.Name.
	 */
	static FGrpcCharactersCreateCharacterRequest ToCreateRequest(const FCharacterData& CharacterData);
};
