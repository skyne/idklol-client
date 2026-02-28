// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GrpcHandlerSubsystem.h"
#include "SCharacters/CharactersService.h"
#include "CharactersSubsystem.generated.h"

/**
 * Characters subsystem - handles character service gRPC connection
 */
UCLASS(config=Game)
class TPSCOREMECHANICS_API UCharactersSubsystem : public UGrpcHandlerSubsystem
{
	GENERATED_BODY()
	
protected:
	DECLARE_GRPC_SUBSYSTEM_TYPES(CharacterService)
	
	// Override base class methods
	virtual void OnServiceConnected(UObject* InService, UObject* InClient) override;
	virtual void OnServiceDisconnected() override;
	
public:
	
	// Get character creation catalog asynchronously (C++ only, not exposed to Blueprints)
	TFuture<FGrpcCharactersCharacterCreationCatalog> GetCharacterCreationOptionCatalogAsync();
	
	// Create a new character asynchronously
	TFuture<FGrpcCharactersCreateCharacterResponse> CreateCharacterAsync(const FGrpcCharactersCreateCharacterRequest& Request);
	
	// List all created characters for the current user
	TFuture<FGrpcCharactersListCreatedCharactersResponse> ListCreatedCharactersAsync();
};
