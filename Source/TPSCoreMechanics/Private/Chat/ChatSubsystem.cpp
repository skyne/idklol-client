// Fill out your copyright notice in the Description page of Project Settings.


#include "Chat/ChatSubsystem.h"

void UChatSubsystem::NewChatMessage(FString Message)
{
	UE_LOG(LogTemp, Warning, TEXT("NewChatMessage"));
}

void UChatSubsystem::TriggerNewMessageReceived(const FString& Timestamp,const FString& Sender,const FString& Message)
{
	OnNewMessageReceived.Broadcast(Timestamp, Sender, Message);
}