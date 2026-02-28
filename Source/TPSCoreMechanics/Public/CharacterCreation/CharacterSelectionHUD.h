// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CharacterSelectionUI.h"
#include "CharacterCardWidget.h"
#include "SCharacters/CharactersMessage.h"
#include "CharacterSelectionHUD.generated.h"

/**
 * HUD for character selection screen
 */
UCLASS()
class TPSCOREMECHANICS_API ACharacterSelectionHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Selection HUD")
	TSubclassOf<UCharacterSelectionUI> CharacterSelectionUIClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Selection HUD")
	TSubclassOf<UCharacterCardWidget> CharacterCardWidgetClass;
	
	// Update the character list display
	void UpdateCharacterList(const TArray<FGrpcCharactersCharacter>& Characters);
	
	UPROPERTY()
	UCharacterSelectionUI* CharacterSelectionUI = nullptr;
	
protected:
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;
	
	// Handlers for card widget events
	UFUNCTION()
	void HandleCardSelected(FString CharacterId);
	
	UFUNCTION()
	void HandleCardDelete(FString CharacterId);
};
