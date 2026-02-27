// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterCreation/CharacterCreation.h"

#include "GameFramework/GameMode.h"
#include "GameFramework/HUD.h"
#include "Kismet/GameplayStatics.h"
#include "CharacterCreation/CharacterCreationHUD.h"
#include "CharacterCreation/CharacterCreationMapper.h"
#include "CharacterCreation/CharacterSelectionCharacter.h"
#include "Characters/CharactersSubsystem.h"
#include "TPSCoreMechanics/TPSCoreMechanics.h"

// Sets default values
ACharacterCreation::ACharacterCreation()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void ACharacterCreation::BeginPlay()
{
	Super::BeginPlay();
	
	UCharactersSubsystem* CharactersSubsystem = GetGameInstance()->GetSubsystem<UCharactersSubsystem>();
	
	CharactersSubsystem->OnConnectionStatusChanged.AddUniqueDynamic(this, &ACharacterCreation::HandleServiceConnectionStatusChanged);
	
	if(CharactersSubsystem->IsConnected()) // in case the subsystem was already connected before we added our event handler, trigger it manually
	{
		HandleServiceConnectionStatusChanged(EGrpcConnectionStatus::Connected);
	}

	if (ACharacterCreationHUD* HUD = GetCharacterCreationHUD()) {
		HUD->OnNameChanged.AddUniqueDynamic(this, &ACharacterCreation::HandleNameChanged);
	}
}

void ACharacterCreation::HandleServiceConnectionStatusChanged(EGrpcConnectionStatus NewStatus)
{
	LOG_DEBUG("[CharacterCreation] Character service connection status changed: %d", static_cast<int32>(NewStatus));
	if (NewStatus == EGrpcConnectionStatus::Connected && Races.Num() == 0)
	{
		UCharactersSubsystem* CharactersSubsystem = GetGameInstance()->GetSubsystem<UCharactersSubsystem>();
		auto Future = CharactersSubsystem->GetCharacterCreationOptionCatalogAsync();
	
		Future.Next([this](FGrpcCharactersCharacterCreationCatalog Result)
		{
			LOG_DEBUG("[CharacterCreation] Received character creation catalog from service");
			// Map Races
			auto Races = FCharacterCreationMapper::MapToStringByteKVP(
				Result.Races,
				[](const auto& Race) { return Race.Name; },
				[](const auto& Race) { return static_cast<uint8>(Race.Race); }
			);
			this->Races = Races;
			
			// Map Genders
			auto Genders = FCharacterCreationMapper::MapToStringByteKVP(
				Result.Genders,
				[](const auto& Gender) { return Gender.Name; },
				[](const auto& Gender) { return static_cast<uint8>(Gender.Gender); }
			);
			this->Genders = Genders;
			
			// Map Skin Colors
			auto SkinColors = FCharacterCreationMapper::MapToStringByteKVP(
				Result.SkinColors,
				[](const auto& SkinColor) { return SkinColor.Name; },
				[](const auto& SkinColor) { return static_cast<uint8>(SkinColor.SkinColor); }
			);
			this->SkinColors = SkinColors;
			
			// Map Classes
			auto Classes = FCharacterCreationMapper::MapToStringByteKVP(
				Result.Classes,
				[](const auto& Class) { return Class.Name; },
				[](const auto& Class) { return static_cast<uint8>(Class.CharacterClass); }
			);
			this->Classes = Classes;
			
			// Map restriction combinations
			auto AllowedRaceGenders = FCharacterCreationMapper::MapToRaceGenderCombinations(
				Result.AllowedRaceGender,
				[](const auto& Item) { return Item.Race; },
				[](const auto& Item) { return Item.Gender; }
			);
			this->AllowedRaceGenders = AllowedRaceGenders;
			
			auto AllowedRaceGenderSkinColors = FCharacterCreationMapper::MapToRaceGenderSkinColorCombinations(
				Result.AllowedRaceGenderSkinColor,
				[](const auto& Item) { return Item.Race; },
				[](const auto& Item) { return Item.Gender; },
				[](const auto& Item) { return Item.SkinColor; }
			);
			this->AllowedRaceGenderSkinColors = AllowedRaceGenderSkinColors;
			
			auto AllowedRaceGenderClasses = FCharacterCreationMapper::MapToRaceGenderClassCombinations(
				Result.AllowedRaceGenderClass,
				[](const auto& Item) { return Item.Race; },
				[](const auto& Item) { return Item.Gender; },
				[](const auto& Item) { return Item.CharacterClass; }
			);
			this->AllowedRaceGenderClasses = AllowedRaceGenderClasses;
			
			LOG("[CharacterCreation] Catalog mapped - Races: %d, Genders: %d, Classes: %d, SkinColors: %d", 
				this->Races.Num(), this->Genders.Num(), this->Classes.Num(), this->SkinColors.Num());

			AsyncTask(ENamedThreads::GameThread, [this](){
				if (ACharacterCreationHUD* HUD = GetCharacterCreationHUD())
				{
					LOG("[CharacterCreation] HUD found, binding events and updating UI");
					
					HUD->OnSelectedRaceChanged.AddUniqueDynamic(this, &ACharacterCreation::HandleRaceChanged);
					HUD->OnSelectedGenderChanged.AddUniqueDynamic(this, &ACharacterCreation::HandleGenderChanged);
					HUD->OnSelectedClassChanged.AddUniqueDynamic(this, &ACharacterCreation::HandleClassChanged);
					HUD->OnSelectedSkinColorChanged.AddUniqueDynamic(this, &ACharacterCreation::HandleSkinColorChanged);
					
				// Create widgets in dependency order: Race (affects 3) → Gender (affects 2) → Class (affects 1) → SkinColor (affects 0)
				// This ensures proper render order and cascading validation
					LOG("[CharacterCreation] Updating HUD - Races: %d, Genders: %d, Classes: %d, SkinColors: %d",
						this->Races.Num(), this->Genders.Num(), this->Classes.Num(), this->SkinColors.Num());

					HUD->UpdateAvailableRaces(this->Races);
					HUD->UpdateAvailableGenders(this->Genders);
					HUD->UpdateAvailableClasses(this->Classes);
					HUD->UpdateAvailableSkinColors(this->SkinColors);
				}
				else
				{
					LOG("[CharacterCreation] HUD is null or not CharacterCreationHUD type");
				}
			});
		});
	}
}

