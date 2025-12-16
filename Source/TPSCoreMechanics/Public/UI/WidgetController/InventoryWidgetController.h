// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/WidgetController.h"
#include "InventoryWidgetController.generated.h"

struct FMasterItemDefinition;
struct FPackagedInventory;
class UInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInventoryItemSignature, const FMasterItemDefinition&, Item);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInventoryBroadcastComplete);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FScrollBoxResetSignature);

UCLASS(Blueprintable, BlueprintType)
class TPSCOREMECHANICS_API UInventoryWidgetController : public UWidgetController
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FInventoryItemSignature InventoryItemDelegate;

	UPROPERTY(BlueprintAssignable)
	FInventoryBroadcastComplete InventoryBroadcastCompleteDelegate;

	UPROPERTY(BlueprintAssignable)
	FScrollBoxResetSignature ScrollBoxResetDelegate;

	void SetOwningActor(AActor* InOwningActor);

	void BindCallbacksToDependencies();

	void BradcastInitialValues();

private:
	void UpdateInventory(const FPackagedInventory& InventoryContents);

	void BroadcastInventoryContents();

	UPROPERTY()
	TObjectPtr<UInventoryComponent> OwningInventory;

	UPROPERTY()
	TObjectPtr<AActor> OwningActor;
};
