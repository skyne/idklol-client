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

UObject* ATPSCorePlayerController::GetInventoryWidgetController()
{
	if (!IsValid(InventoryWidgetController))
	{
		InventoryWidgetController = NewObject<UObject>(this, InventoryWidgetControllerClass);

		if (UFunction* SetOwningActorFn = InventoryWidgetController->FindFunction(TEXT("SetOwningActor")))
		{
			struct FSetOwningActorParams
			{
				AActor* InOwningActor;
			};
			FSetOwningActorParams Params{ this };
			InventoryWidgetController->ProcessEvent(SetOwningActorFn, &Params);
		}

		if (UFunction* BindCallbacksFn = InventoryWidgetController->FindFunction(TEXT("BindCallbacksToDependencies")))
		{
			InventoryWidgetController->ProcessEvent(BindCallbacksFn, nullptr);
		}
	}
	return InventoryWidgetController;
}

void ATPSCorePlayerController::CreateInventoryWidget()
{
	if (UUserWidget* Widget = CreateWidget<UUserWidget>(this, InventoryWidgetClass))
	{
		InventoryWidget = Widget;

		if (UFunction* SetWidgetControllerFn = InventoryWidget->FindFunction(TEXT("SetWidgetController")))
		{
			struct FSetWidgetControllerParams
			{
				UObject* InWidgetController;
			};
			FSetWidgetControllerParams Params{ GetInventoryWidgetController() };
			InventoryWidget->ProcessEvent(SetWidgetControllerFn, &Params);
		}

		if (IsValid(InventoryWidgetController))
		{
			if (UFunction* BroadcastInitialValuesFn = InventoryWidgetController->FindFunction(TEXT("BradcastInitialValues")))
			{
				InventoryWidgetController->ProcessEvent(BroadcastInitialValuesFn, nullptr);
			}
		}

		InventoryWidget->AddToViewport();
	}
}
