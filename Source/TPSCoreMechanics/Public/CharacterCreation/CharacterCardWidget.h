// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SCharacters/CharactersMessage.h"
#include "CharacterCardWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCardSelected, FString, CharacterId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCardDelete, FString, CharacterId);

/**
 * Base class for individual character card widget
 * Blueprint should implement visual representation of character data
 */
UCLASS()
class TPSCOREMECHANICS_API UCharacterCardWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	
	// Set character data for this card
	UFUNCTION(BlueprintCallable, Category="Character Card")
	void SetCharacterData(const FGrpcCharactersCharacter& Character);
	
	// Get the character ID
	UFUNCTION(BlueprintPure, Category="Character Card")
	FString GetCharacterId() const { return CharacterId; }
	
	// Delegates for card interactions
	UPROPERTY(BlueprintAssignable, Category="Character Card")
	FOnCardSelected OnSelected;
	
	UPROPERTY(BlueprintAssignable, Category="Character Card")
	FOnCardDelete OnDelete;
	
protected:
	// Character data stored in the card
	UPROPERTY(BlueprintReadOnly, Category="Character Card")
	FString CharacterId;
	
	UPROPERTY(BlueprintReadOnly, Category="Character Card")
	FString CharacterName;
	
	UPROPERTY(BlueprintReadOnly, Category="Character Card")
	FString RaceText;
	
	UPROPERTY(BlueprintReadOnly, Category="Character Card")
	FString GenderText;
	
	UPROPERTY(BlueprintReadOnly, Category="Character Card")
	FString ClassText;
	
	UPROPERTY(BlueprintReadOnly, Category="Character Card")
	FString SkinColorText;
	
	UPROPERTY(BlueprintReadOnly, Category="Character Card")
	FString CreatedAtText;
	
	UPROPERTY(BlueprintReadOnly, Category="Character Card")
	FText LevelText;
	
	UPROPERTY(BlueprintReadOnly, Category="Character Card")
	FString AvatarPath;
	
	// Blueprint or C++ can call these to trigger events
	UFUNCTION(BlueprintCallable, Category="Character Card")
	void TriggerOnSelected();
	
	UFUNCTION(BlueprintCallable, Category="Character Card")
	void TriggerOnDelete();
};