void ACharacterCreation::HandleNameChanged(const FString& NewText)
{
	CurrentCharacter.Name = NewText;
}

void ACharacterCreation::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unbind from subsystem
	if (UCharactersSubsystem* CharactersSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCharactersSubsystem>() : nullptr)
	{
		CharactersSubsystem->OnConnectionStatusChanged.RemoveDynamic(this, &ACharacterCreation::HandleServiceConnectionStatusChanged);
	}
	
	// Unbind HUD event handlers
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PC = World->GetFirstPlayerController();
		if (PC)
		{
			if (ACharacterCreationHUD* HUD = Cast<ACharacterCreationHUD>(PC->GetHUD()))
			{
				HUD->OnSelectedRaceChanged.RemoveDynamic(this, &ACharacterCreation::HandleRaceChanged);
				HUD->OnSelectedGenderChanged.RemoveDynamic(this, &ACharacterCreation::HandleGenderChanged);
				HUD->OnSelectedClassChanged.RemoveDynamic(this, &ACharacterCreation::HandleClassChanged);
				HUD->OnSelectedSkinColorChanged.RemoveDynamic(this, &ACharacterCreation::HandleSkinColorChanged);
			}
		}
	}
	
	Super::EndPlay(EndPlayReason);
}

void ACharacterCreation::HandleRaceChanged(uint8 NewRace)
{
	LOG("[CharacterCreation] Race changed to: %d", NewRace);

	CurrentCharacter.Race = NewRace;

	// Update dependent options in dependency order: Gender -> Class -> SkinColor
	const auto allowedGenders = GetGendersForRace(CurrentCharacter.Race);
	
	if (ACharacterCreationHUD* HUD = GetCharacterCreationHUD())
	{
		HUD->UpdateAvailableGenders(allowedGenders);
		// Gender update will trigger HandleGenderChanged which updates Class and SkinColor
	}
	
	ACharacterSelectionCharacter* character = Cast<ACharacterSelectionCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (character)
	{
		character->UpdateParameters(CurrentCharacter);
	}
}

