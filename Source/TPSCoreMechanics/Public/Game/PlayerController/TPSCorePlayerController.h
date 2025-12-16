// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/InventoryInterface.h"
#include "TPSCorePlayerController.generated.h"

class UTPSCoreSystemsWidget;
class UInventoryWidgetController;
class UInventoryComponent;


UCLASS()
class TPSCOREMECHANICS_API ATPSCorePlayerController : public APlayerController, public IAbilitySystemInterface,
                                                      public IInventoryInterface
{
	GENERATED_BODY()

public:
	ATPSCorePlayerController();

	virtual UInventoryComponent* GetInventoryComponent_Implementation() override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UInventoryWidgetController* GetInventoryWidgetController();

	UFUNCTION(BlueprintCallable)
	void CreateInventoryWidget();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true), Replicated)
	TObjectPtr<UInventoryComponent> InventoryComponent;

	UPROPERTY()
	TObjectPtr<UInventoryWidgetController> InventoryWidgetController;

	UPROPERTY(EditDefaultsOnly, Category="Custom Values|Widgets")
	TSubclassOf<UInventoryWidgetController> InventoryWidgetControllerClass;

	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess = true))
	TObjectPtr<UTPSCoreSystemsWidget> InventoryWidget;

	UPROPERTY(EditDefaultsOnly, Category="Custom Values|Widgets")
	TSubclassOf<UTPSCoreSystemsWidget> InventoryWidgetClass;
};
