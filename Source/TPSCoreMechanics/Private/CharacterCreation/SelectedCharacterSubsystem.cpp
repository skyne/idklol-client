// Fill out your copyright notice in the Description page of Project Settings.

#include "CharacterCreation/SelectedCharacterSubsystem.h"

bool USelectedCharacterSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return !IsRunningDedicatedServer();
}

void USelectedCharacterSubsystem::SetSelectedCharacter(const FCharacterData& CharacterData)
{
	SelectedCharacter = CharacterData;
	bHasSelectedCharacter = true;
	bAppearanceApplied = false; // Reset the flag when new character is selected
}

FCharacterData USelectedCharacterSubsystem::GetSelectedCharacter() const
{
	return SelectedCharacter;
}

bool USelectedCharacterSubsystem::HasSelectedCharacter() const
{
	return bHasSelectedCharacter;
}

void USelectedCharacterSubsystem::ClearSelectedCharacter()
{
	SelectedCharacter = FCharacterData();
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
