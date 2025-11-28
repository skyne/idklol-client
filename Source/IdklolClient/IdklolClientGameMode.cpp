// Copyright Epic Games, Inc. All Rights Reserved.

#include "IdklolClientGameMode.h"
#include "IdklolClientCharacter.h"
#include "UObject/ConstructorHelpers.h"

AIdklolClientGameMode::AIdklolClientGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
