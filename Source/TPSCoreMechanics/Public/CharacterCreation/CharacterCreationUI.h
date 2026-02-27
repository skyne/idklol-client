// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterCreationUI.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCreateCharacter);

UCLASS()
class TPSCOREMECHANICS_API UCharacterCreationUI : public UUserWidget
{
	GENERATED_BODY()
	
public:

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category="User Interface | Interaction")
	UPanelWidget* GetSelectorContainer() const;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category="User Interface | Interaction")
	UPanelWidget* GetInputBoxContainer() const;


	UPROPERTY(BlueprintAssignable, Category="Character Creation")
	FOnCreateCharacter OnCreateCharacter;

protected:
	void TriggerOnCreateCharacter()
	{
		OnCreateCharacter.Broadcast();
	}
};


