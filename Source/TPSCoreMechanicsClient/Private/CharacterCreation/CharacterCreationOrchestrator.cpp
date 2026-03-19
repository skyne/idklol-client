// Fill out your copyright notice in the Description page of Project Settings.

#include "CharacterCreation/CharacterCreationOrchestrator.h"
#include "ServerConnection/ServerConnectionSubsystem.h"

#include "CharacterCreation/CharacterCreation.h"
#include "CharacterCreation/CharacterSelection.h"
#include "CharacterCreation/CharacterCreationHUD.h"
#include "CharacterCreation/CharacterSelectionHUD.h"
#include "CharacterCreation/SelectedCharacterSubsystem.h"
#include "Characters/CharactersSubsystem.h"
#include "Characters/CharacterGrpcMapper.h"
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

	UCharactersSubsystem* CharactersSubsystem = GetGameInstance()->GetSubsystem<UCharactersSubsystem>();
	if (!CharactersSubsystem || !CharactersSubsystem->IsConnected())
	{
		LOG("[CharacterCreationOrchestrator] Cannot get character data — not connected to character service");
		return;
	}

	// Fetch the full character list so we can cache the selected character locally in
	// USelectedCharacterSubsystem (used to pre-render appearance before server replication arrives).
	auto Future = CharactersSubsystem->ListCreatedCharactersAsync();
	Future.Next([this, CharacterId](const FGrpcCharactersListCreatedCharactersResponse& Response)
	{
		AsyncTask(ENamedThreads::GameThread, [this, CharacterId, Response]()
		{
			// Cache the character data for local pre-render (best-effort, non-blocking).
			const FGrpcCharactersCharacter* Found = Response.Characters.FindByPredicate(
				[&CharacterId](const FGrpcCharactersCharacter& C) { return C.CharacterId == CharacterId; });

			if (Found)
			{
				if (USelectedCharacterSubsystem* Sel = GetGameInstance()->GetSubsystem<USelectedCharacterSubsystem>())
				{
					Sel->SetSelectedCharacter(FCharacterGrpcMapper::ToNative(*Found));
					LOG("[CharacterCreationOrchestrator] Cached selected character: %s", *Found->Name);
				}
			}
			else
			{
				LOG_WARNING("[CharacterCreationOrchestrator] Character %s not found in list — connecting anyway", *CharacterId);
			}

			// Initiate travel to the game server (handles auth token + server address).
			if (UServerConnectionSubsystem* Conn = GetGameInstance()->GetSubsystem<UServerConnectionSubsystem>())
			{
				Conn->ConnectToServer(CharacterId);
			}
			else
			{
				LOG_ERROR("[CharacterCreationOrchestrator] UServerConnectionSubsystem not available");
			}
		});
	});
}
