// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Actor.h"
#include "CommonTypes.h"
#include "GrpcHandlerSubsystem.h"
#include "CharacterCreation.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacterCreationComplete);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCancelCharacterCreation);

USTRUCT(BlueprintType)
struct FCharacterCreatorTemplate
{
	GENERATED_BODY()
	
	uint8 Race;
	uint8 Gender;
	uint8 CharacterClass;
	uint8 SkinColor;
	FString Name;
};

UCLASS()
class TPSCOREMECHANICS_API ACharacterCreation : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACharacterCreation();
	
	// Delegate broadcast when character creation is successfully completed
	UPROPERTY(BlueprintAssignable, Category="Character Creation")
	FOnCharacterCreationComplete OnCharacterCreationComplete;
	
	// Delegate broadcast when user cancels character creation to go back to selection
	UPROPERTY(BlueprintAssignable, Category="Character Creation")
	FOnCancelCharacterCreation OnCancelCharacterCreation;
	
	UPROPERTY(BlueprintReadWrite, Category = "Character Creation")
	UUserWidget* CharacterCreationUI = nullptr;
	
	TArray<FStringByteKVP> Races;	
	TArray<FStringByteKVP> Classes;	
	TArray<FStringByteKVP> Genders;	
	TArray<FStringByteKVP> SkinColors;
	
	TArray<FRaceGenderCombination> AllowedRaceGenders;
	TArray<FRaceGenderSkinColorCombination> AllowedRaceGenderSkinColors;
	TArray<FRaceGenderClassCombination> AllowedRaceGenderClasses;

private:

	UPROPERTY()
	FCharacterCreatorTemplate CurrentCharacter = {0,0,0,0,""};

	// Event handlers for HUD selection changes
	UFUNCTION()
	void HandleRaceChanged(uint8 NewRace);
	
	UFUNCTION()
	void HandleGenderChanged(uint8 NewGender);
	
	UFUNCTION()
	void HandleClassChanged(uint8 NewClass);
	
	UFUNCTION()
	void HandleSkinColorChanged(uint8 NewSkinColor);
	
	TArray<FStringByteKVP> GetGendersForRace(uint8 RaceId);
	
	TArray<FStringByteKVP> GetClassesForRaceAndGender(uint8 RaceId, uint8 GenderId);

	TArray<FStringByteKVP> GetSkinColorsForRaceGenderAndClass(uint8 RaceId, uint8 GenderId, uint8 ClassId);
	
	// Helper function to get the CharacterCreationHUD
	class ACharacterCreationHUD* GetCharacterCreationHUD() const;
	
protected:
	UFUNCTION()
	void HandleServiceConnectionStatusChanged(EGrpcConnectionStatus NewStatus);
	
	UFUNCTION()
	void HandleNameChanged(const FString& NewText);
	
	UFUNCTION()
	void HandleCreateCharacter();
	
	UFUNCTION()
	void HandleBackToSelection();
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
