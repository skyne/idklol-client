#include "UI/WidgetController/ChatWidgetController.h"

#include "Game/PlayerController/TPSCorePlayerController.h"

void UChatWidgetController::SetOwningActor(AActor* InOwningActor)
{
	OwningPlayerController = Cast<ATPSCorePlayerController>(InOwningActor);
}

void UChatWidgetController::BindCallbacksToDependencies()
{
	if (!IsValid(OwningPlayerController))
	{
		return;
	}

	if (UGameInstance* GameInstance = OwningPlayerController->GetGameInstance())
	{
		ChatSubsystem = GameInstance->GetSubsystem<UChatSubsystem>();
	}

	if (IsValid(ChatSubsystem))
	{
		ChatSubsystem->OnChatMessageAdded.AddUniqueDynamic(this, &UChatWidgetController::HandleChatMessageAdded);
		ChatSubsystem->OnChatHistoryReset.AddUniqueDynamic(this, &UChatWidgetController::HandleChatHistoryReset);
		ChatSubsystem->OnConnectionStatusChanged.AddUniqueDynamic(this, &UChatWidgetController::HandleConnectionStatusChanged);
	}

	OwningPlayerController->OnInteractiveWindowFocusChanged.AddUniqueDynamic(this, &UChatWidgetController::HandleInteractiveWindowFocusChanged);
}

void UChatWidgetController::BroadcastInitialValues()
{
	if (IsValid(ChatSubsystem))
	{
		TArray<FChatMessageEnvelope> ExistingMessages;
		ChatSubsystem->GetMessageHistory(ExistingMessages);

		HistoryResetDelegate.Broadcast();
		for (const FChatMessageEnvelope& Message : ExistingMessages)
		{
			MessageAddedDelegate.Broadcast(Message);
		}
		HistorySyncCompleteDelegate.Broadcast();
		ConnectionStatusChangedDelegate.Broadcast(ChatSubsystem->GetConnectionStatus());
	}

	if (IsValid(OwningPlayerController))
	{
		ChatWindowFocusChangedDelegate.Broadcast(OwningPlayerController->IsChatWindowFocused());
	}
}

void UChatWidgetController::SendChatMessage(const FString& Message)
{
	if (IsValid(ChatSubsystem))
	{
		ChatSubsystem->SendChatMessage(Message);
	}
}

void UChatWidgetController::SetChatWindowFocus(bool bShouldFocus)
{
	if (IsValid(OwningPlayerController))
	{
		OwningPlayerController->SetChatWindowFocus(bShouldFocus);
	}
}

void UChatWidgetController::HandleChatMessageAdded(FChatMessageEnvelope Message)
{
	MessageAddedDelegate.Broadcast(Message);
}

void UChatWidgetController::HandleChatHistoryReset()
{
	HistoryResetDelegate.Broadcast();
}

void UChatWidgetController::HandleConnectionStatusChanged(EGrpcConnectionStatus NewStatus)
{
	ConnectionStatusChangedDelegate.Broadcast(NewStatus);
}

void UChatWidgetController::HandleInteractiveWindowFocusChanged(bool bHasInteractiveWindowFocus, FName FocusReason)
{
	ChatWindowFocusChangedDelegate.Broadcast(bHasInteractiveWindowFocus && FocusReason == FName(TEXT("Chat")));
}