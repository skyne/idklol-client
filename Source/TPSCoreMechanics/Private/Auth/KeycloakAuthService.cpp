// Fill out your copyright notice in the Description page of Project Settings.

#include "Auth/KeycloakAuthService.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

void UKeycloakAuthService::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Create token manager
	TokenManager = NewObject<UTokenManager>(this);

	// Log config values for debugging
	UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth] Configuration loaded:"));
	UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth]   KeycloakUrl: %s"), *KeycloakUrl);
	UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth]   RealmName: %s"), *RealmName);
	UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth]   ClientId: %s"), *ClientId);
	UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth]   Token Endpoint: %s"), *GetTokenEndpointUrl());

	// Register console commands for testing
	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("KeycloakAuth.TestRefresh"),
		TEXT("Manually trigger token refresh for testing"),
		FConsoleCommandDelegate::CreateUObject(this, &UKeycloakAuthService::RefreshAccessToken),
		ECVF_Default
	);

	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("KeycloakAuth.CheckToken"),
		TEXT("Check current token status"),
		FConsoleCommandDelegate::CreateLambda([this]()
		{
			if (!TokenManager)
			{
				UE_LOG(LogTemp, Warning, TEXT("[KeycloakAuth] TokenManager not initialized"));
				return;
			}

			int32 SecondsRemaining = TokenManager->GetSecondsUntilExpiration();
			bool bExpired = TokenManager->IsExpired();
			bool bExpiringSoon = TokenManager->WillExpireSoon();

			UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth] ===== TOKEN STATUS ====="));
			UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth] Has Access Token: %s"), 
				TokenManager->GetAccessToken().IsEmpty() ? TEXT("No") : TEXT("Yes"));
			UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth] Has Refresh Token: %s"), 
				TokenManager->GetRefreshToken().IsEmpty() ? TEXT("No") : TEXT("Yes"));
			UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth] Expired: %s"), bExpired ? TEXT("YES") : TEXT("No"));
			UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth] Expiring Soon: %s"), bExpiringSoon ? TEXT("YES") : TEXT("No"));
			UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth] Seconds Until Expiry: %d"), SecondsRemaining);
		}),
		ECVF_Default
	);

	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("KeycloakAuth.SetToken"),
		TEXT("Set a new token for debugging. Usage: KeycloakAuth.SetToken <jwt_token_or_json>"),
		FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& Args)
		{
			if (Args.Num() == 0)
			{
				UE_LOG(LogTemp, Error, TEXT("[KeycloakAuth] SetToken requires a token argument"));
				UE_LOG(LogTemp, Error, TEXT("[KeycloakAuth] Usage: KeycloakAuth.SetToken <jwt_token_or_json>"));
				UE_LOG(LogTemp, Error, TEXT("[KeycloakAuth] Example JWT: KeycloakAuth.SetToken eyJhbGci..."));
				UE_LOG(LogTemp, Error, TEXT("[KeycloakAuth] Example JSON: KeycloakAuth.SetToken {\"access_token\":\"...\",\"refresh_token\":\"...\"}"));
				return;
			}

			// Join all arguments (in case JSON was split by spaces)
			FString TokenData = FString::Join(Args, TEXT(" "));

			UE_LOG(LogTemp, Warning, TEXT("[KeycloakAuth] ===== SETTING NEW TOKEN ====="));
			
			// Check if this is a JSON token response (Keycloak format)
			if (TokenData.Contains(TEXT("access_token")))
			{
				UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth] Detected JSON token format, parsing..."));
				
				if (SetTokensFromJson(TokenData))
				{
					UE_LOG(LogTemp, Warning, TEXT("[KeycloakAuth] SUCCESS: Token set with auto-refresh enabled"));
					UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth] Access token: %s..."), *TokenManager->GetAccessToken().Left(50));
					
					int32 SecondsRemaining = TokenManager->GetSecondsUntilExpiration();
					UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth] Expires in: %d seconds"), SecondsRemaining);
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("[KeycloakAuth] FAILED: Could not parse JSON token format"));
				}
			}
			else
			{
				// Plain JWT token
				UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth] Detected plain JWT format"));
				
				int64 ExpiresAt = 0;
				if (TokenManager->ParseJWTExpiration(TokenData, ExpiresAt))
				{
					int64 CurrentTime = FDateTime::UtcNow().ToUnixTimestamp();
					int32 ExpiresIn = static_cast<int32>(ExpiresAt - CurrentTime);
					
					// Set token without refresh token
					TokenManager->SetTokens(TokenData, TEXT(""), ExpiresIn);
					
					UE_LOG(LogTemp, Warning, TEXT("[KeycloakAuth] SUCCESS: Token set (no auto-refresh)"));
					UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth] Access token: %s..."), *TokenData.Left(50));
					UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth] Expires in: %d seconds"), ExpiresIn);
					UE_LOG(LogTemp, Warning, TEXT("[KeycloakAuth] NOTE: No refresh token - provide JSON format for auto-refresh"));
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("[KeycloakAuth] FAILED: Could not parse JWT token"));
					UE_LOG(LogTemp, Error, TEXT("[KeycloakAuth] Make sure the token is a valid JWT format"));
				}
			}
		}),
		ECVF_Default
	);

	// Try to load saved tokens
	if (TokenManager->LoadTokens())
	{
		UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth] Loaded saved tokens from config"));

		// Check if tokens need refresh
		if (TokenManager->IsExpired())
		{
			UE_LOG(LogTemp, Warning, TEXT("[KeycloakAuth] Saved access token is expired, will refresh on first use"));
		}
		else if (TokenManager->WillExpireSoon())
		{
			UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth] Access token will expire soon, refreshing proactively"));
			RefreshAccessToken();
		}
		else
		{
			int32 SecondsRemaining = TokenManager->GetSecondsUntilExpiration();
			UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth] Access token valid for %d more seconds"), SecondsRemaining);
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth] No saved tokens found, awaiting SetInitialTokens call"));
	}
}