void ACharacterCreation::HandleGenderChanged(uint8 NewGender)
{
	LOG("[CharacterCreation] Gender changed to: %d", NewGender);
	CurrentCharacter.Gender = NewGender;

	// Update dependent options in dependency order: Class -> SkinColor
	const auto allowedClasses = GetClassesForRaceAndGender(CurrentCharacter.Race, CurrentCharacter.Gender);

	if (ACharacterCreationHUD* HUD = GetCharacterCreationHUD())
	{
		HUD->UpdateAvailableClasses(allowedClasses);
		// Class update will trigger HandleClassChanged which updates SkinColor
	}
	
	ACharacterSelectionCharacter* character = Cast<ACharacterSelectionCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (character)
	{
		character->UpdateParameters(CurrentCharacter);
	}
}

void ACharacterCreation::HandleClassChanged(uint8 NewClass)
{
	LOG("[CharacterCreation] Class changed to: %d", NewClass);
	CurrentCharacter.CharacterClass = NewClass;
	
	// Update dependent options: SkinColor
	const auto allowedSkinColors = GetSkinColorsForRaceGenderAndClass(CurrentCharacter.Race, CurrentCharacter.Gender, CurrentCharacter.CharacterClass);
	
	if (ACharacterCreationHUD* HUD = GetCharacterCreationHUD())
	{
		HUD->UpdateAvailableSkinColors(allowedSkinColors);
	}
	
	ACharacterSelectionCharacter* character = Cast<ACharacterSelectionCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (character)
	{
		character->UpdateParameters(CurrentCharacter);
	}
}

void ACharacterCreation::HandleSkinColorChanged(uint8 NewSkinColor)
{
	LOG("[CharacterCreation] Skin color changed to: %d", NewSkinColor);
	CurrentCharacter.SkinColor = NewSkinColor;

	// SkinColor has no dependent options
	
	ACharacterSelectionCharacter* character = Cast<ACharacterSelectionCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (character)
	{
		character->UpdateParameters(CurrentCharacter);
	}
}

TArray<FStringByteKVP> ACharacterCreation::GetGendersForRace(uint8 RaceId)
{
	// Get allowed gender IDs for this race
	TArray<uint8> AllowedGenderIds;
	for (const auto& Combo : AllowedRaceGenders)
	{
		if (Combo.Race == RaceId)
		{
			AllowedGenderIds.AddUnique(Combo.Gender);
		}
	}
	
	// Filter Genders array to only include allowed genders
	return Genders.FilterByPredicate([&AllowedGenderIds](const FStringByteKVP& GenderKVP)
	{
		return AllowedGenderIds.Contains(GenderKVP.Value);
	});
}

TArray<FStringByteKVP> ACharacterCreation::GetClassesForRaceAndGender(uint8 RaceId, uint8 GenderId)
{
	// Get allowed class IDs for this race and gender
	TArray<uint8> AllowedClassIds;
	for (const auto& Combo : AllowedRaceGenderClasses)
	{
		if (Combo.Race == RaceId && Combo.Gender == GenderId)
		{
			AllowedClassIds.AddUnique(Combo.CharacterClass);
		}
	}
	
	// Filter Classes array to only include allowed classes
	return Classes.FilterByPredicate([&AllowedClassIds](const FStringByteKVP& ClassKVP)
	{
		return AllowedClassIds.Contains(ClassKVP.Value);
	});
}

TArray<FStringByteKVP> ACharacterCreation::GetSkinColorsForRaceGenderAndClass(uint8 RaceId, uint8 GenderId,
	uint8 ClassId)
{
	// Get allowed skin color IDs for this race, gender - note: class is not part of skin color restrictions
	TArray<uint8> AllowedSkinColorIds;
	for (const auto& Combo : AllowedRaceGenderSkinColors)
	{
		if (Combo.Race == RaceId && Combo.Gender == GenderId)
		{
			AllowedSkinColorIds.AddUnique(Combo.SkinColor);
		}
	}
	
	// Filter SkinColors array to only include allowed skin colors
	return SkinColors.FilterByPredicate([&AllowedSkinColorIds](const FStringByteKVP& SkinColorKVP)
	{
		return AllowedSkinColorIds.Contains(SkinColorKVP.Value);
	});
}

ACharacterCreationHUD* ACharacterCreation::GetCharacterCreationHUD() const
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
	
	return Cast<ACharacterCreationHUD>(PC->GetHUD());
}

// Called every frame
void ACharacterCreation::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

