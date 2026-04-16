#include "Game/PlayerController/TPSCorePlayerControllerClient.h"
#include "Chat/ChatSubsystem.h"
#include "Engine/GameInstance.h"

ATPSCorePlayerControllerClient::ATPSCorePlayerControllerClient()
{
}

void ATPSCorePlayerControllerClient::ClientSendNpcChatMessage_Implementation(const FString& Sender, const FString& Message)
{
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UChatSubsystem* ChatSubsystem = GameInstance->GetSubsystem<UChatSubsystem>())
        {
            ChatSubsystem->SendChatMessage(Message, Sender);
        }
    }
}
