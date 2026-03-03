// Fill out your copyright notice in the Description page of Project Settings.

#include "Auth/TokenManager.h"
#include "Misc/Base64.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

void UTokenManager::SetTokens(const FString& InAccessToken, const FString& InRefreshToken, int32 ExpiresInSeconds)
{
	AccessToken = InAccessToken;
	RefreshToken = InRefreshToken;

	// Calculate expiration timestamp
	int64 CurrentTimestamp = FDateTime::UtcNow().ToUnixTimestamp();
	ExpiresAtTimestamp = CurrentTimestamp + ExpiresInSeconds;

	// Try to parse JWT for more accurate expiration
	int64 JWTExpiration = 0;
	if (ParseJWTExpiration(InAccessToken, JWTExpiration))
	{
		ExpiresAtTimestamp = JWTExpiration;
	}

	UE_LOG(LogTemp, Log, TEXT("[TokenManager] Tokens set. Expires at: %lld (in %d seconds)"), 
		ExpiresAtTimestamp, ExpiresInSeconds);
}

bool UTokenManager::IsExpired() const
{
	if (ExpiresAtTimestamp == 0)
	{
		return true;
	}

	int64 CurrentTimestamp = FDateTime::UtcNow().ToUnixTimestamp();
	return CurrentTimestamp >= ExpiresAtTimestamp;
}

bool UTokenManager::WillExpireSoon(int32 ThresholdSeconds) const
{
	if (ExpiresAtTimestamp == 0)
	{
		return true;
	}

	int64 CurrentTimestamp = FDateTime::UtcNow().ToUnixTimestamp();
	int64 TimeRemaining = ExpiresAtTimestamp - CurrentTimestamp;
	
	return TimeRemaining <= ThresholdSeconds;
}

int32 UTokenManager::GetSecondsUntilExpiration() const
{
	if (ExpiresAtTimestamp == 0)
	{
		return -1;
	}

	int64 CurrentTimestamp = FDateTime::UtcNow().ToUnixTimestamp();
	return static_cast<int32>(ExpiresAtTimestamp - CurrentTimestamp);
}

bool UTokenManager::SaveTokens()
{
	if (AccessToken.IsEmpty() || RefreshToken.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TokenManager] Cannot save empty tokens"));
		return false;
	}

	// Encrypt tokens before saving
	EncryptedAccessToken = EncryptString(AccessToken);
	EncryptedRefreshToken = EncryptString(RefreshToken);
	TokenExpiresAt = ExpiresAtTimestamp;

	// Save to config
	SaveConfig();

	UE_LOG(LogTemp, Log, TEXT("[TokenManager] Tokens saved to config (encrypted)"));
	return true;
}

bool UTokenManager::LoadTokens()
{
	// Load from config
	LoadConfig();

	if (EncryptedAccessToken.IsEmpty() || EncryptedRefreshToken.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("[TokenManager] No saved tokens found in config"));
		return false;
	}

	// Decrypt tokens
	AccessToken = DecryptString(EncryptedAccessToken);
	RefreshToken = DecryptString(EncryptedRefreshToken);
	ExpiresAtTimestamp = TokenExpiresAt;

	if (AccessToken.IsEmpty() || RefreshToken.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[TokenManager] Failed to decrypt saved tokens"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[TokenManager] Tokens loaded from config. Expires at: %lld"), ExpiresAtTimestamp);
	return true;
}

void UTokenManager::ClearTokens()
{
	AccessToken.Empty();
	RefreshToken.Empty();
	ExpiresAtTimestamp = 0;
	EncryptedAccessToken.Empty();
	EncryptedRefreshToken.Empty();
	TokenExpiresAt = 0;

	SaveConfig();

	UE_LOG(LogTemp, Log, TEXT("[TokenManager] All tokens cleared"));
}

