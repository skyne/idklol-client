// Fill out your copyright notice in the Description page of Project Settings.

#include "CharacterCreation/CharacterCreationOrchestrator.h"

#include "CharacterCreation/CharacterCreation.h"
#include "CharacterCreation/CharacterSelection.h"
#include "CharacterCreation/CharacterCreationHUD.h"
#include "CharacterCreation/CharacterSelectionHUD.h"
#include "TPSCoreMechanics/TPSCoreMechanics.h"

ACharacterCreationOrchestrator::ACharacterCreationOrchestrator()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ACharacterCreationOrchestrator::BeginPlay()
{
	Super::BeginPlay();
	
	LOG("[CharacterCreationOrchestrator] Starting character creation flow");
	
	// Start with character creation
	TransitionToSelection();
}

void ACharacterCreationOrchestrator::TransitionToCreation()
{
	LOG("[CharacterCreationOrchestrator] Transitioning to Character Creation");
	
	// Cleanup any existing actor
	CleanupCurrentActor();
	
	// Explicitly clean up old HUD's UI before switching
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		if (ACharacterSelectionHUD* OldHUD = Cast<ACharacterSelectionHUD>(PC->GetHUD()))
		{
			// Force UI cleanup before HUD destruction
			if (OldHUD->CharacterSelectionUI)
			{
				OldHUD->CharacterSelectionUI->RemoveFromParent();
				OldHUD->CharacterSelectionUI = nullptr;
				LOG_DEBUG("[CharacterCreationOrchestrator] Cleaned up CharacterSelectionUI before transition");
			}
		}
	}
	
	// Set HUD to character creation
	if (PC && CharacterCreationHUDClass)
	{
		PC->ClientSetHUD(CharacterCreationHUDClass);
		LOG("[CharacterCreationOrchestrator] Set CharacterCreationHUD");
	}
	else
	{
		LOG("[CharacterCreationOrchestrator] Failed to set HUD - PC or HUDClass null");
	}
	
	// Spawn character creation actor
	if (CharacterCreationActorClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		CharacterCreationActor = GetWorld()->SpawnActor<ACharacterCreation>(CharacterCreationActorClass, SpawnParams);
		
		if (CharacterCreationActor)
		{
			// Bind to completion event
			CharacterCreationActor->OnCharacterCreationComplete.AddUniqueDynamic(this, &ACharacterCreationOrchestrator::HandleCharacterCreationComplete);
			CharacterCreationActor->OnCancelCharacterCreation.AddUniqueDynamic(this, &ACharacterCreationOrchestrator::HandleCancelCharacterCreation);
			CurrentState = ECharacterCreationState::Creation;
			LOG("[CharacterCreationOrchestrator] CharacterCreation actor spawned successfully");
		}
		else
		{
			LOG("[CharacterCreationOrchestrator] Failed to spawn CharacterCreation actor");
		}
	}
	else
	{
		LOG("[CharacterCreationOrchestrator] CharacterCreationActorClass not set");
	}
}

void ACharacterCreationOrchestrator::TransitionToSelection()
{
	LOG("[CharacterCreationOrchestrator] Transitioning to Character Selection");
	
	// Cleanup any existing actor
	CleanupCurrentActor();
	
	// Explicitly clean up old HUD's UI before switching
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		if (ACharacterCreationHUD* OldHUD = Cast<ACharacterCreationHUD>(PC->GetHUD()))
		{
			// Force UI cleanup before HUD destruction
			if (OldHUD->CharacterCreationUI)
			{
				OldHUD->CharacterCreationUI->RemoveFromParent();
				OldHUD->CharacterCreationUI = nullptr;
				LOG_DEBUG("[CharacterCreationOrchestrator] Cleaned up CharacterCreationUI before transition");
			}
		}
	}
	
	// Set HUD to character selection
	if (PC && CharacterSelectionHUDClass)
	{
		PC->ClientSetHUD(CharacterSelectionHUDClass);
		LOG("[CharacterCreationOrchestrator] Set CharacterSelectionHUD");
	}
	else
	{
		LOG("[CharacterCreationOrchestrator] Failed to set HUD - PC or HUDClass null");
	}
	
	// Spawn character selection actor
	if (CharacterSelectionActorClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		CharacterSelectionActor = GetWorld()->SpawnActor<ACharacterSelection>(CharacterSelectionActorClass, SpawnParams);
		
		if (CharacterSelectionActor)
		{
			// Bind to action events
			CharacterSelectionActor->OnRequestCreateNew.AddUniqueDynamic(this, &ACharacterCreationOrchestrator::HandleRequestCreateNew);
			CharacterSelectionActor->OnCharacterSelectedForPlay.AddUniqueDynamic(this, &ACharacterCreationOrchestrator::HandleCharacterSelectedForPlay);
			
			// Refresh character list
			CharacterSelectionActor->RefreshCharacterList();
			
			CurrentState = ECharacterCreationState::Selection;
			LOG("[CharacterCreationOrchestrator] CharacterSelection actor spawned successfully");
		}
		else
		{
			LOG("[CharacterCreationOrchestrator] Failed to spawn CharacterSelection actor");
		}
	}
	else
	{
		LOG("[CharacterCreationOrchestrator] CharacterSelectionActorClass not set");
	}
}

void ACharacterCreationOrchestrator::CleanupCurrentActor()
{
	if (CharacterCreationActor)
	{
		CharacterCreationActor->OnCharacterCreationComplete.RemoveDynamic(this, &ACharacterCreationOrchestrator::HandleCharacterCreationComplete);
		CharacterCreationActor->OnCancelCharacterCreation.RemoveDynamic(this, &ACharacterCreationOrchestrator::HandleCancelCharacterCreation);
		CharacterCreationActor->Destroy();
		CharacterCreationActor = nullptr;
		LOG_DEBUG("[CharacterCreationOrchestrator] Destroyed CharacterCreation actor");
	}
	
	if (CharacterSelectionActor)
	{
		CharacterSelectionActor->OnRequestCreateNew.RemoveDynamic(this, &ACharacterCreationOrchestrator::HandleRequestCreateNew);
		CharacterSelectionActor->OnCharacterSelectedForPlay.RemoveDynamic(this, &ACharacterCreationOrchestrator::HandleCharacterSelectedForPlay);
		CharacterSelectionActor->Destroy();
		CharacterSelectionActor = nullptr;
		LOG_DEBUG("[CharacterCreationOrchestrator] Destroyed CharacterSelection actor");
	}
	
	CurrentState = ECharacterCreationState::None;
}

void ACharacterCreationOrchestrator::HandleCharacterCreationComplete()
{
	LOG("[CharacterCreationOrchestrator] Character creation completed - transitioning to selection");
	TransitionToSelection();
}

void ACharacterCreationOrchestrator::HandleCancelCharacterCreation()
{
	LOG("[CharacterCreationOrchestrator] Character creation canceled - transitioning to selection");
	TransitionToSelection();
}

void ACharacterCreationOrchestrator::HandleRequestCreateNew()
{
	LOG("[CharacterCreationOrchestrator] Create new character requested - transitioning to creation");
	TransitionToCreation();
}

void ACharacterCreationOrchestrator::HandleCharacterSelectedForPlay(FString CharacterId)
{
	LOG("[CharacterCreationOrchestrator] Character selected for play: %s", *CharacterId);
	
	// TODO: Transition to game world with selected character
	// For now, just log the event
	LOG("[CharacterCreationOrchestrator] TODO: Enter game world with character %s", *CharacterId);
}
