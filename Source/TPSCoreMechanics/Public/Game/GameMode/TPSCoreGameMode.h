// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
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
	 * Extracts CharacterId from URL options and stores it for use in PostLogin.
	 * Called before the pawn is spawned.
	 */
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

private:
	UPROPERTY(EditDefaultsOnly, Category="Custom Values|Class Defaults")
	TObjectPtr<UCharacterClassInfo> ClassDefaults;

	/** Temporary: CharacterId stored during Login, consumed in PostLogin. */
	TMap<FObjectKey, FString> PendingCharacterIds;
};
