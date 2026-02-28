// Fill out your copyright notice in the Description page of Project Settings.

#include "GrpcHandlerSubsystem.h"
#include "Auth/KeycloakAuthService.h"
#include "TurboLinkGrpcUtilities.h"
#include "TurboLinkGrpcManager.h"
#include "TurboLinkGrpcService.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

void UGrpcHandlerSubsystem::SetConnectionStatus(EGrpcConnectionStatus NewStatus)
{
	if (ConnectionStatus != NewStatus)
	{
		const EGrpcConnectionStatus OldStatus = ConnectionStatus;
		ConnectionStatus = NewStatus;
		
		UE_LOG(LogTemp, Log, TEXT("[%s] Connection status changed: %d -> %d"), 
			*GetLogPrefix(), 
			static_cast<int32>(OldStatus), 
			static_cast<int32>(ConnectionStatus));
		
		OnConnectionStatusChanged.Broadcast(ConnectionStatus);
	}
}

void UGrpcHandlerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency(UTurboLinkGrpcManager::StaticClass());
	Collection.InitializeDependency(UKeycloakAuthService::StaticClass());
	Super::Initialize(Collection);

	// Get KeycloakAuthService for automatic token refresh
	AuthService = GetGameInstance()->GetSubsystem<UKeycloakAuthService>();

	// First, try to use tokens from KeycloakAuthService if already saved
	if (AuthService && AuthService->HasValidTokens())
	{
		AuthToken = AuthService->GetValidAccessToken();
		UE_LOG(LogTemp, Log, TEXT("[%s] Using saved tokens from KeycloakAuthService (with auto-refresh)"), *GetLogPrefix());
		UE_LOG(LogTemp, Log, TEXT("[%s] Token expires in %d seconds"), 
			*GetLogPrefix(), AuthService->GetTokenManager()->GetSecondsUntilExpiration());
	}
	else
	{
		// Load initial token from command line or INI file
		// This uses aggressive cache avoidance to read directly from file
		FString InitialTokenData = GetAuthTokenValue();
		
		// Try to parse as JSON (Keycloak token response with access_token and refresh_token)
		bool bSetupAutoRefresh = false;
		if (AuthService && InitialTokenData.Contains(TEXT("access_token")))
		{
			// Looks like JSON - try to parse it
			UE_LOG(LogTemp, Log, TEXT("[%s] Detected JSON token response, setting up auto-refresh"), *GetLogPrefix());
			if (AuthService->SetTokensFromJson(InitialTokenData))
			{
				AuthToken = AuthService->GetValidAccessToken();
				bSetupAutoRefresh = true;
				UE_LOG(LogTemp, Log, TEXT("[%s] ===== AUTO-REFRESH ENABLED ====="), *GetLogPrefix());
				UE_LOG(LogTemp, Log, TEXT("[%s] Token will auto-refresh before expiry"), *GetLogPrefix());
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[%s] Failed to parse JSON token response"), *GetLogPrefix());
				AuthToken = InitialTokenData; // Fallback to using as-is
			}
		}
		else
		{
			// Plain JWT token - store it but can't auto-refresh without refresh token
			AuthToken = InitialTokenData;
			
			// Try to parse JWT expiration for tracking (but no auto-refresh possible)
			if (AuthService && !AuthToken.IsEmpty())
			{
				FString TokenOnly = AuthToken.Replace(TEXT("Bearer "), TEXT(""));
				
				int64 ExpiresAt = 0;
				if (AuthService->GetTokenManager()->ParseJWTExpiration(TokenOnly, ExpiresAt))
				{
					int64 CurrentTime = FDateTime::UtcNow().ToUnixTimestamp();
					int32 ExpiresIn = static_cast<int32>(ExpiresAt - CurrentTime);
					
					// Set token without refresh token
					AuthService->GetTokenManager()->SetTokens(TokenOnly, TEXT(""), ExpiresIn);
					
					UE_LOG(LogTemp, Warning, TEXT("[%s] Using access token only (no refresh token)"), *GetLogPrefix());
					UE_LOG(LogTemp, Warning, TEXT("[%s] Token expires in %d seconds - NO AUTO-REFRESH"), *GetLogPrefix(), ExpiresIn);
					UE_LOG(LogTemp, Warning, TEXT("[%s] To enable auto-refresh, provide Keycloak JSON response with refresh_token"), *GetLogPrefix());
				}
			}
		}
		
		if (!bSetupAutoRefresh)
		{
			UE_LOG(LogTemp, Log, TEXT("[%s] ===== AUTO-REFRESH NOT AVAILABLE ====="), *GetLogPrefix());
			UE_LOG(LogTemp, Log, TEXT("[%s] Provide full Keycloak JSON response to enable auto-refresh"), *GetLogPrefix());
			UE_LOG(LogTemp, Log, TEXT("[%s] Example: {\"access_token\":\"...\",\"refresh_token\":\"...\",\"expires_in\":36000}"), *GetLogPrefix());
		}
	}

	// Register console command for reloading auth token
	FString CommandName = FString::Printf(TEXT("%s.ReloadAuthToken"), *GetServiceName());
	ReloadAuthTokenCommand = IConsoleManager::Get().RegisterConsoleCommand(
		*CommandName,
		TEXT("Reload the auth token from the config file without restarting the editor"),
		FConsoleCommandDelegate::CreateUObject(this, &UGrpcHandlerSubsystem::ReloadAuthToken),
		ECVF_Default
	);

	SetConnectionStatus(EGrpcConnectionStatus::Connecting);
	InitializeConnection();
}

