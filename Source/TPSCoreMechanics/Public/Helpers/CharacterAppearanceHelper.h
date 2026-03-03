// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/CharacterTypes.h"
#include "CharacterAppearanceHelper.generated.h"

/**
 * Helper class for applying character appearance parameters (mesh, colors, etc.)
 * Shared between CharacterSelectionCharacter and TPSCoreMechanicsCharacter
 */
UCLASS()
class TPSCOREMECHANICS_API UCharacterAppearanceHelper : public UObject
{
	GENERATED_BODY()
	
public:
	/**
	 * Apply skin color to a skeletal mesh
	 * @param SkinColor The skin color enum value
	 * @param CharacterMesh The skeletal mesh component to apply the color to
	 */
	static void ApplySkinColor(ECharacterSkinColor SkinColor, USkeletalMeshComponent* CharacterMesh);
	
	/**
	 * Get the linear color for a skin color enum
	 * @param SkinColor The skin color enum value
	 * @return The linear color for the skin color
	 */
	static FLinearColor GetSkinColorValue(ECharacterSkinColor SkinColor);
	
	/**
	 * Get the skeletal mesh for a given gender
	 * @param Gender The gender enum value
	 * @param MaleMesh The male skeletal mesh reference
	 * @param FemaleMesh The female skeletal mesh reference
	 * @return The appropriate skeletal mesh for the gender
	 */
	static TSoftObjectPtr<USkeletalMesh> GetMeshForGender(
		ECharacterGender Gender,
		const TSoftObjectPtr<USkeletalMesh>& MaleMesh,
		const TSoftObjectPtr<USkeletalMesh>& FemaleMesh
	);
};
