// Fill out your copyright notice in the Description page of Project Settings.

#include "CharacterCreation/CharacterSelectionHUD.h"

#include "Components/PanelWidget.h"
#include "TPSCoreMechanics/TPSCoreMechanics.h"

void ACharacterSelectionHUD::BeginPlay()
{
	Super::BeginPlay();
	
	LOG("BeginPlay called on CharacterSelectionHUD: %s", *GetClass()->GetName());
	
	if (CharacterSelectionUIClass)
	{
		LOG("CharacterSelectionUIClass is set to: %s", *CharacterSelectionUIClass->GetName());
		CharacterSelectionUI = CreateWidget<UCharacterSelectionUI>(GetWorld(), CharacterSelectionUIClass);
		if (CharacterSelectionUI)
		{
			CharacterSelectionUI->AddToViewport();
			LOG("Added CharacterSelectionUI to viewport");
		}
		else
		{
			LOG("Failed to create CharacterSelectionUI");
		}
	}
	else
	{
		LOG("CharacterSelectionUIClass is not set");
	}
}

void ACharacterSelectionHUD::BeginDestroy()
{
	// Clean up UI
	if (CharacterSelectionUI)
	{
		CharacterSelectionUI->RemoveFromParent();
		CharacterSelectionUI = nullptr;
	}
	
	Super::BeginDestroy();
}

void ACharacterSelectionHUD::UpdateCharacterList(const TArray<FGrpcCharactersCharacter>& Characters)
{
	if (!CharacterSelectionUI)
	{
		LOG("[CharacterSelectionHUD] Cannot update character list - UI not created");
		return;
	}
	
	LOG("[CharacterSelectionHUD] Updating character list with %d characters", Characters.Num());
	
	// Clear existing list
	CharacterSelectionUI->ClearCharacterList();
	
	// Update the UI with new character data
	CharacterSelectionUI->SetCharacterList(Characters);
	
	// Get the container for character cards
	UPanelWidget* Container = CharacterSelectionUI->GetCharacterListContainer();
	if (!Container)
	{
		LOG("[CharacterSelectionHUD] CharacterListContainer is null");
		return;
	}
	
	if (!CharacterCardWidgetClass)
	{
		LOG("[CharacterSelectionHUD] CharacterCardWidgetClass is not set");
		return;
	}
	
	// Create a card widget for each character
	for (const auto& Character : Characters)
	{
		UCharacterCardWidget* CardWidget = CreateWidget<UCharacterCardWidget>(GetWorld(), CharacterCardWidgetClass);
		if (CardWidget)
		{
			CardWidget->SetCharacterData(Character);
			Container->AddChild(CardWidget);
			
			// Bind card delegates to propagate selection/deletion events
			CardWidget->OnSelected.AddDynamic(this, &ACharacterSelectionHUD::HandleCardSelected);
			CardWidget->OnDelete.AddDynamic(this, &ACharacterSelectionHUD::HandleCardDelete);
			
			LOG_DEBUG("[CharacterSelectionHUD] Created card for character: %s (%s)",
				*Character.Name, *Character.CharacterId);
		}
		else
		{
			LOG("[CharacterSelectionHUD] Failed to create CharacterCardWidget for %s", *Character.Name);
		}
	}
	
	LOG("[CharacterSelectionHUD] Character list updated successfully");
}

void ACharacterSelectionHUD::HandleCardSelected(FString CharacterId)
{
	if (CharacterSelectionUI)
	{
		CharacterSelectionUI->TriggerOnCharacterSelected(CharacterId);
	}
}

void ACharacterSelectionHUD::HandleCardDelete(FString CharacterId)
{
	if (CharacterSelectionUI)
	{
		CharacterSelectionUI->TriggerOnDeleteCharacter(CharacterId);
	}
}
