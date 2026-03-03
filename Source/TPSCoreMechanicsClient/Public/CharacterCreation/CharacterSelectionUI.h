// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SCharacters/CharactersMessage.h"
#include "CharacterSelectionUI.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterSelected, FString, CharacterId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCreateNewCharacter);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeleteCharacter, FString, CharacterId);

/**
 * Base class for Character Selection UI
 * Blueprint should implement visual representation and bind to delegates
 */
UCLASS()
class TPSCOREMECHANICSCLIENT_API UCharacterSelectionUI : public UUserWidget
{
	GENERATED_BODY()
	
public:
	
	// Blueprint must implement this to return the container widget for character cards
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category="User Interface | Interaction")
	UPanelWidget* GetCharacterListContainer() const;
	
	// Set the list of characters to display
	UFUNCTION(BlueprintCallable, Category="Character Selection")
	void SetCharacterList(const TArray<FGrpcCharactersCharacter>& Characters);
	
	// Clear all character cards from the list
	UFUNCTION(BlueprintCallable, Category="Character Selection")
	void ClearCharacterList();
	
	// Delegates for UI events
	UPROPERTY(BlueprintAssignable, Category="Character Selection")
	FOnCharacterSelected OnCharacterSelected;
	
	UPROPERTY(BlueprintAssignable, Category="Character Selection")
	FOnCreateNewCharacter OnCreateNewCharacter;
	
	UPROPERTY(BlueprintAssignable, Category="Character Selection")
	FOnDeleteCharacter OnDeleteCharacter;
	
	// Blueprint or C++ can call these to trigger events
	UFUNCTION(BlueprintCallable, Category="Character Selection")
	void TriggerOnCharacterSelected(const FString& CharacterId);
	
	UFUNCTION(BlueprintCallable, Category="Character Selection")
	void TriggerOnCreateNewCharacter();
	
	UFUNCTION(BlueprintCallable, Category="Character Selection")
	void TriggerOnDeleteCharacter(const FString& CharacterId);
	
protected:
	
	// Stored character data for reference
	UPROPERTY(BlueprintReadOnly, Category="Character Selection")
	TArray<FGrpcCharactersCharacter> Characters;
};
