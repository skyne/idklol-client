// Fill out your copyright notice in the Description page of Project Settings.

#include "CharacterCreation/CharacterCardWidget.h"

#include "CharacterCreation/CharacterCreationMapper.h"
#include "TPSCoreMechanics/TPSCoreMechanics.h"

void UCharacterCardWidget::SetCharacterData(const FGrpcCharactersCharacter& Character)
{
	CharacterId = Character.CharacterId;
	CharacterName = Character.Name;
	
	// Convert enums to readable text using mapper helpers
	RaceText = FCharacterCreationMapper::RaceEnumToString(Character.Race);
	GenderText = FCharacterCreationMapper::GenderEnumToString(Character.Gender);
	ClassText = FCharacterCreationMapper::ClassEnumToString(Character.CharacterClass);
	SkinColorText = FCharacterCreationMapper::SkinColorEnumToString(Character.SkinColor);
	CreatedAtText = Character.CreatedAt;
	
	// Set level (hardcoded for now, server doesn't support Level field yet)
	LevelText = FText::FromString(TEXT("Level 1"));
	
	// Generate avatar path based on race and gender
	AvatarPath = FString::Printf(TEXT("Avatars/%s_%s"), *RaceText, *GenderText);
	
	LOG_DEBUG("[CharacterCardWidget] Character data set: %s (%s)", *CharacterName, *CharacterId);
}

void UCharacterCardWidget::TriggerOnSelected()
{
	LOG("[CharacterCardWidget] Character selected: %s (%s)", *CharacterName, *CharacterId);
	OnSelected.Broadcast(CharacterId);
}

void UCharacterCardWidget::TriggerOnDelete()
{
	LOG("[CharacterCardWidget] Delete requested for: %s (%s)", *CharacterName, *CharacterId);
	OnDelete.Broadcast(CharacterId);
}
