// Fill out your copyright notice in the Description page of Project Settings.

#include "CharacterCreation/SelectedCharacterSubsystem.h"

void USelectedCharacterSubsystem::SetSelectedCharacter(const FGrpcCharactersCharacter& CharacterData)
{
	SelectedCharacter = CharacterData;
	bHasSelectedCharacter = true;
	bAppearanceApplied = false; // Reset the flag when new character is selected
}

FGrpcCharactersCharacter USelectedCharacterSubsystem::GetSelectedCharacter() const
{
	return SelectedCharacter;
}

bool USelectedCharacterSubsystem::HasSelectedCharacter() const
{
	return bHasSelectedCharacter;
}

void USelectedCharacterSubsystem::ClearSelectedCharacter()
{
	SelectedCharacter = FGrpcCharactersCharacter();
	bHasSelectedCharacter = false;
	bAppearanceApplied = false;
}

void USelectedCharacterSubsystem::MarkAppearanceApplied()
{
	bAppearanceApplied = true;
}

bool USelectedCharacterSubsystem::ShouldApplyAppearance() const
{
	return bHasSelectedCharacter && !bAppearanceApplied;
}
