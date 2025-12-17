// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "ChatSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnNewMessageReceivedSignature, FString, Timestamp, FString, Sender, FString, Message);

/**
 * 
 */
UCLASS()
class TPSCOREMECHANICS_API UChatSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()
	
public:
	
	UFUNCTION(BlueprintCallable)
	void NewChatMessage(FString Message);
	
	// Call this from C++ to trigger the Blueprint event
	UPROPERTY(BlueprintAssignable, Category = "Chat Subsystem")
	FOnNewMessageReceivedSignature OnNewMessageReceived;
	
	UFUNCTION(BlueprintCallable, Category = "Chat Subsystem")
	void TriggerNewMessageReceived(const FString& Timestamp,const FString& Sender,const FString& Message);
};