void UGrpcHandlerSubsystem::InitializeConnection()
{
	const FString ServiceName = GetServiceName();
	
	UTurboLinkGrpcManager* TurboLinkManager = UTurboLinkGrpcUtilities::GetTurboLinkGrpcManager(this);
	if (!TurboLinkManager)
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] TurboLinkGrpcManager not available"), *GetLogPrefix());
		SetConnectionStatus(EGrpcConnectionStatus::Disconnected);
		return;
	}
	
	Service = TurboLinkManager->MakeService(ServiceName);
	if (!IsValid(Service))
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] Failed to create service: %s"), *GetLogPrefix(), *ServiceName);
		SetConnectionStatus(EGrpcConnectionStatus::Disconnected);
		return;
	}
	
	// Connect the service
	UGrpcService* GrpcService = Cast<UGrpcService>(Service);
	if (GrpcService)
	{
		GrpcService->Connect();
		GrpcService->OnServiceStateChanged.AddUniqueDynamic(this, &UGrpcHandlerSubsystem::HandleGrpcStateChange);
	}
	
	// Create client through reflection - assumes MakeClient() method exists
	UFunction* MakeClientFunc = Service->FindFunction(TEXT("MakeClient"));
	if (!MakeClientFunc)
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] Service does not have MakeClient() method"), *GetLogPrefix());
		SetConnectionStatus(EGrpcConnectionStatus::Disconnected);
		return;
	}
	
	// Call MakeClient() and get the result
	struct FMakeClientResult
	{
		UObject* ReturnValue;
	} MakeClientResult;
	
	Service->ProcessEvent(MakeClientFunc, &MakeClientResult);
	Client = MakeClientResult.ReturnValue;
	
	if (!IsValid(Client))
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] Failed to create client for service: %s"), *GetLogPrefix(), *ServiceName);
		SetConnectionStatus(EGrpcConnectionStatus::Disconnected);
		return;
	}
	
	// Call derived class hook to bind events and start streams
	OnServiceConnected(Service, Client);
	
	SetConnectionStatus(EGrpcConnectionStatus::Connected);
	
	UE_LOG(LogTemp, Log, TEXT("[%s] Successfully initialized service: %s"), *GetLogPrefix(), *ServiceName);
}

void UGrpcHandlerSubsystem::Deinitialize()
{
	// Call derived class cleanup
	OnServiceDisconnected();
	
	// Unbind from service state changes
	if (UGrpcService* GrpcService = Cast<UGrpcService>(Service))
	{
		GrpcService->OnServiceStateChanged.RemoveDynamic(this, &UGrpcHandlerSubsystem::HandleGrpcStateChange);
	}
	
	// Unregister console command
	if (ReloadAuthTokenCommand)
	{
		IConsoleManager::Get().UnregisterConsoleObject(ReloadAuthTokenCommand);
		ReloadAuthTokenCommand = nullptr;
	}
	
	// Clear references
	Client = nullptr;
	Service = nullptr;
	SetConnectionStatus(EGrpcConnectionStatus::Disconnected);
	
	Super::Deinitialize();
}

