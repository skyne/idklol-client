// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "NPC/NPCCharacter.h"
#include "TPSCoreGameMode.generated.h"

class UCharacterClassInfo;
class ATPSCoreMechanicsCharacter;

UCLASS()
class TPSCOREMECHANICS_API ATPSCoreGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ATPSCoreGameMode();

	virtual void BeginPlay() override;

	/**
	 * Validates AuthToken from travel options before accepting the connection.
	 *
	 * NOTE: this currently validates payload shape/claims only (email extraction),
	 * not cryptographic signature verification.
	 */
	virtual void PreLogin(const FString& Options, const FString& Address,
		const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	/** Extracts CharacterId (and OwnerEmail from JWT claims) and stores them for PostLogin. */
	virtual APlayerController* Login(
		UPlayer* NewPlayer,
		ENetRole InRemoteRole,
		const FString& Portal,
		const FString& Options,
		const FUniqueNetIdRepl& UniqueId,
		FString& ErrorMessage) override;

	/**
	 * Called after pawn is spawned and possessed.
	 * Triggers async NATS fetch of character data and calls
	 * ATPSCoreMechanicsCharacter::InitializeFromCharacterData once it arrives.
	 */
	virtual void PostLogin(APlayerController* NewPlayer) override;

	UCharacterClassInfo* GetCharacterClassDefaultInfo() const;

	/** Zone identifier sent to npc-metadata-service to fetch this map's NPCs. */
	UFUNCTION(BlueprintPure, Category = "NPC")
	const FString& GetZoneId() const { return ZoneId; }

	/**
	 * NPC actor class spawned by UServerNpcManagerSubsystem.
	 * Defaults to ANPCCharacter. Override with a Blueprint subclass to apply
	 * custom mesh, animations, or AI behaviour.
	 */
	UFUNCTION(BlueprintPure, Category = "NPC")
	TSubclassOf<ANPCCharacter> GetNpcClass() const { return NpcClass; }

private:
	UPROPERTY(EditDefaultsOnly, Category="Custom Values|Class Defaults")
	TObjectPtr<UCharacterClassInfo> ClassDefaults;

	/**
	 * Zone id forwarded to npc.meta.by_zone on BeginPlay.
	 * Leave empty to fall back to the map name.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "NPC")
	FString ZoneId;

	/**
	 * NPC actor class to spawn. Defaults to ANPCCharacter (C++ base).
	 * Set to a Blueprint subclass (e.g. BP_NPCCharacter) when you need
	 * custom meshes or animations driven from the editor.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "NPC")
	TSubclassOf<ANPCCharacter> NpcClass;

	/** Temporary: CharacterId stored during Login, consumed in PostLogin. */
	TMap<FObjectKey, FString> PendingCharacterIds;

	/** Email from the connecting player's JWT, stored during Login, consumed in PostLogin. */
	TMap<FObjectKey, FString> PendingOwnerEmails;
};
