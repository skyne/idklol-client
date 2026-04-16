// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Characters/CharacterTypes.h"
#include "ServerCharacterLoaderSubsystem.generated.h"

DECLARE_DELEGATE_TwoParams(FOnCharacterLoaded, bool /*bSuccess*/, const FCharacterData& /*Character*/);

/**
 * Server-only subsystem that fetches character data from the character service via NATS
 * when a player joins. The result is used to call ATPSCoreMechanicsCharacter::InitializeFromCharacterData
 * authoritatively on the server so replication distributes appearance to all clients.
 *
 * Only active on dedicated server (guarded by IsRunningDedicatedServer() in Initialize).
 * NATS subject is configured in /Script/TPSCoreMechanics.TPSNatsSubjectsConfig
 * (CharactersGetSubject="characters.get" by default) — request payload: {"id":"<CharacterId>"}
 */
UCLASS()
class TPSCOREMECHANICS_API UServerCharacterLoaderSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	/**
	 * Asynchronously fetch a character by ID from the character service via NATS.
	 * Callback is invoked on the game thread.
	 * @param CharacterId  The UUID of the character to load.
	 * @param Callback     Called with bSuccess=false and an empty FCharacterData on timeout.
	 */
	void FetchCharacter(const FString& CharacterId, FOnCharacterLoaded Callback);

private:
};