void UGrpcHandlerSubsystem::HandleGrpcStateChange(EGrpcServiceState ServiceState)
{
	UE_LOG(LogTemp, Log, TEXT("[%s] gRPC state changed: %d"), *GetLogPrefix(), static_cast<int32>(ServiceState));
	
	switch (ServiceState)
	{
	case EGrpcServiceState::TransientFailure:
		SetConnectionStatus(EGrpcConnectionStatus::TransientError);
		UE_LOG(LogTemp, Warning, TEXT("[%s] Scheduling reconnect after transient failure (delay: %.1fs)"), *GetLogPrefix(), CurrentReconnectDelay);
		ScheduleReconnect();
		break;

	case EGrpcServiceState::Shutdown:
		SetConnectionStatus(EGrpcConnectionStatus::Shutdown);
		UE_LOG(LogTemp, Warning, TEXT("[%s] Scheduling reconnect after shutdown (delay: %.1fs)"), *GetLogPrefix(), CurrentReconnectDelay);
		ScheduleReconnect();
		break;

	case EGrpcServiceState::Ready:
		SetConnectionStatus(EGrpcConnectionStatus::Connected);
		ResetReconnectDelay();
		break;

	default:
		SetConnectionStatus(EGrpcConnectionStatus::Unknown);
		break;
	}
}

void UGrpcHandlerSubsystem::ScheduleReconnect()
{
	if (GetWorld() && !GetWorld()->GetTimerManager().IsTimerActive(ReconnectTimerHandle))
	{
		GetWorld()->GetTimerManager().SetTimer(ReconnectTimerHandle, this, &UGrpcHandlerSubsystem::PerformReconnect, CurrentReconnectDelay, false);
		// Exponential backoff for next time
		CurrentReconnectDelay = FMath::Min(CurrentReconnectDelay * 2.0f, MaxReconnectDelay);
	}
}

void UGrpcHandlerSubsystem::PerformReconnect()
{
	UE_LOG(LogTemp, Log, TEXT("[%s] Performing scheduled reconnect..."), *GetLogPrefix());
	SetConnectionStatus(EGrpcConnectionStatus::Connecting);
	InitializeConnection();
}

void UGrpcHandlerSubsystem::ResetReconnectDelay()
{
	CurrentReconnectDelay = MinReconnectDelay;
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ReconnectTimerHandle);
	}
}

FString UGrpcHandlerSubsystem::GetLogPrefix() const
{
	return GetClass()->GetName();
}

FString UGrpcHandlerSubsystem::GetAuthTokenValue() const
{
	// Check command line first
	FString CommandLineAuthToken;
	if (FParse::Value(FCommandLine::Get(), TEXT("AuthToken="), CommandLineAuthToken))
	{
		UE_LOG(LogTemp, Log, TEXT("[%s] Using command line auth token"), *GetLogPrefix());
		
		// Check if it's JSON (contains access_token) or plain JWT
		if (CommandLineAuthToken.Contains(TEXT("access_token")))
		{
			// Return raw JSON for parsing
			return CommandLineAuthToken;
		}
		else
		{
			// Plain token - add Bearer prefix
			return FString::Printf(TEXT("Bearer %s"), *CommandLineAuthToken);
		}
	}
	
	// Read DIRECTLY from the file on disk to bypass all UE config caching
	FString ConfigFilePath = FPaths::ProjectConfigDir() / TEXT("DefaultGame.ini");
	FString FileContent;
	
	if (FFileHelper::LoadFileToString(FileContent, *ConfigFilePath))
	{
		// Parse the file content line by line
		TArray<FString> Lines;
		FileContent.ParseIntoArrayLines(Lines);
		
		bool InCorrectSection = false;
		for (const FString& Line : Lines)
		{
			FString TrimmedLine = Line.TrimStartAndEnd();
			
			// Check if we're entering the correct section
			if (TrimmedLine == TEXT("[/Script/TPSCoreMechanics.GrpcHandlerSubsystem]"))
			{
				InCorrectSection = true;
				continue;
			}
			
			// Check if we've entered a different section
			if (TrimmedLine.StartsWith(TEXT("[")) && InCorrectSection)
			{
				break;
			}
			
			// Look for DefaultAuthToken in the correct section
			if (InCorrectSection && TrimmedLine.StartsWith(TEXT("DefaultAuthToken=")))
			{
				FString TokenValue;
				if (TrimmedLine.Split(TEXT("="), nullptr, &TokenValue))
				{
					UE_LOG(LogTemp, Log, TEXT("[%s] Loaded auth token from file (first 50 chars): %s..."), 
						*GetLogPrefix(), *TokenValue.Left(50));
					
					// Check if it's JSON (contains access_token) or plain JWT
					if (TokenValue.Contains(TEXT("access_token")))
					{
						// Return raw JSON for parsing
						return TokenValue;
					}
					else
					{
						// Plain token - add Bearer prefix
						return FString::Printf(TEXT("Bearer %s"), *TokenValue);
					}
				}
			}
		}
	}
	
	// Fallback to default
	FString BearerToken = FString::Printf(TEXT("Bearer %s"), *DefaultAuthToken);
	UE_LOG(LogTemp, Warning, TEXT("[%s] Using fallback default token"), *GetLogPrefix());
	return BearerToken;
}

