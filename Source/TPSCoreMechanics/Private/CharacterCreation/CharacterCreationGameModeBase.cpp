// Fill out your copyright notice in the Description page of Project Settings.

#include "CharacterCreation/CharacterCreationGameModeBase.h"
#include "CharacterCreation/CharacterSelectionCharacter.h"

ACharacterCreationGameModeBase::ACharacterCreationGameModeBase()
{
	// Default pawn class should be ACharacterSelectionCharacter or a subclass
	// This is required for the character creation/selection system to work
	DefaultPawnClass = ACharacterSelectionCharacter::StaticClass();
}

void ACharacterCreationGameModeBase::BeginPlay()
{
	Super::BeginPlay();
	
	// Validate that the DefaultPawnClass is ACharacterSelectionCharacter or a subclass
	if (DefaultPawnClass && !DefaultPawnClass->IsChildOf(ACharacterSelectionCharacter::StaticClass()))
	{
		UE_LOG(LogTemp, Error, 
			TEXT("CharacterCreationGameModeBase: DefaultPawnClass must be ACharacterSelectionCharacter or a subclass! Current class: %s"),
			*DefaultPawnClass->GetName());
	}
}

FString ACharacterCreationGameModeBase::GetGameWorldTransitionURL() const
{
	if (GameWorldMap.IsNull())
	{
		return TEXT("");
	}
	
	FString MapPath = GameWorldMap.ToSoftObjectPath().GetAssetPathString();
	
	// Get just the map name without the full path
	FString MapName;
	MapPath.Split(TEXT("/"), nullptr, &MapName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	MapName.Split(TEXT("."), &MapName, nullptr);
	
	// If a game mode is specified, append it to the URL
	if (GameWorldGameMode)
	{
		FString GameModePath = GameWorldGameMode->GetPathName();
		// Append _C for blueprint classes if needed
		if (GameModePath.Contains(TEXT("/Game/")))
		{
			GameModePath += TEXT("_C");
		}
		return FString::Printf(TEXT("%s?game=%s"), *MapName, *GameModePath);
	}
	
	return MapName;
}
