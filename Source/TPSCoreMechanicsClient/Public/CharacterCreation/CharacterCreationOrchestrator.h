// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CharacterCreationOrchestrator.generated.h"

class ACharacterCreation;
class ACharacterSelection;
class ACharacterCreationHUD;
class ACharacterSelectionHUD;

/**
 * Orchestrator that manages the character creation/selection flow
 * Handles actor lifecycle, HUD transitions, and state management
 */
UCLASS()
class TPSCOREMECHANICSCLIENT_API ACharacterCreationOrchestrator : public AActor
{
	GENERATED_BODY()
	
public:	
	ACharacterCreationOrchestrator();

protected:
	virtual void BeginPlay() override;

public:
	// HUD classes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Creation Orchestrator")
	TSubclassOf<ACharacterCreationHUD> CharacterCreationHUDClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Creation Orchestrator")
	TSubclassOf<ACharacterSelectionHUD> CharacterSelectionHUDClass;
	
	// Actor classes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Creation Orchestrator")
	TSubclassOf<ACharacterCreation> CharacterCreationActorClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Creation Orchestrator")
	TSubclassOf<ACharacterSelection> CharacterSelectionActorClass;

private:
	// Current active actors
	UPROPERTY()
	ACharacterCreation* CharacterCreationActor = nullptr;
	
	UPROPERTY()
	ACharacterSelection* CharacterSelectionActor = nullptr;
	
	// State management
	enum class ECharacterCreationState : uint8
	{
		None,
		Creation,
		Selection
	};
	
	ECharacterCreationState CurrentState = ECharacterCreationState::None;
	
	// Transition methods
	void TransitionToCreation();
	void TransitionToSelection();
	void CleanupCurrentActor();
	
	// Event handlers
	UFUNCTION()
	void HandleCharacterCreationComplete();
	
	UFUNCTION()
	void HandleCancelCharacterCreation();
	
	UFUNCTION()
	void HandleRequestCreateNew();
	
	UFUNCTION()
	void HandleCharacterSelectedForPlay(FString CharacterId);
};
