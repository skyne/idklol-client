// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NpcInteractionPromptWidget.generated.h"

class UButton;
class UTextBlock;

UCLASS(BlueprintType)
class TPSCOREMECHANICS_API UNpcInteractionPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "NPC|Interaction")
	void SetPromptText(const FText& InText);

	UFUNCTION(BlueprintCallable, Category = "NPC|Interaction")
	void SetPromptScreenPosition(const FVector2D& InScreenPosition);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "NPC|Interaction")
	FText DefaultPromptText = FText::FromString(TEXT("Interact"));

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> PromptButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PromptTextBlock;
};