void UKeycloakAuthService::Deinitialize()
{
	// Save current tokens before shutdown
	if (TokenManager && TokenManager->GetAccessToken().Len() > 0)
	{
		TokenManager->SaveTokens();
	}

	TokenManager = nullptr;
	Super::Deinitialize();
}

void UKeycloakAuthService::SetInitialTokens(const FString& AccessToken, const FString& RefreshToken, int32 ExpiresInSeconds)
{
	if (!TokenManager)
	{
		UE_LOG(LogTemp, Error, TEXT("[KeycloakAuth] TokenManager not initialized"));
		return;
	}

	TokenManager->SetTokens(AccessToken, RefreshToken, ExpiresInSeconds);
	TokenManager->SaveTokens();

	UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth] Initial tokens set and saved"));
}

bool UKeycloakAuthService::SetTokensFromJson(const FString& JsonResponse)
{
	if (!TokenManager)
	{
		UE_LOG(LogTemp, Error, TEXT("[KeycloakAuth] TokenManager not initialized"));
		return false;
	}

	// Parse JSON response
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonResponse);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[KeycloakAuth] Failed to parse JSON response"));
		return false;
	}

	// Extract required fields
	if (!JsonObject->HasField(TEXT("access_token")) || 
	    !JsonObject->HasField(TEXT("refresh_token")) ||
	    !JsonObject->HasField(TEXT("expires_in")))
	{
		UE_LOG(LogTemp, Error, TEXT("[KeycloakAuth] JSON missing required fields (access_token, refresh_token, expires_in)"));
		return false;
	}

	FString AccessToken = JsonObject->GetStringField(TEXT("access_token"));
	FString RefreshToken = JsonObject->GetStringField(TEXT("refresh_token"));
	int32 ExpiresIn = static_cast<int32>(JsonObject->GetNumberField(TEXT("expires_in")));

	SetInitialTokens(AccessToken, RefreshToken, ExpiresIn);
	
	UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth] Tokens loaded from JSON successfully"));
	return true;
}

