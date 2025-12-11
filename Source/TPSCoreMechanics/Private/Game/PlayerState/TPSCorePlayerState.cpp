// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/PlayerState/TPSCorePlayerState.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/TPSCoreAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/TPSCoreAttributeSet.h"

ATPSCorePlayerState::ATPSCorePlayerState()
{
	SetNetUpdateFrequency(100.f);
	SetMinNetUpdateFrequency(66.f);

	AbilitySystemComponent = CreateDefaultSubobject<UTPSCoreAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	Attributes = CreateDefaultSubobject<UTPSCoreAttributeSet>("AttributeSet");
}

UAbilitySystemComponent* ATPSCorePlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UTPSCoreAbilitySystemComponent* ATPSCorePlayerState::GetTPSCoreAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UTPSCoreAttributeSet* ATPSCorePlayerState::GetAttributes() const
{
	return Attributes;
}
