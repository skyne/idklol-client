// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ServerConnectionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnServerConnectionResult, bool, bSuccess, const FString&, ErrorMessage);

/**
 * Manages client → dedicated server connection at character-selection time.
 *
 * Call ConnectToServer(CharacterId) once the player has picked their character.
 * The subsystem will:
 *   1. Retrieve a fresh access token from UKeycloakAuthService.
 *   2. Build a travel URL: <ServerAddress>?CharacterId=<uuid>&AuthToken=<jwt>
 *   3. Call ClientTravel (or OpenLevel when bUseOpenLevelFallback=true for PIE).
 *
 * Config keys (DefaultGame.ini):
 *   [/Script/TPSCoreMechanicsClient.ServerConnectionSubsystem]
 *   ServerAddress=127.0.0.1:7777
 *   bUseOpenLevelFallback=False
 *
 * NOTE: The JWT is appended directly to the URL option string. Standard Keycloak
 * RS256 tokens use base64url (no padding, no '&'/'?' chars) so they are safe in URL
 * options. Keep tokens short-lived (<15 min) to limit exposure in server logs.
 * For very large deployments, replace with a one-time nonce endpoint.
 */
UCLASS(Config=Game)
class TPSCOREMECHANICSCLIENT_API UServerConnectionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/**
	 * Initiate travel to the game server with the selected character.
	 * Automatically reads the auth token from UKeycloakAuthService.
	 * Fires OnConnectionResult after the travel is initiated (or if a precondition fails).
	 *
	 * @param CharacterId  UUID of the character to enter the game with.
	 */
	UFUNCTION(BlueprintCallable, Category = "Server Connection")
	void ConnectToServer(const FString& CharacterId);

	/**
	 * IP:Port of the dedicated server to connect to.
	 * Used when bUseOpenLevelFallback=false (the default).
	 * Example: "127.0.0.1:7777"
	 */
	UPROPERTY(Config, BlueprintReadOnly, Category = "Server Connection")
	FString ServerAddress = TEXT("127.0.0.1:7777");

	/**
	 * When true, falls back to OpenLevel using the map path from
	 * ACharacterCreationGameModeBase::GetGameWorldTransitionURL().
	 * Set to true in DefaultEditor.ini for "Run Under One Process" PIE testing.
	 * Leave false (default) for packaged builds and dedicated-server Docker deployments.
	 */
	UPROPERTY(Config, BlueprintReadOnly, Category = "Server Connection")
	bool bUseOpenLevelFallback = false;

	/** Fired when travel is initiated (bSuccess=true) or a precondition fails (bSuccess=false). */
	UPROPERTY(BlueprintAssignable, Category = "Server Connection")
	FOnServerConnectionResult OnConnectionResult;
};
