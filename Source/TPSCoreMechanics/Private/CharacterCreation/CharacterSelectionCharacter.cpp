// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterCreation/CharacterSelectionCharacter.h"

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "TPSCoreMechanics/TPSCoreMechanics.h"
#include "Helpers/CharacterAppearanceHelper.h"

void ACharacterSelectionCharacter::HandleSkinColorUpdate(FCharacterCreatorTemplate& CharacterCreator, USkeletalMeshComponent* CharacterMesh)
{
	// Convert uint8 to enum
	EGrpcCharactersSkinColor SkinColor = static_cast<EGrpcCharactersSkinColor>(CharacterCreator.SkinColor);
	UCharacterAppearanceHelper::ApplySkinColor(SkinColor, CharacterMesh);
}

void ACharacterSelectionCharacter::UpdateParameters(FCharacterCreatorTemplate& CharacterCreator)
{
	LOG_DEBUG("[CharacterSelectionCharacter] Update Parameters.");
	if (Parameters.Gender != CharacterCreator.Gender)
	{
		USkeletalMeshComponent* CharacterMesh = GetMesh();
	
		// Convert uint8 to enum and use helper
		EGrpcCharactersGender Gender = static_cast<EGrpcCharactersGender>(CharacterCreator.Gender);
		TSoftObjectPtr<USkeletalMesh> TargetMeshPtr = UCharacterAppearanceHelper::GetMeshForGender(Gender, MaleMesh, FemaleMesh);

		if (TargetMeshPtr.IsNull())
		{
			LOG_DEBUG("[CharacterSelectionCharacter] Target Mesh is Null.");
			return;
		}

		// Get the global Streamable Manager
		FStreamableManager& Streamable = UAssetManager::GetStreamableManager();

		// Create a delegate that points to our callback function
		// We pass the SoftObjectPtr as a parameter so the callback knows which one finished
		FStreamableDelegate Delegate = FStreamableDelegate::CreateUObject(this, &ACharacterSelectionCharacter::OnMeshLoaded, TargetMeshPtr);

		// Start the asynchronous load
		Streamable.RequestAsyncLoad(TargetMeshPtr.ToSoftObjectPath(), Delegate);
		
		ApplyNewParams(CharacterCreator);
		return; //everything else (color change) should happen after the new SKM is loaded and applied
	}
	else if (Parameters.SkinColor != CharacterCreator.SkinColor)
	{
		USkeletalMeshComponent* CharacterMesh = GetMesh();
		HandleSkinColorUpdate(CharacterCreator, CharacterMesh);
	}
}

void ACharacterSelectionCharacter::OnMeshLoaded(TSoftObjectPtr<USkeletalMesh> LoadedMeshPtr)
{
	// The asset is now in memory; .Get() retrieves the raw pointer
	USkeletalMesh* NewMesh = LoadedMeshPtr.Get();

	if (NewMesh && GetMesh())
	{
		GetMesh()->SetSkeletalMesh(NewMesh);
        
		// Optional: Trigger a Blueprint event or sound to signal completion
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Mesh Load Complete!"));
		HandleSkinColorUpdate(Parameters, GetMesh());
	}
	else
	{
		LOG_DEBUG("[CharacterSelectionCharacter] New mesh or current Mesh not found.");
	}
}

void ACharacterSelectionCharacter::ApplyNewParams(const FCharacterCreatorTemplate& Params)
{
	Parameters = Params;
}
