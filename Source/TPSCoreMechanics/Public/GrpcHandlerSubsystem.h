// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GrpcHandlerSubsystem.generated.h"

UENUM(BlueprintType)
enum class EGrpcConnectionStatus : uint8
{
	Disconnected	UMETA(DisplayName = "Disconnected"),
	Connecting		UMETA(DisplayName = "Connecting"),
	Connected		UMETA(DisplayName = "Connected"),
	TransientError	UMETA(DisplayName = "Transient Error"),
	Shutdown		UMETA(DisplayName = "Shutdown"),
	Unknown			UMETA(DisplayName = "Unknown")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConnectionStatusChangedSignature, EGrpcConnectionStatus, NewStatus);

/**
 * Base class for gRPC service subsystems
 * Handles common connection logic, status management, and error handling
 * Use the templated TGrpcHandlerSubsystem for type-safe derived classes
 */
UCLASS(Abstract)
class TPSCOREMECHANICS_API UGrpcHandlerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
protected:
	/** Generic service object */
	UPROPERTY()
	TObjectPtr<UObject> Service;
	
	/** Generic client object */
	UPROPERTY()
	TObjectPtr<UObject> Client;
	
	/** Current connection status exposed to Blueprints */
	UPROPERTY(BlueprintReadOnly, Category = "gRPC Subsystem", meta = (AllowPrivateAccess = "true"))
	EGrpcConnectionStatus ConnectionStatus = EGrpcConnectionStatus::Disconnected;
	
	/** Update connection status and broadcast events */
	void SetConnectionStatus(EGrpcConnectionStatus NewStatus);
	
	/** Initialize the gRPC connection - calls virtual methods for derived class customization */
	void InitializeConnection();
	
	/** Handle gRPC service state changes - can be overridden for custom behavior */
	UFUNCTION()
	virtual void HandleGrpcStateChange(EGrpcServiceState ServiceState);
	
	/** 
	 * Get the service name to create from TurboLinkGrpcManager
	 * Must be implemented by derived classes (e.g., "ChatService", "CharacterService")
	 */
	virtual FString GetServiceName() const PURE_VIRTUAL(UGrpcHandlerSubsystem::GetServiceName, return TEXT(""););
	
	/**
	 * Called after successful service and client creation
	 * Derived classes should bind to client events and start streams here
	 * @param InService - The created service (cast to your specific type)
	 * @param InClient - The created client (cast to your specific type)
	 */
	virtual void OnServiceConnected(UObject* InService, UObject* InClient) PURE_VIRTUAL(UGrpcHandlerSubsystem::OnServiceConnected,);
	
	/**
	 * Called during cleanup to unbind events
	 * Derived classes should remove their event bindings here
	 */
	virtual void OnServiceDisconnected() {}
	
	/** Get a friendly name for logging (defaults to class name) */
	virtual FString GetLogPrefix() const;
	
	/**
	 * Type-safe helper to get the service cast to a specific type
	 * Use this in derived classes instead of manually casting Service
	 */
	template<typename TServiceType>
	TServiceType* GetServiceAs()
	{
		return Cast<TServiceType>(Service);
	}
	
	/**
	 * Type-safe helper to get the client cast to a specific type
	 * Use this in derived classes instead of manually casting Client
	 */
	template<typename TClientType>
	TClientType* GetClientAs()
	{
		return Cast<TClientType>(Client);
	}
	
public:
	// USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	/** Event broadcast when connection status changes */
	UPROPERTY(BlueprintAssignable, Category = "gRPC Subsystem")
	FOnConnectionStatusChangedSignature OnConnectionStatusChanged;
	
	/** Blueprint-friendly accessor for the current connection status */
	UFUNCTION(BlueprintPure, Category = "gRPC Subsystem")
	EGrpcConnectionStatus GetConnectionStatus() const { return ConnectionStatus; }
	
	/** Check if the service is connected and ready to use */
	UFUNCTION(BlueprintPure, Category = "gRPC Subsystem")
	bool IsConnected() const { return ConnectionStatus == EGrpcConnectionStatus::Connected; }
};

/**
 * Helper macro to declare type-safe service and client getters in your subsystem
 * This eliminates the need to repeatedly specify template parameters and service names
 * 
 * Usage in your derived class header (in protected section):
 *   DECLARE_GRPC_SUBSYSTEM_TYPES(ChatService)
 * 
 * This automatically generates:
 *   - GetService() -> UChatService*
 *   - GetClient() -> UChatServiceClient*
 *   - GetServiceName() override that returns "ChatService"
 * 
 * The macro constructs the full type names by prepending 'U' and appending 'Client'
 */
#define DECLARE_GRPC_SUBSYSTEM_TYPES(ServiceBaseName) \
	U##ServiceBaseName* GetService() { return GetServiceAs<U##ServiceBaseName>(); } \
	U##ServiceBaseName##Client* GetClient() { return GetClientAs<U##ServiceBaseName##Client>(); } \
	virtual FString GetServiceName() const override { return TEXT(#ServiceBaseName); }

