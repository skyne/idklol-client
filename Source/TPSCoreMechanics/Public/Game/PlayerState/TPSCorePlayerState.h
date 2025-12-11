// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "TPSCorePlayerState.generated.h"

class UTPSCoreAbilitySystemComponent;
class UTPSCoreAttributeSet;

UCLASS()
class TPSCOREMECHANICS_API ATPSCorePlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ATPSCorePlayerState();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure)
	UTPSCoreAbilitySystemComponent* GetTPSCoreAbilitySystemComponent() const;

	UFUNCTION(BlueprintPure)
	UTPSCoreAttributeSet* GetAttributes() const;

private:
	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	TObjectPtr<UTPSCoreAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	TObjectPtr<UTPSCoreAttributeSet> Attributes;
};
