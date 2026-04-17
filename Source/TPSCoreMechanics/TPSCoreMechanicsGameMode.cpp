// Copyright Epic Games, Inc. All Rights Reserved.

#include "TPSCoreMechanicsGameMode.h"
#include "Game/PlayerController/TPSCorePlayerController.h"
#include "TPSCoreMechanicsCharacter.h"

ATPSCoreMechanicsGameMode::ATPSCoreMechanicsGameMode()
{
	// Keep a code-only default for server/cook reliability. Blueprint GameModes
	// can still override this in project settings or map world settings.
	DefaultPawnClass = ATPSCoreMechanicsCharacter::StaticClass();
	PlayerControllerClass = ATPSCorePlayerController::StaticClass();
}
