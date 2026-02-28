// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SCharacters/CharactersMessage.h"
#include "SelectedCharacterSubsystem.generated.h"

/**
 * Subsystem to store the selected character data when transitioning from character selection to game world
 */
UCLASS()
class TPSCOREMECHANICS_API USelectedCharacterSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	/**
	 * Set the selected character data
	 * @param CharacterData The character data to store
	 */
	UFUNCTION(BlueprintCallable, Category="Selected Character")
	void SetSelectedCharacter(const FGrpcCharactersCharacter& CharacterData);
	
	/**
	 * Get the selected character data
	 * @return The selected character data
	 */
	UFUNCTION(BlueprintCallable, Category="Selected Character")
	FGrpcCharactersCharacter GetSelectedCharacter() const;
	
	/**
	 * Check if there is a selected character
	 * @return True if there is a selected character
	 */
	UFUNCTION(BlueprintCallable, Category="Selected Character")
	bool HasSelectedCharacter() const;
	
	/**
	 * Clear the selected character data
	 */
	UFUNCTION(BlueprintCallable, Category="Selected Character")
	void ClearSelectedCharacter();
	
	/**
	 * Mark that the character appearance has been applied
	 * This prevents re-applying appearance but keeps the character data
	 */
	UFUNCTION(BlueprintCallable, Category="Selected Character")
	void MarkAppearanceApplied();
	
	/**
	 * Check if appearance needs to be applied
	 * @return True if there's a selected character and appearance hasn't been applied yet
	 */
	UFUNCTION(BlueprintCallable, Category="Selected Character")
	bool ShouldApplyAppearance() const;
	
private:
	UPROPERTY()
	FGrpcCharactersCharacter SelectedCharacter;
	
	UPROPERTY()
	bool bHasSelectedCharacter = false;
	
	UPROPERTY()
	bool bAppearanceApplied = false;
};
