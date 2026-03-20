#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NpcInteractionResultWidget.generated.h"

class UTextBlock;

UCLASS(BlueprintType)
class TPSCOREMECHANICS_API UNpcInteractionResultWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "NPC|Interaction")
	void SetContext(const FText& InTitle, const FText& InMessage);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "NPC|Interaction")
	FText DefaultTitle = FText::FromString(TEXT("NPC Interaction"));

	UPROPERTY(EditDefaultsOnly, Category = "NPC|Interaction")
	FText DefaultMessage = FText::FromString(TEXT("You start a conversation."));

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleTextBlock;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MessageTextBlock;
};
