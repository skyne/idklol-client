// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/PlayerController/TPSCorePlayerController.h"
#include "Chat/ChatSubsystem.h"
#include "Engine/GameInstance.h"

void ATPSCorePlayerController::ClientSendNpcChatMessage_Implementation(
	const FString& Sender,
	const FString& Message)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UChatSubsystem* ChatSubsystem = GameInstance->GetSubsystem<UChatSubsystem>())
		{
			ChatSubsystem->SendChatMessage(Message, Sender);
		}
	}
}
