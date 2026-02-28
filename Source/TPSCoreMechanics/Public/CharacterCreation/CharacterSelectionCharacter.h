// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CharacterCreation.h"
#include "TPSCoreMechanics/TPSCoreMechanicsCharacter.h"
#include "CharacterSelectionCharacter.generated.h"

/**
 * 
 */
UCLASS()
class TPSCOREMECHANICS_API ACharacterSelectionCharacter : public ATPSCoreMechanicsCharacter
{
	GENERATED_BODY()
	
public:
	void HandleSkinColorUpdate(FCharacterCreatorTemplate& CharacterCreator, USkeletalMeshComponent* CharacterMesh);
	void UpdateParameters(FCharacterCreatorTemplate& CharacterCreator);
	
protected:
	UFUNCTION()
	void OnMeshLoaded(TSoftObjectPtr<USkeletalMesh> LoadedMeshPtr);
	
private: 
	UFUNCTION()
	void ApplyNewParams(const FCharacterCreatorTemplate& Params);
	
	FCharacterCreatorTemplate Parameters;
};
