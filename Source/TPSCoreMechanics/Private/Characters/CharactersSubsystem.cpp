// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/CharactersSubsystem.h"
#include "TurboLinkGrpcUtilities.h"
#include "TurboLinkGrpcManager.h"


void UCharactersSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency(UTurboLinkGrpcManager::StaticClass());
	Super::Initialize(Collection);

	SetChatConnectionStatus(EChatConnectionStatus::Connecting);
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
	
	ChatService = Cast<UChatService>(TurboLinkManager->MakeService("ChatService"));
	if (!IsValid(ChatService))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create ChatService"));
		//SetChatConnectionStatus(EChatConnectionStatus::Disconnected);
		return;
	}
	
	CharactersService->Connect();
	
	Client = CharactersService->MakeClient();
	if (!IsValid(Client))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create Characters client"));
		//SetChatConnectionStatus(EChatConnectionStatus::Disconnected);
		return;
	}

	FGrpcContextHandle Context = Client->InitStream();

	FGrpcGoogleProtobufEmpty Request = {};

	CharactersService->OnServiceStateChanged.AddUniqueDynamic(this, &UCharactersSubsystem::HandleGrpcStateChange);
	
	//SetChatConnectionStatus(EChatConnectionStatus::Connected);
}