// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/CharactersSubsystem.h"

void UCharactersSubsystem::OnServiceConnected(UObject* InService, UObject* InClient)
{
	UCharacterService* CharacterService = GetService();
	UCharacterServiceClient* CharacterClient = GetClient();
	
	if (!CharacterService || !CharacterClient)
	{
		UE_LOG(LogTemp, Error, TEXT("[CharactersSubsystem] Invalid service or client"));
		return;
	}
	
	// Bind to any character service events here
	// Example: CharacterClient->OnSomeEvent.AddUniqueDynamic(this, &UCharactersSubsystem::HandleSomeEvent);
	
	UE_LOG(LogTemp, Log, TEXT("[CharactersSubsystem] Character service connected"));
}

void UCharactersSubsystem::OnServiceDisconnected()
{
	// Unbind from any character service events here
	// Example:
	// if (UCharacterServiceClient* CharacterClient = GetClient())
	// {
	//     CharacterClient->OnSomeEvent.RemoveDynamic(this, &UCharactersSubsystem::HandleSomeEvent);
	// }
}