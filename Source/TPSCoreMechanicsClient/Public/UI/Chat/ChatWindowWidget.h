#pragma once

#include "CoreMinimal.h"
#include "UI/WindowWidget.h"
#include "Chat/ChatSubsystem.h"
#include "ChatWindowWidget.generated.h"

class UChatMessageRowWidget;
class UPanelWidget;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChatWindowMessageSubmittedSignature, FString, Message);

UCLASS(Abstract, Blueprintable)
class TPSCOREMECHANICSCLIENT_API UChatWindowWidget : public UWindowWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Chat")
	UPanelWidget* GetMessageListContainer() const;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Chat")
	UWidget* GetTitleBarWidget() const;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Chat")
	UWidget* GetTextEntrySurface() const;

	UFUNCTION(BlueprintCallable, Category = "Chat")
	void SetMessageHistory(const TArray<FChatMessageEnvelope>& InMessages);

	UFUNCTION(BlueprintCallable, Category = "Chat")
	void AppendChatMessage(const FChatMessageEnvelope& InMessage);

	UFUNCTION(BlueprintCallable, Category = "Chat")
	void ClearChatMessages();

	UFUNCTION(BlueprintCallable, Category = "Chat")
	void SetActiveChannelId(const FString& InChannelId);

	UFUNCTION(BlueprintCallable, Category = "Chat")
	void SetVisibleMessageKinds(const TArray<EChatMessageKind>& InVisibleMessageKinds);

	UFUNCTION(BlueprintCallable, Category = "Chat")
	void SetChatWindowFocused(bool bFocused);

	UFUNCTION(BlueprintCallable, Category = "Chat")
	void SetWindowTitleText(const FText& InWindowTitleText);

	UFUNCTION(BlueprintCallable, Category = "Chat")
	void SetChatModeDisplayText(const FText& InChatModeDisplayText);

	UFUNCTION(BlueprintCallable, Category = "Chat")
	void SubmitPendingMessage(const FString& Message);

	UPROPERTY(BlueprintAssignable, Category = "Chat")
	FOnChatWindowMessageSubmittedSignature OnMessageSubmitted;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Chat")
	void OnChatWindowStateChanged();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chat")
	TSubclassOf<UChatMessageRowWidget> ChatMessageRowWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "Chat")
	TArray<FChatMessageEnvelope> MessageHistory;

	UPROPERTY(BlueprintReadOnly, Category = "Chat")
	FString ActiveChannelId;

	UPROPERTY(BlueprintReadOnly, Category = "Chat")
	TArray<EChatMessageKind> VisibleMessageKinds;

	UPROPERTY(BlueprintReadOnly, Category = "Chat")
	FText WindowTitleText;

	UPROPERTY(BlueprintReadOnly, Category = "Chat")
	FText ChatModeDisplayText;

	UPROPERTY(BlueprintReadOnly, Category = "Chat")
	bool bChatWindowFocused = false;

private:
	void RebuildMessageList();
	bool ShouldDisplayMessage(const FChatMessageEnvelope& Message) const;
};