void UGrpcHandlerSubsystem::ReloadAuthToken()
{
	// Get the fresh token value directly from file (bypasses all UE config caching)
	FString TokenData = GetAuthTokenValue();
	
	UE_LOG(LogTemp, Warning, TEXT("[%s] ===== AUTH TOKEN RELOAD INITIATED ====="), *GetLogPrefix());
	
	// Check if this is a JSON token response (Keycloak format)
	if (TokenData.Contains(TEXT("access_token")))
	{
		UE_LOG(LogTemp, Log, TEXT("[%s] Detected JSON token format, parsing..."), *GetLogPrefix());
		
		if (AuthService)
		{
			if (AuthService->SetTokensFromJson(TokenData))
			{
				// Extract just the access token for AuthToken cache
				FString AccessToken = AuthService->GetValidAccessToken();
				if (!AccessToken.IsEmpty())
				{
					AuthToken = FString::Printf(TEXT("Bearer %s"), *AccessToken);
					UE_LOG(LogTemp, Warning, TEXT("[%s] AUTO-REFRESH ENABLED (refresh token loaded)"), *GetLogPrefix());
					UE_LOG(LogTemp, Log, TEXT("[%s] Access token (first 50 chars): %s..."), 
						*GetLogPrefix(), *AccessToken.Left(50));
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[%s] Failed to parse JSON token format"), *GetLogPrefix());
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[%s] Cannot parse JSON token - KeycloakAuthService not available"), *GetLogPrefix());
		}
	}
	else
	{
		// Plain JWT token (already has "Bearer " prefix from GetAuthTokenValue)
		if (TokenData != AuthToken)
		{
			AuthToken = TokenData;
			UE_LOG(LogTemp, Warning, TEXT("[%s] Token reloaded (plain JWT format)"), *GetLogPrefix());
			UE_LOG(LogTemp, Log, TEXT("[%s] New token (first 50 chars): %s..."), 
				*GetLogPrefix(), *AuthToken.Replace(TEXT("Bearer "), TEXT("")).Left(50));
			
			// Also update KeycloakAuthService if available
			// Parse the new token to track expiration (even without refresh token)
			if (AuthService && !AuthToken.IsEmpty())
			{
				FString TokenOnly = AuthToken.Replace(TEXT("Bearer "), TEXT(""));
				
				int64 ExpiresAt = 0;
				if (AuthService->GetTokenManager()->ParseJWTExpiration(TokenOnly, ExpiresAt))
				{
					int64 CurrentTime = FDateTime::UtcNow().ToUnixTimestamp();
					int32 ExpiresIn = static_cast<int32>(ExpiresAt - CurrentTime);
					
					// Update token in manager (without refresh token)
					AuthService->GetTokenManager()->SetTokens(TokenOnly, TEXT(""), ExpiresIn);
					
					UE_LOG(LogTemp, Log, TEXT("[%s] Updated token in KeycloakAuthService. Expires in %d seconds"), 
						*GetLogPrefix(), ExpiresIn);
					UE_LOG(LogTemp, Warning, TEXT("[%s] NOTE: No refresh token - auto-refresh not available"), *GetLogPrefix());
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[%s] Auth token reloaded (no change detected)"), *GetLogPrefix());
		}
	}
}

FString UGrpcHandlerSubsystem::GetValidAuthToken()
{
	// If KeycloakAuthService is available, use it for automatic token refresh
	if (AuthService && AuthService->HasValidTokens())
	{
		FString ValidToken = AuthService->GetValidAccessToken();
		
		// Update our cached token if it changed
		if (!ValidToken.IsEmpty() && ValidToken != AuthToken)
		{
			AuthToken = ValidToken;
			UE_LOG(LogTemp, Log, TEXT("[%s] Auth token updated from KeycloakAuthService"), *GetLogPrefix());
		}
		
		return ValidToken;
	}
	
	// Fallback to cached token if auth service not available or no refresh token
	return GetCachedAuthToken();
}
