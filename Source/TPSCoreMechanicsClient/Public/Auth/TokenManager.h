// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TokenManager.generated.h"

/**
 * Manages JWT access and refresh tokens with encryption
 * Handles token parsing, expiration checking, and secure storage
 */
UCLASS(Config=Game)
class TPSCOREMECHANICSCLIENT_API UTokenManager : public UObject
{
	GENERATED_BODY()

public:
	/** Initialize with tokens (typically from initial authentication) */
	void SetTokens(const FString& InAccessToken, const FString& InRefreshToken, int32 ExpiresInSeconds);

	/** Get the current access token */
	FString GetAccessToken() const { return AccessToken; }

	/** Get the current refresh token */
	FString GetRefreshToken() const { return RefreshToken; }

	/** Check if the access token is expired */
	bool IsExpired() const;

	/** Check if the access token will expire soon (within 5 minutes) */
	bool WillExpireSoon(int32 ThresholdSeconds = 300) const;

	/** Get seconds until token expires (negative if already expired) */
	int32 GetSecondsUntilExpiration() const;

	/** Save tokens to encrypted config file */
	bool SaveTokens();

	/** Load tokens from encrypted config file */
	bool LoadTokens();

	/** Clear all tokens from memory and config */
	void ClearTokens();

	/** Parse JWT token and extract expiration timestamp */
	bool ParseJWTExpiration(const FString& JWTToken, int64& OutExpirationTimestamp);

protected:
	/** Base64 decode a string */
	static bool Base64Decode(const FString& EncodedString, FString& OutDecodedString);

	/** Encrypt data using project-specific key */
	static FString EncryptString(const FString& PlainText);

	/** Decrypt data using project-specific key */
	static FString DecryptString(const FString& EncryptedText);

	/** Get the encryption key for this project */
	static TArray<uint8> GetEncryptionKey();

private:
	/** Current access token (JWT) */
	UPROPERTY()
	FString AccessToken;

	/** Current refresh token */
	UPROPERTY()
	FString RefreshToken;

	/** Unix timestamp when access token expires */
	UPROPERTY()
	int64 ExpiresAtTimestamp = 0;

	/** Encrypted access token stored in config */
	UPROPERTY(Config)
	FString EncryptedAccessToken;

	/** Encrypted refresh token stored in config */
	UPROPERTY(Config)
	FString EncryptedRefreshToken;

	/** Token expiration timestamp stored in config */
	UPROPERTY(Config)
	int64 TokenExpiresAt = 0;
};
