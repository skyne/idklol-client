// Fill out your copyright notice in the Description page of Project Settings.

#include "CharacterCreation/CharacterSelectionUI.h"

#include "Components/PanelWidget.h"
#include "TPSCoreMechanics/TPSCoreMechanics.h"

void UCharacterSelectionUI::SetCharacterList(const TArray<FGrpcCharactersCharacter>& InCharacters)
{
	Characters = InCharacters;
	LOG("[CharacterSelectionUI] Character list set with %d characters", Characters.Num());
	
	// Blueprint should implement visual representation
	// This is a placeholder for C++ to store the data
}

void UCharacterSelectionUI::ClearCharacterList()
{
	Characters.Empty();
	
	UPanelWidget* Container = GetCharacterListContainer();
	if (Container)
	{
		Container->ClearChildren();
		LOG_DEBUG("[CharacterSelectionUI] Character list cleared");
	}
}

void UCharacterSelectionUI::TriggerOnCharacterSelected(const FString& CharacterId)
{
	LOG("[CharacterSelectionUI] Character selected: %s", *CharacterId);
	OnCharacterSelected.Broadcast(CharacterId);
}

void UCharacterSelectionUI::TriggerOnCreateNewCharacter()
{
	LOG("[CharacterSelectionUI] Create new character triggered");
	OnCreateNewCharacter.Broadcast();
}

void UCharacterSelectionUI::TriggerOnDeleteCharacter(const FString& CharacterId)
{
	LOG("[CharacterSelectionUI] Delete character triggered: %s", *CharacterId);
	OnDeleteCharacter.Broadcast(CharacterId);
}
