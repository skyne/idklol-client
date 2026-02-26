// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/CharactersSubsystem.h"
#include "TurboLinkGrpcUtilities.h"
#include "TurboLinkGrpcManager.h"


void UCharactersSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency(UTurboLinkGrpcManager::StaticClass());
	Super::Initialize(Collection);

	InitializeConnection();
}

void UCharactersSubsystem::InitializeConnection()
{
	UTurboLinkGrpcManager* TurboLinkManager = UTurboLinkGrpcUtilities::GetTurboLinkGrpcManager(this);
	if (!TurboLinkManager)
	{
		UE_LOG(LogTemp, Error, TEXT("TurboLinkGrpcManager not available"));
		//SetChatConnectionStatus(EChatConnectionStatus::Disconnected);
		return;
	}
	
	CharacterService = Cast<UCharacterService>(TurboLinkManager->MakeService("CharacterService"));
	if (!IsValid(CharacterService))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create CharacterService"));
		//SetChatConnectionStatus(EChatConnectionStatus::Disconnected);
		return;
	}
	
	CharacterService->Connect();
	
	Client = CharacterService->MakeClient();
	if (!IsValid(Client))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create Characters client"));
		//SetChatConnectionStatus(EChatConnectionStatus::Disconnected);
		return;
	}
	
	//CharactersService->OnServiceStateChanged.AddUniqueDynamic(this, &UCharactersSubsystem::HandleGrpcStateChange);
	
	//SetChatConnectionStatus(EChatConnectionStatus::Connected);
}