FString UKeycloakAuthService::GetValidAccessToken()
{
	if (!TokenManager)
	{
		UE_LOG(LogTemp, Error, TEXT("[KeycloakAuth] TokenManager not initialized"));
		return FString();
	}

	// Check if token needs refresh
	if (TokenManager->IsExpired() || TokenManager->WillExpireSoon())
	{
		UE_LOG(LogTemp, Warning, TEXT("[KeycloakAuth] Access token expired or expiring soon, needs refresh"));
		
		// If already refreshing, return current token (might fail, but refresh in progress)
		if (bIsRefreshing)
		{
			UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth] Refresh already in progress, returning current token"));
			return FString::Printf(TEXT("Bearer %s"), *TokenManager->GetAccessToken());
		}

		// Trigger refresh (synchronous for this call, but HTTP request is async)
		RefreshAccessToken();
		
		// Return current token, which will be updated when refresh completes
		// Note: This means the current request might fail if token is expired
		// Consider implementing request queuing if this becomes an issue
		return FString::Printf(TEXT("Bearer %s"), *TokenManager->GetAccessToken());
	}

	// Token is valid
	return FString::Printf(TEXT("Bearer %s"), *TokenManager->GetAccessToken());
}

void UKeycloakAuthService::RefreshAccessToken()
{
	FScopeLock Lock(&RefreshMutex);

	if (bIsRefreshing)
	{
		UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth] Refresh already in progress, skipping"));
		return;
	}

	if (!TokenManager || TokenManager->GetRefreshToken().IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[KeycloakAuth] Cannot refresh: no refresh token available"));
		OnAuthenticationFailed.Broadcast();
		return;
	}

	bIsRefreshing = true;
	UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth] Starting token refresh..."));

	// Create HTTP request
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetURL(GetTokenEndpointUrl());
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/x-www-form-urlencoded"));
	HttpRequest->SetContentAsString(BuildRefreshTokenRequestBody());

	// Note: On macOS/iOS, SSL verification is handled by the OS
	if (bAllowInsecureSSL)
	{
		UE_LOG(LogTemp, Warning, TEXT("[KeycloakAuth] bAllowInsecureSSL=true"));
		UE_LOG(LogTemp, Warning, TEXT("[KeycloakAuth] On macOS: Trust the certificate in Keychain Access, or use HTTP for development"));
	}

	// Bind response handler
	HttpRequest->OnProcessRequestComplete().BindUObject(this, &UKeycloakAuthService::OnRefreshTokenResponse);

	// Execute request
	if (!HttpRequest->ProcessRequest())
	{
		UE_LOG(LogTemp, Error, TEXT("[KeycloakAuth] Failed to initiate HTTP request"));
		bIsRefreshing = false;
		OnTokenRefreshed.Broadcast(false);
	}
}

bool UKeycloakAuthService::HasValidTokens() const
{
	if (!TokenManager)
	{
		return false;
	}

	return !TokenManager->GetAccessToken().IsEmpty() && 
	       !TokenManager->GetRefreshToken().IsEmpty() &&
	       !TokenManager->IsExpired();
}

void UKeycloakAuthService::ClearTokens()
{
	if (TokenManager)
	{
		TokenManager->ClearTokens();
	}

	UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth] All tokens cleared"));
}

