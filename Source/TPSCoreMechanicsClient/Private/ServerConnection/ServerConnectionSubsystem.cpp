// Fill out your copyright notice in the Description page of Project Settings.

#include "ServerConnection/ServerConnectionSubsystem.h"
#include "Auth/KeycloakAuthService.h"
#include "CharacterCreation/CharacterCreationGameModeBase.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TPSCoreMechanics/TPSCoreMechanics.h"

void UServerConnectionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FString OverrideServerAddress;
	if (FParse::Value(FCommandLine::Get(), TEXT("ServerAddress="), OverrideServerAddress) && !OverrideServerAddress.IsEmpty())
	{
		ServerAddress = OverrideServerAddress;
	}

	LOG("[ServerConnectionSubsystem] Initialized (ServerAddress=%s, OpenLevelFallback=%s)",
		*ServerAddress, bUseOpenLevelFallback ? TEXT("true") : TEXT("false"));
}

void UServerConnectionSubsystem::ConnectToServer(const FString& CharacterId)
{
	if (CharacterId.IsEmpty())
	{
		LOG_ERROR("[ServerConnectionSubsystem] ConnectToServer called with empty CharacterId");
		OnConnectionResult.Broadcast(false, TEXT("No character selected"));
		return;
	}

	// Retrieve a fresh auth token — optional, but the server will reject if auth is required.
	FString AuthToken;
	if (UKeycloakAuthService* Auth = GetGameInstance()->GetSubsystem<UKeycloakAuthService>())
	{
		if (Auth->HasValidTokens())
		{
			AuthToken = Auth->GetValidAccessToken();

			// Server-side PreLogin expects the raw JWT as URL option value.
			if (AuthToken.StartsWith(TEXT("Bearer "), ESearchCase::IgnoreCase))
			{
				AuthToken.RightChopInline(7);
			}

			AuthToken.TrimStartAndEndInline();
			AuthToken = FGenericPlatformHttp::UrlEncode(AuthToken);
		}
		else
		{
			LOG_WARNING("[ServerConnectionSubsystem] No valid auth tokens — connecting without AuthToken");
		}
	}

	if (bUseOpenLevelFallback)
	{
		// PIE / single-process mode: derive the target map from the current game mode.
		ACharacterCreationGameModeBase* GM = Cast<ACharacterCreationGameModeBase>(
			GetGameInstance()->GetWorld()->GetAuthGameMode());

		if (!GM)
		{
			LOG_ERROR("[ServerConnectionSubsystem] OpenLevel fallback: current game mode is not ACharacterCreationGameModeBase");
			OnConnectionResult.Broadcast(false, TEXT("OpenLevel fallback requires ACharacterCreationGameModeBase"));
			return;
		}

		FString MapURL = GM->GetGameWorldTransitionURL();
		if (MapURL.IsEmpty())
		{
			LOG_ERROR("[ServerConnectionSubsystem] OpenLevel fallback: GameWorldMap not set on ACharacterCreationGameModeBase");
			OnConnectionResult.Broadcast(false, TEXT("GameWorldMap not configured"));
			return;
		}

		// Unreal travel URLs use '?' as option delimiter (not '&').
		MapURL = FString::Printf(TEXT("%s?CharacterId=%s"), *MapURL, *CharacterId);
		if (!AuthToken.IsEmpty())
		{
			MapURL = FString::Printf(TEXT("%s?AuthToken=%s"), *MapURL, *AuthToken);
		}

		LOG("[ServerConnectionSubsystem] OpenLevel → %s", *MapURL);
		UGameplayStatics::OpenLevel(GetGameInstance(), FName(*MapURL));
	}
	else
	{
		// Dedicated-server mode: ClientTravel to IP:Port.
		FString URL = FString::Printf(TEXT("%s?CharacterId=%s"), *ServerAddress, *CharacterId);
		if (!AuthToken.IsEmpty())
		{
			URL = FString::Printf(TEXT("%s?AuthToken=%s"), *URL, *AuthToken);
		}

		APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController();
		if (!PC)
		{
			LOG_ERROR("[ServerConnectionSubsystem] No local PlayerController — cannot initiate ClientTravel");
			OnConnectionResult.Broadcast(false, TEXT("No local player controller"));
			return;
		}

		LOG("[ServerConnectionSubsystem] ClientTravel → %s", *URL);
		PC->ClientTravel(URL, TRAVEL_Absolute);
	}

	OnConnectionResult.Broadcast(true, FString());
}