bool UTokenManager::ParseJWTExpiration(const FString& JWTToken, int64& OutExpirationTimestamp)
{
	// JWT format: header.payload.signature
	TArray<FString> Parts;
	JWTToken.ParseIntoArray(Parts, TEXT("."));

	if (Parts.Num() != 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TokenManager] Invalid JWT format (expected 3 parts, got %d)"), Parts.Num());
		return false;
	}

	// Decode the payload (second part)
	FString DecodedPayload;
	if (!Base64Decode(Parts[1], DecodedPayload))
	{
		UE_LOG(LogTemp, Warning, TEXT("[TokenManager] Failed to decode JWT payload"));
		return false;
	}

	// Parse JSON
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(DecodedPayload);
	
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TokenManager] Failed to parse JWT payload JSON"));
		return false;
	}

	// Extract 'exp' claim
	if (!JsonObject->HasField(TEXT("exp")))
	{
		UE_LOG(LogTemp, Warning, TEXT("[TokenManager] JWT payload missing 'exp' claim"));
		return false;
	}

	OutExpirationTimestamp = static_cast<int64>(JsonObject->GetNumberField(TEXT("exp")));
	
	UE_LOG(LogTemp, Log, TEXT("[TokenManager] Parsed JWT expiration: %lld"), OutExpirationTimestamp);
	return true;
}

bool UTokenManager::Base64Decode(const FString& EncodedString, FString& OutDecodedString)
{
	// JWT uses Base64 URL encoding, need to convert to standard Base64
	FString StandardBase64 = EncodedString;
	StandardBase64.ReplaceInline(TEXT("-"), TEXT("+"));
	StandardBase64.ReplaceInline(TEXT("_"), TEXT("/"));

	// Add padding if needed
	int32 PaddingNeeded = (4 - (StandardBase64.Len() % 4)) % 4;
	for (int32 i = 0; i < PaddingNeeded; i++)
	{
		StandardBase64.AppendChar('=');
	}

	// Decode
	TArray<uint8> DecodedData;
	if (!FBase64::Decode(StandardBase64, DecodedData))
	{
		return false;
	}

	// Convert to string
	OutDecodedString = FString(UTF8_TO_TCHAR(DecodedData.GetData()));
	return true;
}

FString UTokenManager::EncryptString(const FString& PlainText)
{
	if (PlainText.IsEmpty())
	{
		return FString();
	}

	// Convert string to bytes
	FTCHARToUTF8 Converter(*PlainText);
	TArray<uint8> PlainBytes;
	PlainBytes.Append(reinterpret_cast<const uint8*>(Converter.Get()), Converter.Length());

	// Get encryption key
	TArray<uint8> Key = GetEncryptionKey();

	// Simple XOR encryption (for basic obfuscation)
	// Note: For production, consider using a proper encryption library
	TArray<uint8> EncryptedBytes;
	EncryptedBytes.SetNum(PlainBytes.Num());

	for (int32 i = 0; i < PlainBytes.Num(); i++)
	{
		EncryptedBytes[i] = PlainBytes[i] ^ Key[i % Key.Num()];
	}

	// Convert to Base64 for storage
	return FBase64::Encode(EncryptedBytes);
}

FString UTokenManager::DecryptString(const FString& EncryptedText)
{
	if (EncryptedText.IsEmpty())
	{
		return FString();
	}

	// Decode from Base64
	TArray<uint8> EncryptedBytes;
	if (!FBase64::Decode(EncryptedText, EncryptedBytes))
	{
		return FString();
	}

	// Get encryption key
	TArray<uint8> Key = GetEncryptionKey();

	// XOR decrypt
	TArray<uint8> PlainBytes;
	PlainBytes.SetNum(EncryptedBytes.Num());

	for (int32 i = 0; i < EncryptedBytes.Num(); i++)
	{
		PlainBytes[i] = EncryptedBytes[i] ^ Key[i % Key.Num()];
	}

	// Convert back to string
	PlainBytes.Add(0); // Null terminator
	return FString(UTF8_TO_TCHAR(PlainBytes.GetData()));
}

TArray<uint8> UTokenManager::GetEncryptionKey()
{
	// Project-specific encryption key
	// Note: In production, this should be more secure (e.g., derived from machine/user ID)
	FString KeyString = TEXT("TPSCoreMechanics_TokenEncryption_Key_2026");
	
	TArray<uint8> Key;
	FTCHARToUTF8 Converter(*KeyString);
	Key.Append(reinterpret_cast<const uint8*>(Converter.Get()), Converter.Length());
	
	return Key;
}
