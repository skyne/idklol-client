#pragma once

#include "CoreMinimal.h"
#include "Game/PlayerController/TPSCorePlayerController.h"
#include "TPSCorePlayerControllerClient.generated.h"

UCLASS()
class TPSCOREMECHANICSCLIENT_API ATPSCorePlayerControllerClient : public ATPSCorePlayerController
{
    GENERATED_BODY()

public:
    ATPSCorePlayerControllerClient();

protected:
    virtual void ClientSendNpcChatMessage_Implementation(const FString& Sender, const FString& Message) override;
};
