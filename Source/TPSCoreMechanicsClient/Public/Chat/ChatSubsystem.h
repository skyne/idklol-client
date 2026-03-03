// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GrpcHandlerSubsystem.h"
#include "SChat/ChatService.h"
#include "ChatSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnNewMessageReceivedSignature, FString, Timestamp, FString, Sender, FString, Message);

/**
 * Chat subsystem - handles chat service gRPC connection and messaging
 */
UCLASS(config=Game)
class TPSCOREMECHANICSCLIENT_API UChatSubsystem : public UGrpcHandlerSubsystem
{
	GENERATED_BODY()
	
protected:
	DECLARE_GRPC_SUBSYSTEM_TYPES(ChatService)
	
	// Override base class methods
	virtual void OnServiceConnected(UObject* InService, UObject* InClient) override;
	virtual void OnServiceDisconnected() override;
	virtual void HandleGrpcStateChange(EGrpcServiceState ServiceState) override;
	
	// Chat-specific stream response handler
	UFUNCTION()
	void HandleStreamResponse(FGrpcContextHandle Handle, const FGrpcResult& GrpcResult, const FGrpcChatChatMessage& Response);
	
public:
	// Chat-specific functionality
	UFUNCTION(BlueprintCallable, Category = "Chat Subsystem")
	void NewChatMessage(FString Message);
	
	/** Event broadcast when a new chat message is received */
	UPROPERTY(BlueprintAssignable, Category = "Chat Subsystem")
	FOnNewMessageReceivedSignature OnNewMessageReceived;
	
	UFUNCTION(BlueprintCallable, Category = "Chat Subsystem")
	void TriggerNewMessageReceived(const FString& Timestamp, const FString& Sender, const FString& Message);
};