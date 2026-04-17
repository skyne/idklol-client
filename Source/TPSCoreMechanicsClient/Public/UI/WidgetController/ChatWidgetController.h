#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/WidgetController.h"
#include "Chat/ChatSubsystem.h"
#include "ChatWidgetController.generated.h"

class ATPSCorePlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FChatWidgetControllerMessageSignature, FChatMessageEnvelope, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FChatWidgetControllerHistoryResetSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FChatWidgetControllerHistorySyncCompleteSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FChatWidgetControllerConnectionStatusSignature, EGrpcConnectionStatus, ConnectionStatus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FChatWidgetControllerFocusSignature, bool, bIsFocused);

UCLASS(Blueprintable, BlueprintType)
class TPSCOREMECHANICSCLIENT_API UChatWidgetController : public UWidgetController
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FChatWidgetControllerMessageSignature MessageAddedDelegate;

	UPROPERTY(BlueprintAssignable)
	FChatWidgetControllerHistoryResetSignature HistoryResetDelegate;

	UPROPERTY(BlueprintAssignable)
	FChatWidgetControllerHistorySyncCompleteSignature HistorySyncCompleteDelegate;

	UPROPERTY(BlueprintAssignable)
	FChatWidgetControllerConnectionStatusSignature ConnectionStatusChangedDelegate;

	UPROPERTY(BlueprintAssignable)
	FChatWidgetControllerFocusSignature ChatWindowFocusChangedDelegate;

	UFUNCTION(BlueprintCallable, Category = "Chat")
	void SetOwningActor(AActor* InOwningActor);

	UFUNCTION(BlueprintCallable, Category = "Chat")
	void BindCallbacksToDependencies();

	UFUNCTION(BlueprintCallable, Category = "Chat")
	void BroadcastInitialValues();

	UFUNCTION(BlueprintCallable, Category = "Chat")
	void SendChatMessage(const FString& Message);

	UFUNCTION(BlueprintCallable, Category = "Chat")
	void SetChatWindowFocus(bool bShouldFocus);

private:
	UFUNCTION()
	void HandleChatMessageAdded(FChatMessageEnvelope Message);

	UFUNCTION()
	void HandleChatHistoryReset();

	UFUNCTION()
	void HandleConnectionStatusChanged(EGrpcConnectionStatus NewStatus);

	UFUNCTION()
	void HandleInteractiveWindowFocusChanged(bool bHasInteractiveWindowFocus, FName FocusReason);

	UPROPERTY()
	TObjectPtr<ATPSCorePlayerController> OwningPlayerController;

	UPROPERTY()
	TObjectPtr<UChatSubsystem> ChatSubsystem;
};