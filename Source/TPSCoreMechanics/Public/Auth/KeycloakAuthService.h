// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Http.h"
#include "Auth/TokenManager.h"
#include "KeycloakAuthService.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTokenRefreshedSignature, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAuthenticationFailedSignature);

/**
 * Service for handling OAuth2 token operations with Keycloak
 * Manages token refresh, validation, and provides valid tokens for gRPC requests
 */
UCLASS(Config=Game)
class TPSCOREMECHANICS_API UKeycloakAuthService : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Set initial tokens (typically provided externally or from config)
	 * This also saves the tokens to encrypted storage
	 */
	UFUNCTION(BlueprintCallable, Category = "Keycloak Auth")
	void SetInitialTokens(const FString& AccessToken, const FString& RefreshToken, int32 ExpiresInSeconds = 36000);

	/**
	 * Parse tokens from a JSON response (e.g., from Keycloak token endpoint)
	 * Expected format: {"access_token":"...", "refresh_token":"...", "expires_in":36000}
	 */
	UFUNCTION(BlueprintCallable, Category = "Keycloak Auth")
	bool SetTokensFromJson(const FString& JsonResponse);

	/**
	 * Get a valid access token, refreshing if necessary
	 * Returns empty string if refresh fails
	 */
	UFUNCTION(BlueprintCallable, Category = "Keycloak Auth")
	FString GetValidAccessToken();

	/**
	 * Manually trigger token refresh
	 * Useful for testing or explicit refresh requests
	 */
	UFUNCTION(BlueprintCallable, Category = "Keycloak Auth")
	void RefreshAccessToken();

	/**
	 * Check if we have valid tokens
	 */
	UFUNCTION(BlueprintPure, Category = "Keycloak Auth")
	bool HasValidTokens() const;

	/**
	 * Clear all tokens from memory and storage
	 */
	UFUNCTION(BlueprintCallable, Category = "Keycloak Auth")
	void ClearTokens();

	/**
	 * Get the token manager for direct access
	 */
	UTokenManager* GetTokenManager() const { return TokenManager; }

	/** Event broadcast when token refresh completes (success or failure) */
	UPROPERTY(BlueprintAssignable, Category = "Keycloak Auth")
	FOnTokenRefreshedSignature OnTokenRefreshed;

	/** Event broadcast when authentication fails and requires user intervention */
	UPROPERTY(BlueprintAssignable, Category = "Keycloak Auth")
	FOnAuthenticationFailedSignature OnAuthenticationFailed;

protected:
	/** Keycloak server URL */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Keycloak Auth")
	FString KeycloakUrl = TEXT("http://keycloak:8080");

	/** Keycloak realm name */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Keycloak Auth")
	FString RealmName = TEXT("idklol");

	/** OAuth2 client ID */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Keycloak Auth")
	FString ClientId = TEXT("idklol-characters");

	/** Allow insecure SSL connections (for development with self-signed certificates) */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Keycloak Auth")
	bool bAllowInsecureSSL = false;

	/** Token manager instance */
	UPROPERTY()
	TObjectPtr<UTokenManager> TokenManager;

	/** Critical section for thread-safe token refresh */
	FCriticalSection RefreshMutex;

	/** Flag to prevent multiple simultaneous refresh attempts */
	bool bIsRefreshing = false;

	/** Queue of callbacks waiting for refresh to complete */
	TArray<TFunction<void(bool)>> PendingRefreshCallbacks;

	/** Handle HTTP response for token refresh */
	void OnRefreshTokenResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	/** Build the token endpoint URL */
	FString GetTokenEndpointUrl() const;

	/** Build form-urlencoded request body for refresh token grant */
	FString BuildRefreshTokenRequestBody() const;
};
