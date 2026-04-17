#include "Game/PlayerController/TPSCorePlayerControllerClient.h"
#include "Chat/ChatSubsystem.h"
#include "Engine/GameInstance.h"
#include "TPSCoreMechanics/TPSCoreMechanics.h"

ATPSCorePlayerControllerClient::ATPSCorePlayerControllerClient()
{
}

void ATPSCorePlayerControllerClient::ClientSendNpcChatMessage_Implementation(const FString& Sender, const FString& Message)
{
	UGameInstance* GameInstance = GetGameInstance();
	if (!IsValid(GameInstance))
	{
		LOG_WARNING("ClientSendNpcChatMessage: missing game instance");
		return;
	}

	if (UChatSubsystem* ChatSubsystem = GameInstance->GetSubsystem<UChatSubsystem>())
	{
		LOG("ClientSendNpcChatMessage: routing NPC message to chat subsystem sender=%s message=%s",
			*Sender,
			*Message);
		ChatSubsystem->TriggerNewMessageReceived(TEXT("NOW"), Sender, Message);
		return;
	}

	LOG_WARNING("ClientSendNpcChatMessage: UChatSubsystem unavailable, falling back to base implementation");
	Super::ClientSendNpcChatMessage_Implementation(Sender, Message);
}
