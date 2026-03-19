// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/CharactersSubsystem.h"

#include "TPSCoreMechanics/TPSCoreMechanics.h"

namespace
{
template <typename TResponse>
class TSafeGrpcPromise
{
public:
	TFuture<TResponse> GetFuture()
	{
		return Promise.GetFuture();
	}

	void TrySetValue(TResponse Response)
	{
		FScopeLock Lock(&Mutex);
		if (bIsCompleted)
		{
			return;
		}

		bIsCompleted = true;
		Promise.SetValue(MoveTemp(Response));
	}

	~TSafeGrpcPromise()
	{
		TrySetValue(TResponse{});
	}

private:
	TPromise<TResponse> Promise;
	FCriticalSection Mutex;
	bool bIsCompleted = false;
};
}

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
	auto Promise = MakeShared<TSafeGrpcPromise<FGrpcCharactersCharacterCreationCatalog>>();
	TFuture<FGrpcCharactersCharacterCreationCatalog> Future = Promise->GetFuture();
	if (ConnectionStatus != EGrpcConnectionStatus::Connected)
	{
		Promise->TrySetValue({});
		return Future;
	}
	UCharacterService* CharacterService = GetService();
	
	FGrpcMetaData MetaData;
	MetaData.MetaData.Add("authorization", GetValidAuthToken());
	
	CharacterService->CallGetCharacterCreationCatalog(
		{}, [Promise](const FGrpcResult& GrpcResult, const FGrpcCharactersCharacterCreationCatalog& Response)
		{
			if (GrpcResult.Code == EGrpcResultCode::Ok)
			{
				LOG_DEBUG("[CharactersSubsystem] Received character creation catalog");
				Promise->TrySetValue(Response);
			}
			else
			{
				LOG_DEBUG("[CharactersSubsystem] Failed to get character creation catalog. Code=%d, Message=%s",
					static_cast<int32>(GrpcResult.Code),
					*GrpcResult.GetMessageString()					
				);
				Promise->TrySetValue({});
			}
		}, MetaData);
	
	return Future;
}

TFuture<FGrpcCharactersCreateCharacterResponse> UCharactersSubsystem::CreateCharacterAsync(const FGrpcCharactersCreateCharacterRequest& Request)
{
	LOG("[CharactersSubsystem] Creating character: %s", *Request.Name);
	auto Promise = MakeShared<TSafeGrpcPromise<FGrpcCharactersCreateCharacterResponse>>();
	TFuture<FGrpcCharactersCreateCharacterResponse> Future = Promise->GetFuture();
	
	if (ConnectionStatus != EGrpcConnectionStatus::Connected)
	{
		LOG("[CharactersSubsystem] Cannot create character - not connected to service");
		Promise->TrySetValue({});
		return Future;
	}
	
	UCharacterService* CharacterService = GetService();
	
	FString AuthTokenValue = GetValidAuthToken();
	LOG("[CharactersSubsystem] Using auth token: %s", *AuthTokenValue);
	
	FGrpcMetaData MetaData;
	MetaData.MetaData.Add("authorization", AuthTokenValue);
	
	CharacterService->CallCreateCharacter(
		Request, [Promise](const FGrpcResult& GrpcResult, const FGrpcCharactersCreateCharacterResponse& Response)
		{
			if (GrpcResult.Code == EGrpcResultCode::Ok)
			{
				LOG("[CharactersSubsystem] Character created successfully: %s", *Response.CharacterId);
				Promise->TrySetValue(Response);
			}
			else
			{
				LOG("[CharactersSubsystem] Failed to create character. Code=%d, Message=%s",
					static_cast<int32>(GrpcResult.Code),
					*GrpcResult.GetMessageString()					
				);
				Promise->TrySetValue({});
			}
		}, MetaData);
	
	return Future;
}

TFuture<FGrpcCharactersListCreatedCharactersResponse> UCharactersSubsystem::ListCreatedCharactersAsync()
{
	LOG_DEBUG("[CharactersSubsystem] Requesting list of created characters");
	auto Promise = MakeShared<TSafeGrpcPromise<FGrpcCharactersListCreatedCharactersResponse>>();
	TFuture<FGrpcCharactersListCreatedCharactersResponse> Future = Promise->GetFuture();
	
	if (ConnectionStatus != EGrpcConnectionStatus::Connected)
	{
		LOG("[CharactersSubsystem] Cannot list characters - not connected to service");
		Promise->TrySetValue({});
		return Future;
	}
	
	UCharacterService* CharacterService = GetService();
	
	FGrpcMetaData MetaData;
	MetaData.MetaData.Add("authorization", GetValidAuthToken());
	
	CharacterService->CallListCreatedCharacters(
		{}, [Promise](const FGrpcResult& GrpcResult, const FGrpcCharactersListCreatedCharactersResponse& Response)
		{
			if (GrpcResult.Code == EGrpcResultCode::Ok)
			{
				LOG_DEBUG("[CharactersSubsystem] Received %d characters", Response.Characters.Num());
				Promise->TrySetValue(Response);
			}
			else
			{
				LOG("[CharactersSubsystem] Failed to list characters. Code=%d, Message=%s",
					static_cast<int32>(GrpcResult.Code),
					*GrpcResult.GetMessageString()					
				);
				Promise->TrySetValue({});
			}
		}, MetaData);
	
	return Future;
}
