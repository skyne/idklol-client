// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SCharacters/CharactersService.h"
#include "CharactersSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class TPSCOREMECHANICS_API UCharactersSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
private:
	UPROPERTY()
	UCharacterServiceClient* Client;
	UPROPERTY()
	UCharacterService* CharacterService;
	
	void InitializeConnection();
	
public:
	void Initialize(FSubsystemCollectionBase& Collection) override;
};
