// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/PlayerController/TPSCorePlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Inventory/InventoryComponent.h"
#include "Net/UnrealNetwork.h"

ATPSCorePlayerController::ATPSCorePlayerController()
{
	bReplicates = true;

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>("InventoryComponent");
	InventoryComponent->SetIsReplicated(true);
}

UAbilitySystemComponent* ATPSCorePlayerController::GetAbilitySystemComponent() const
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn());
}

void ATPSCorePlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATPSCorePlayerController, InventoryComponent);
}
