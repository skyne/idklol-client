// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterCreation/CharacterSelectionCharacter.h"

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "TPSCoreMechanics/TPSCoreMechanics.h"

void ACharacterSelectionCharacter::HandleSkinColorUpdate(FCharacterCreatorTemplate& CharacterCreator, USkeletalMeshComponent* CharacterMesh)
{
	// PALE=1,
	// FAIR=2,
	// TAN=3,
	// BROWN=4,
	// DARK=5,
	// GREEN=6,
	// GRAY=7,
	FLinearColor newColor;
	switch (CharacterCreator.SkinColor)
	{
	case 1:
		newColor = {1.0f, 0.8f, 0.6f, 1.0f};
		break;
	case 2:
		newColor = {0.9f, 0.7f, 0.5f, 1.0f};
		break;
	case 3:
		newColor = {0.8f, 0.6f, 0.4f, 1.0f};
		break;
	case 4:
		newColor = {0.6f, 0.4f, 0.2f, 1.0f};
		break;
	case 5:
		newColor = {0.4f, 0.3f, 0.1f, 1.0f};
		break;
	case 6:
		newColor = {0.3f, 0.5f, 0.3f, 1.0f};
		break;
	case 7:
		newColor = {0.5f, 0.5f, 0.5f, 1.0f};
		break;
	default:
		newColor = FLinearColor::White;
		break;
	}
    
	if (CharacterMesh)
	{
		const int MaterialsCount = CharacterMesh->GetNumMaterials();
		
		for (int MaterialIndex = 0; MaterialIndex < MaterialsCount; MaterialIndex++)
		{
			UMaterialInstanceDynamic* DynamicMat = CharacterMesh->CreateDynamicMaterialInstance(MaterialIndex);

			if (DynamicMat)
			{
				// 3. Set your parameters immediately
				DynamicMat->SetVectorParameterValue(TEXT("Tint"), newColor);
			}
		}
	}
}

void ACharacterSelectionCharacter::UpdateParameters(FCharacterCreatorTemplate& CharacterCreator)
{
	LOG_DEBUG("[CharacterSelectionCharacter] Update Parameters.");
	if (Parameters.Gender != CharacterCreator.Gender)
	{
		USkeletalMeshComponent* CharacterMesh = GetMesh();
	
		TSoftObjectPtr<USkeletalMesh> TargetMeshPtr;
		
		// MALE=1,
		// FEMALE=2,
		switch (CharacterCreator.Gender)
		{
		case 2:
			TargetMeshPtr = FemaleMesh;
			break;
		case 1:
		default:
			TargetMeshPtr = MaleMesh;
		}

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
