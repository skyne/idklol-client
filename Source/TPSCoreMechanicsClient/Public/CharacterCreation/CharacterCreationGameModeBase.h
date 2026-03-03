// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CharacterCreationGameModeBase.generated.h"

class ACharacterSelectionCharacter;

/**
 * Base game mode for character creation and selection
 * Provides configuration for transitioning to the game world
 */
UCLASS()
class TPSCOREMECHANICSCLIENT_API ACharacterCreationGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	ACharacterCreationGameModeBase();
	
	virtual void BeginPlay() override;
	/**
	 * The map to load when a character is selected and ready to enter the game world
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Creation")
	TSoftObjectPtr<UWorld> GameWorldMap;
	
	/**
	 * The game mode to use when entering the game world
	 * If not set, the map's default game mode will be used
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Creation")
	TSubclassOf<AGameModeBase> GameWorldGameMode;
	
	/**
	 * Get the game world transition URL (includes map and game mode override)
	 * @return The URL to use for OpenLevel (e.g., "MapName?game=/Path/To/GameMode")
	 */
	UFUNCTION(BlueprintCallable, Category="Character Creation")
	FString GetGameWorldTransitionURL() const;
};
