// Fill out your copyright notice in the Description page of Project Settings.

#include "Helpers/CharacterAppearanceHelper.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

void UCharacterAppearanceHelper::ApplySkinColor(EGrpcCharactersSkinColor SkinColor, USkeletalMeshComponent* CharacterMesh)
{
	if (!CharacterMesh)
	{
		return;
	}
	
	FLinearColor newColor = GetSkinColorValue(SkinColor);
	
	const int MaterialsCount = CharacterMesh->GetNumMaterials();
	
	for (int MaterialIndex = 0; MaterialIndex < MaterialsCount; MaterialIndex++)
	{
		UMaterialInstanceDynamic* DynamicMat = CharacterMesh->CreateDynamicMaterialInstance(MaterialIndex);
		
		if (DynamicMat)
		{
			DynamicMat->SetVectorParameterValue(TEXT("Tint"), newColor);
		}
	}
}

FLinearColor UCharacterAppearanceHelper::GetSkinColorValue(EGrpcCharactersSkinColor SkinColor)
{
	switch (SkinColor)
	{
	case EGrpcCharactersSkinColor::PALE:
		return {1.0f, 0.8f, 0.6f, 1.0f};
	case EGrpcCharactersSkinColor::FAIR:
		return {0.9f, 0.7f, 0.5f, 1.0f};
	case EGrpcCharactersSkinColor::TAN:
		return {0.8f, 0.6f, 0.4f, 1.0f};
	case EGrpcCharactersSkinColor::BROWN:
		return {0.6f, 0.4f, 0.2f, 1.0f};
	case EGrpcCharactersSkinColor::DARK:
		return {0.4f, 0.3f, 0.1f, 1.0f};
	case EGrpcCharactersSkinColor::GREEN:
		return {0.3f, 0.5f, 0.3f, 1.0f};
	case EGrpcCharactersSkinColor::GRAY:
		return {0.5f, 0.5f, 0.5f, 1.0f};
	default:
		return FLinearColor::White;
	}
}

TSoftObjectPtr<USkeletalMesh> UCharacterAppearanceHelper::GetMeshForGender(
	EGrpcCharactersGender Gender,
	const TSoftObjectPtr<USkeletalMesh>& MaleMesh,
	const TSoftObjectPtr<USkeletalMesh>& FemaleMesh)
{
	switch (Gender)
	{
	case EGrpcCharactersGender::FEMALE:
		return FemaleMesh;
	case EGrpcCharactersGender::MALE:
	default:
		return MaleMesh;
	}
}
