// Fill out your copyright notice in the Description page of Project Settings.

#include "GrpcHandlerSubsystem.h"
#include "TurboLinkGrpcUtilities.h"
#include "TurboLinkGrpcManager.h"
#include "TurboLinkGrpcService.h"

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
	Super::Initialize(Collection);

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
		if (UGrpcService* GrpcService = Cast<UGrpcService>(Service))
		{
			// Try to reconnect the service when a transient error happens
			UE_LOG(LogTemp, Warning, TEXT("[%s] Attempting to reconnect after transient failure"), *GetLogPrefix());
			GrpcService->Connect();
		}
		break;
		
	case EGrpcServiceState::Shutdown:
		SetConnectionStatus(EGrpcConnectionStatus::Shutdown);
		// Attempt a clean reinitialization of the connection if the server comes back
		if (IsValid(this))
		{
			UE_LOG(LogTemp, Warning, TEXT("[%s] Service shut down, reinitializing connection"), *GetLogPrefix());
			SetConnectionStatus(EGrpcConnectionStatus::Connecting);
			InitializeConnection();
		}
		break;
		
	case EGrpcServiceState::Ready:
		SetConnectionStatus(EGrpcConnectionStatus::Connected);
		break;
		
	default:
		SetConnectionStatus(EGrpcConnectionStatus::Unknown);
		break;
	}
}

FString UGrpcHandlerSubsystem::GetLogPrefix() const
{
	return GetClass()->GetName();
}
