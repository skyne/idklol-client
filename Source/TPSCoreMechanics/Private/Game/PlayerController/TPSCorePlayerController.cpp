// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/PlayerController/TPSCorePlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Inventory/InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "UI/TPSCoreSystemsWidget.h"
#include "UI/WidgetController/InventoryWidgetController.h"
#include "UI/TPSCoreSystemsWidget.h"

ATPSCorePlayerController::ATPSCorePlayerController()
{
	bReplicates = true;

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>("InventoryComponent");
	InventoryComponent->SetIsReplicated(true);
}

UInventoryComponent* ATPSCorePlayerController::GetInventoryComponent_Implementation()
{
	return InventoryComponent;
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

UInventoryWidgetController* ATPSCorePlayerController::GetInventoryWidgetController()
{
	if (!IsValid(InventoryWidgetController))
	{
		InventoryWidgetController = NewObject<UInventoryWidgetController>(this, InventoryWidgetControllerClass);
		InventoryWidgetController->SetOwningActor(this);
		InventoryWidgetController->BindCallbacksToDependencies();
	}
	return InventoryWidgetController;
}

void ATPSCorePlayerController::CreateInventoryWidget()
{
	if (UUserWidget* Widget = CreateWidget<UTPSCoreSystemsWidget>(this, InventoryWidgetClass))
	{
		InventoryWidget = Cast<UTPSCoreSystemsWidget>(Widget);
		InventoryWidget->SetWidgetController(GetInventoryWidgetController());
		InventoryWidgetController->BradcastInitialValues();
		InventoryWidget->AddToViewport();
	}
}