void UKeycloakAuthService::OnRefreshTokenResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	FScopeLock Lock(&RefreshMutex);
	bIsRefreshing = false;

	if (!bWasSuccessful || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[KeycloakAuth] Token refresh failed: HTTP request unsuccessful"));
		
		// Provide helpful SSL error guidance
		if (bAllowInsecureSSL && GetTokenEndpointUrl().StartsWith(TEXT("https://")))
		{
			UE_LOG(LogTemp, Error, TEXT("[KeycloakAuth] SSL Error - To fix self-signed certificate issues on macOS:"));
			UE_LOG(LogTemp, Error, TEXT("[KeycloakAuth]   1. Export the certificate from Keycloak"));
			UE_LOG(LogTemp, Error, TEXT("[KeycloakAuth]   2. Open Keychain Access > System > Certificates"));
			UE_LOG(LogTemp, Error, TEXT("[KeycloakAuth]   3. Drag certificate and set to 'Always Trust'"));
			UE_LOG(LogTemp, Error, TEXT("[KeycloakAuth]   OR use HTTP for local development: http://localhost:8080"));
		}
		
		OnTokenRefreshed.Broadcast(false);
		OnAuthenticationFailed.Broadcast();
		return;
	}

	int32 ResponseCode = Response->GetResponseCode();
	FString ResponseBody = Response->GetContentAsString();

	UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth] Token refresh response: %d"), ResponseCode);

	if (ResponseCode != 200)
	{
		UE_LOG(LogTemp, Error, TEXT("[KeycloakAuth] Token refresh failed: %d - %s"), ResponseCode, *ResponseBody);
		
		// If refresh token is invalid/expired, clear everything
		if (ResponseCode == 400 || ResponseCode == 401)
		{
			UE_LOG(LogTemp, Error, TEXT("[KeycloakAuth] Refresh token invalid or expired, clearing all tokens"));
			ClearTokens();
			OnAuthenticationFailed.Broadcast();
		}
		
		OnTokenRefreshed.Broadcast(false);
		return;
	}

	// Parse JSON response
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[KeycloakAuth] Failed to parse token response JSON"));
		OnTokenRefreshed.Broadcast(false);
		return;
	}

	// Extract tokens from response
	FString NewAccessToken = JsonObject->GetStringField(TEXT("access_token"));
	FString NewRefreshToken = JsonObject->GetStringField(TEXT("refresh_token"));
	int32 ExpiresIn = static_cast<int32>(JsonObject->GetNumberField(TEXT("expires_in")));

	if (NewAccessToken.IsEmpty() || NewRefreshToken.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[KeycloakAuth] Token response missing required fields"));
		OnTokenRefreshed.Broadcast(false);
		return;
	}

	// Update tokens
	TokenManager->SetTokens(NewAccessToken, NewRefreshToken, ExpiresIn);
	TokenManager->SaveTokens();

	UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth] ===== TOKEN REFRESH SUCCESSFUL ====="));
	UE_LOG(LogTemp, Log, TEXT("[KeycloakAuth] New token expires in %d seconds"), ExpiresIn);

	OnTokenRefreshed.Broadcast(true);

	// Execute any pending callbacks
	for (const auto& Callback : PendingRefreshCallbacks)
	{
		Callback(true);
	}
	PendingRefreshCallbacks.Empty();
}

FString UKeycloakAuthService::GetTokenEndpointUrl() const
{
	FString Url = KeycloakUrl.IsEmpty() ? TEXT("http://keycloak:8080") : KeycloakUrl;
	FString Realm = RealmName.IsEmpty() ? TEXT("idklol") : RealmName;
	
	return FString::Printf(TEXT("%s/realms/%s/protocol/openid-connect/token"), 
		*Url, *Realm);
}

FString UKeycloakAuthService::BuildRefreshTokenRequestBody() const
{
	FString RefreshToken = TokenManager->GetRefreshToken();
	FString Client = ClientId.IsEmpty() ? TEXT("idklol-characters") : ClientId;
	
	// URL encode the refresh token
	FString EncodedRefreshToken = FGenericPlatformHttp::UrlEncode(RefreshToken);
	FString EncodedClientId = FGenericPlatformHttp::UrlEncode(Client);

	return FString::Printf(TEXT("grant_type=refresh_token&client_id=%s&refresh_token=%s"),
		*EncodedClientId, *EncodedRefreshToken);
}
