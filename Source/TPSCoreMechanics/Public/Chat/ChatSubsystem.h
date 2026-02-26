// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "SChat/ChatService.h"
#include "ChatSubsystem.generated.h"

UENUM(BlueprintType)
enum class EChatConnectionStatus : uint8
{
	Disconnected	UMETA(DisplayName = "Disconnected"),
	Connecting		UMETA(DisplayName = "Connecting"),
	Connected		UMETA(DisplayName = "Connected"),
	TransientError	UMETA(DisplayName = "Transient Error"),
	Shutdown		UMETA(DisplayName = "Shutdown"),
	Unknown			UMETA(DisplayName = "Unknown")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnNewMessageReceivedSignature, FString, Timestamp, FString, Sender, FString, Message);

/**
 * 
 */
UCLASS()
class TPSCOREMECHANICS_API UChatSubsystem : public UGameInstanceSubsystem

{
	GENERATED_BODY()
	
private:
	UPROPERTY()
	UChatServiceClient* Client;
	UPROPERTY()
	UChatService* ChatService;
	
	/** Current connection status exposed to Blueprints */
	UPROPERTY(BlueprintReadOnly, Category = "Chat Subsystem", meta = (AllowPrivateAccess = "true"))
	EChatConnectionStatus ChatConnectionStatus = EChatConnectionStatus::Disconnected;
	
	/** Internal helper to update status consistently */
	void SetChatConnectionStatus(EChatConnectionStatus NewStatus);
	
	void InitializeConnection();
	
	UFUNCTION()
	void HandleStreamResponse(FGrpcContextHandle Handle, const FGrpcResult& GrpcResult, const FGrpcChatChatMessage& Response);
	
	UFUNCTION()
	void HandleGrpcStateChange(EGrpcServiceState ServiceState);
	
public:
	void Initialize(FSubsystemCollectionBase& Collection) override;
	void Deinitialize() override;
	
	UFUNCTION(BlueprintCallable)
	void NewChatMessage(FString Message);
	
	// Call this from C++ to trigger the Blueprint event
	UPROPERTY(BlueprintAssignable, Category = "Chat Subsystem")
	FOnNewMessageReceivedSignature OnNewMessageReceived;
	
	UFUNCTION(BlueprintCallable, Category = "Chat Subsystem")
	void TriggerNewMessageReceived(const FString& Timestamp,const FString& Sender,const FString& Message);
	
	/** Blueprint-friendly accessor for the current chat connection status */
	UFUNCTION(BlueprintPure, Category = "Chat Subsystem")
	EChatConnectionStatus GetChatConnectionStatus() const { return ChatConnectionStatus; }
};
