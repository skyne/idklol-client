// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/CharactersSubsystem.h"

#include "TPSCoreMechanics/TPSCoreMechanics.h"

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

TFuture<FGrpcCharactersCharacterCreationCatalog> UCharactersSubsystem::GetCharacterCreationOptionCatalogAsync()
{
	LOG_DEBUG("[CharactersSubsystem] Requesting character creation catalog");
	auto Promise = MakeShared<TPromise<FGrpcCharactersCharacterCreationCatalog>>();
	TFuture<FGrpcCharactersCharacterCreationCatalog> Future = Promise->GetFuture();
	if (ConnectionStatus != EGrpcConnectionStatus::Connected)
	{
		Promise->SetValue({});
		return Future;
	}
	UCharacterService* CharacterService = GetService();
	
	FGrpcMetaData MetaData;
	MetaData.MetaData.Add("authorization", GetCachedAuthToken());
	
	CharacterService->CallGetCharacterCreationCatalog(
		{}, [Promise](const FGrpcResult& GrpcResult, const FGrpcCharactersCharacterCreationCatalog& Response)
		{
			if (GrpcResult.Code == EGrpcResultCode::Ok)
			{
				LOG_DEBUG("[CharactersSubsystem] Received character creation catalog");
				Promise->SetValue(Response);
			}
			else
			{
				LOG_DEBUG("[CharactersSubsystem] Failed to get character creation catalog. Code=%d, Message=%s",
					static_cast<int32>(GrpcResult.Code),
					*GrpcResult.GetMessageString()					
				);
				Promise->SetValue({});
			}
		}, MetaData);
	
	return Future;
}

TFuture<FGrpcCharactersCreateCharacterResponse> UCharactersSubsystem::CreateCharacterAsync(const FGrpcCharactersCreateCharacterRequest& Request)
{
	LOG("[CharactersSubsystem] Creating character: %s", *Request.Name);
	auto Promise = MakeShared<TPromise<FGrpcCharactersCreateCharacterResponse>>();
	TFuture<FGrpcCharactersCreateCharacterResponse> Future = Promise->GetFuture();
	
	if (ConnectionStatus != EGrpcConnectionStatus::Connected)
	{
		LOG("[CharactersSubsystem] Cannot create character - not connected to service");
		Promise->SetValue({});
		return Future;
	}
	
	UCharacterService* CharacterService = GetService();
	
	FString AuthTokenValue = GetCachedAuthToken();
	LOG("[CharactersSubsystem] Using auth token: %s", *AuthTokenValue);
	
	FGrpcMetaData MetaData;
	MetaData.MetaData.Add("authorization", AuthTokenValue);
	
	CharacterService->CallCreateCharacter(
		Request, [Promise](const FGrpcResult& GrpcResult, const FGrpcCharactersCreateCharacterResponse& Response)
		{
			if (GrpcResult.Code == EGrpcResultCode::Ok)
			{
				LOG("[CharactersSubsystem] Character created successfully: %s", *Response.CharacterId);
				Promise->SetValue(Response);
			}
			else
			{
				LOG("[CharactersSubsystem] Failed to create character. Code=%d, Message=%s",
					static_cast<int32>(GrpcResult.Code),
					*GrpcResult.GetMessageString()					
				);
				Promise->SetValue({});
			}
		}, MetaData);
	
	return Future;
}

TFuture<FGrpcCharactersListCreatedCharactersResponse> UCharactersSubsystem::ListCreatedCharactersAsync()
{
	LOG_DEBUG("[CharactersSubsystem] Requesting list of created characters");
	auto Promise = MakeShared<TPromise<FGrpcCharactersListCreatedCharactersResponse>>();
	TFuture<FGrpcCharactersListCreatedCharactersResponse> Future = Promise->GetFuture();
	
	if (ConnectionStatus != EGrpcConnectionStatus::Connected)
	{
		LOG("[CharactersSubsystem] Cannot list characters - not connected to service");
		Promise->SetValue({});
		return Future;
	}
	
	UCharacterService* CharacterService = GetService();
	
	FGrpcMetaData MetaData;
	MetaData.MetaData.Add("authorization", GetCachedAuthToken());
	
	CharacterService->CallListCreatedCharacters(
		{}, [Promise](const FGrpcResult& GrpcResult, const FGrpcCharactersListCreatedCharactersResponse& Response)
		{
			if (GrpcResult.Code == EGrpcResultCode::Ok)
			{
				LOG_DEBUG("[CharactersSubsystem] Received %d characters", Response.Characters.Num());
				Promise->SetValue(Response);
			}
			else
			{
				LOG("[CharactersSubsystem] Failed to list characters. Code=%d, Message=%s",
					static_cast<int32>(GrpcResult.Code),
					*GrpcResult.GetMessageString()					
				);
				Promise->SetValue({});
			}
		}, MetaData);
	
	return Future;
}
