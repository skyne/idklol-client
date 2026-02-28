// Fill out your copyright notice in the Description page of Project Settings.

#include "CharacterCreation/CharacterSelection.h"

#include "CharacterCreation/CharacterSelectionHUD.h"
#include "Characters/CharactersSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "TPSCoreMechanics/TPSCoreMechanics.h"

ACharacterSelection::ACharacterSelection()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ACharacterSelection::BeginPlay()
{
	Super::BeginPlay();
	
	// Bind to character service connection status
	UCharactersSubsystem* CharactersSubsystem = GetGameInstance()->GetSubsystem<UCharactersSubsystem>();
	if (CharactersSubsystem)
	{
		CharactersSubsystem->OnConnectionStatusChanged.AddUniqueDynamic(this, &ACharacterSelection::HandleServiceConnectionStatusChanged);
		
		if (CharactersSubsystem->IsConnected())
		{
			RefreshCharacterList();
		}
	}
	
	// Bind to HUD events
	if (ACharacterSelectionHUD* HUD = GetCharacterSelectionHUD())
	{
		if (HUD->CharacterSelectionUI)
		{
			HUD->CharacterSelectionUI->OnCharacterSelected.AddUniqueDynamic(this, &ACharacterSelection::HandleCharacterSelected);
			HUD->CharacterSelectionUI->OnCreateNewCharacter.AddUniqueDynamic(this, &ACharacterSelection::HandleCreateNewCharacter);
			HUD->CharacterSelectionUI->OnDeleteCharacter.AddUniqueDynamic(this, &ACharacterSelection::HandleDeleteCharacter);
		}
	}
}

void ACharacterSelection::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unbind from subsystem
	if (UCharactersSubsystem* CharactersSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCharactersSubsystem>() : nullptr)
	{
		CharactersSubsystem->OnConnectionStatusChanged.RemoveDynamic(this, &ACharacterSelection::HandleServiceConnectionStatusChanged);
	}
	
	// Unbind from HUD
	if (ACharacterSelectionHUD* HUD = GetCharacterSelectionHUD())
	{
		if (HUD->CharacterSelectionUI)
		{
			HUD->CharacterSelectionUI->OnCharacterSelected.RemoveDynamic(this, &ACharacterSelection::HandleCharacterSelected);
			HUD->CharacterSelectionUI->OnCreateNewCharacter.RemoveDynamic(this, &ACharacterSelection::HandleCreateNewCharacter);
			HUD->CharacterSelectionUI->OnDeleteCharacter.RemoveDynamic(this, &ACharacterSelection::HandleDeleteCharacter);
		}
	}
	
	Super::EndPlay(EndPlayReason);
}

void ACharacterSelection::RefreshCharacterList()
{
	LOG("[CharacterSelection] Refreshing character list");
	
	UCharactersSubsystem* CharactersSubsystem = GetGameInstance()->GetSubsystem<UCharactersSubsystem>();
	if (!CharactersSubsystem || !CharactersSubsystem->IsConnected())
	{
		LOG("[CharacterSelection] Cannot refresh - not connected to service");
		return;
	}
	
	auto Future = CharactersSubsystem->ListCreatedCharactersAsync();
	
	Future.Next([this](const FGrpcCharactersListCreatedCharactersResponse& Response)
	{
		AsyncTask(ENamedThreads::GameThread, [this, Response]()
		{
			Characters = Response.Characters;
			LOG("[CharacterSelection] Received %d characters", Characters.Num());
			
			if (ACharacterSelectionHUD* HUD = GetCharacterSelectionHUD())
			{
				HUD->UpdateCharacterList(Characters);
			}
		});
	});
}

void ACharacterSelection::HandleCharacterSelected(FString CharacterId)
{
	LOG("[CharacterSelection] Character selected: %s", *CharacterId);
	
	// Notify orchestrator that a character was selected for play
	OnCharacterSelectedForPlay.Broadcast(CharacterId);
}

void ACharacterSelection::HandleCreateNewCharacter()
{
	LOG("[CharacterSelection] Create new character requested");
	
	// Notify orchestrator to transition to character creation
	OnRequestCreateNew.Broadcast();
}

void ACharacterSelection::HandleDeleteCharacter(FString CharacterId)
{
	LOG("[CharacterSelection] Delete character requested: %s", *CharacterId);
	
	// TODO: Implement character deletion
	LOG("[CharacterSelection] TODO: Implement character deletion for %s", *CharacterId);
}

void ACharacterSelection::HandleServiceConnectionStatusChanged(EGrpcConnectionStatus NewStatus)
{
	LOG_DEBUG("[CharacterSelection] Service connection status changed: %d", static_cast<int32>(NewStatus));
	
	if (NewStatus == EGrpcConnectionStatus::Connected)
	{
		RefreshCharacterList();
	}
}

ACharacterSelectionHUD* ACharacterSelection::GetCharacterSelectionHUD() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	
	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
	{
		return nullptr;
	}
	
	return Cast<ACharacterSelectionHUD>(PC->GetHUD());
}
