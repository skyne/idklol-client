// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SCharacters/CharactersMessage.h"
#include "GrpcHandlerSubsystem.h"
#include "CharacterSelection.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterSelectedForPlay, FString, CharacterId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRequestCreateNew);

/**
 * Actor that manages character selection screen logic
 */
UCLASS()
class TPSCOREMECHANICSCLIENT_API ACharacterSelection : public AActor
{
	GENERATED_BODY()
	
public:	
	ACharacterSelection();
	
	// Delegate broadcast when a character is selected for play
	UPROPERTY(BlueprintAssignable, Category="Character Selection")
	FOnCharacterSelectedForPlay OnCharacterSelectedForPlay;
	
	// Delegate broadcast when user requests to create a new character
	UPROPERTY(BlueprintAssignable, Category="Character Selection")
	FOnRequestCreateNew OnRequestCreateNew;
	
	// Refresh the list of characters from the server
	UFUNCTION(BlueprintCallable, Category="Character Selection")
	void RefreshCharacterList();
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	// Event handlers for UI interactions
	UFUNCTION()
	void HandleCharacterSelected(FString CharacterId);
	
	UFUNCTION()
	void HandleCreateNewCharacter();
	
	UFUNCTION()
	void HandleDeleteCharacter(FString CharacterId);
	
	UFUNCTION()
	void HandleServiceConnectionStatusChanged(EGrpcConnectionStatus NewStatus);
	
	// Helper function to get the CharacterSelectionHUD
	class ACharacterSelectionHUD* GetCharacterSelectionHUD() const;
	
private:
	// Cached list of characters
	TArray<FGrpcCharactersCharacter> Characters;
};
