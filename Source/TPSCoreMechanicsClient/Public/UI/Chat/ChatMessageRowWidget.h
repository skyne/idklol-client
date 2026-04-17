#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Chat/ChatSubsystem.h"
#include "ChatMessageRowWidget.generated.h"

UCLASS(Abstract, Blueprintable)
class TPSCOREMECHANICSCLIENT_API UChatMessageRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void SetMessageEnvelope(const FChatMessageEnvelope& InMessage);

	UFUNCTION(BlueprintPure, Category = "Chat")
	FChatMessageEnvelope GetMessageEnvelope() const { return MessageEnvelope; }

	UFUNCTION(BlueprintPure, Category = "Chat")
	bool IsSelfAuthoredMessage() const { return MessageEnvelope.bIsSelfAuthored; }

	UFUNCTION(BlueprintPure, Category = "Chat")
	bool IsSystemStyleMessage() const { return MessageEnvelope.bIsServiceMessage; }

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Chat")
	void OnMessageEnvelopeSet();

	UPROPERTY(BlueprintReadOnly, Category = "Chat")
	FChatMessageEnvelope MessageEnvelope;

	UPROPERTY(BlueprintReadOnly, Category = "Chat")
	FText SpeakerText;

	UPROPERTY(BlueprintReadOnly, Category = "Chat")
	FText PayloadText;

	UPROPERTY(BlueprintReadOnly, Category = "Chat")
	FText TimestampText;
};