// Copyright Epic Games, Inc. All Rights Reserved.

#include "TPSCoreMechanicsGameMode.h"
#include "TPSCoreMechanicsCharacter.h"
#include "UObject/ConstructorHelpers.h"

ATPSCoreMechanicsGameMode::ATPSCoreMechanicsGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
