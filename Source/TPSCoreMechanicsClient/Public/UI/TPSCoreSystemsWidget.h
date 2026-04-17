// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TPSCoreSystemsWidget.generated.h"

class UWidgetController;
/**
 * 
 */
UCLASS()
class TPSCOREMECHANICSCLIENT_API UTPSCoreSystemsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	void OnWidgetControllerSet();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetWidgetController(UWidgetController* InWidgetController);

private:
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UWidgetController> WidgetController;
};
