// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/GameMode/TPSCoreGameMode.h"
#include "TPSCoreMechanics/TPSCoreMechanicsCharacter.h"

ATPSCoreGameMode::ATPSCoreGameMode()
{
	// Default pawn class should be ATPSCoreMechanicsCharacter or a subclass
	// This is required for the character appearance system to work
	DefaultPawnClass = ATPSCoreMechanicsCharacter::StaticClass();
}

void ATPSCoreGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	// Validate that the DefaultPawnClass is ATPSCoreMechanicsCharacter or a subclass
	if (DefaultPawnClass && !DefaultPawnClass->IsChildOf(ATPSCoreMechanicsCharacter::StaticClass()))
	{
		UE_LOG(LogTemp, Error, 
			TEXT("TPSCoreGameMode: DefaultPawnClass must be ATPSCoreMechanicsCharacter or a subclass for character appearance system to work! Current class: %s"),
			*DefaultPawnClass->GetName());
	}
}

UCharacterClassInfo* ATPSCoreGameMode::GetCharacterClassDefaultInfo() const
{
	return ClassDefaults;
}
