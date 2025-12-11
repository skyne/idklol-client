// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerController.h"
#include "TPSCorePlayerController.generated.h"

class UInventoryComponent;

UCLASS()
class TPSCOREMECHANICS_API ATPSCorePlayerController : public APlayerController, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ATPSCorePlayerController();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true), Replicated)
	TObjectPtr<UInventoryComponent> InventoryComponent;
};
