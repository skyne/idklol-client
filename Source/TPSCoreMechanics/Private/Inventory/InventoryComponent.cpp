// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/InventoryComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "ItemTypes.h"
#include "Inventory/ItemTypesToTables.h"
#include "Libraries/TPSCoreAbilitySystemLibrary.h"
#include "Net/UnrealNetwork.h"

// serializer template generation for TArray<T>
#pragma region

constexpr int32 MAX_INVENTORY_SIZE = 100;

template <int32 MaxNum, class T, class A>
bool MySafeNetSerializeTArray_WithNetSerialize(FArchive& Ar, TArray<T, A>& Array, UPackageMap* PackageMap)
{
	// Implementation that calls NetSerialize on each element
	// Enforces MaxNum limit for safety
	bool bOutSuccess = true;
	if (Ar.IsLoading())
	{
		int32 NumElements;
		Ar << NumElements;
		if (NumElements > MaxNum)
		{
			UE_LOG(LogTemp, Error, TEXT("Array size %d exceeds maximum %d"), NumElements, MaxNum);
			return false;
		}
		Array.SetNum(NumElements);
		for (int32 i = 0; i < NumElements; ++i)
		{
			Array[i].NetSerialize(Ar, PackageMap, bOutSuccess);
		}
	}
	else
	{
		int32 NumElements = Array.Num();
		Ar << NumElements;
		for (int32 i = 0; i < NumElements; ++i)
		{
			Array[i].NetSerialize(Ar, PackageMap, bOutSuccess);
		}
	}
	return bOutSuccess;
}

// Function variant 2: Default serialization
template <int32 MaxNum, class T, class A>
bool MySafeNetSerializeTArray_Default(FArchive& Ar, TArray<T, A>& Array, UPackageMap* PackageMap)
{
	// Implementation using standard Unreal serialization
	// Enforces MaxNum limit for safety
	if (Ar.IsLoading())
	{
		int32 NumElements;
		Ar << NumElements;
		if (NumElements > MaxNum)
		{
			UE_LOG(LogTemp, Error, TEXT("Array size %d exceeds maximum %d"), NumElements, MaxNum);
			return false;
		}
		Array.SetNum(NumElements);
		for (int32 i = 0; i < NumElements; ++i)
		{
			Ar << Array[i];
		}
	}
	else
	{
		int32 NumElements = Array.Num();
		Ar << NumElements;
		for (int32 i = 0; i < NumElements; ++i)
		{
			Ar << Array[i];
		}
	}
	return true;
}

#pragma endregion

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryComponent::AddItem(const FGameplayTag& ItemTag, int32 NumItems)
{
	AActor* Owner = GetOwner();

	if (!IsValid(Owner))
	{
		return;
	}

	// delegate to server if called on client
	if (!Owner->HasAuthority())
	{
		ServerAddItem(ItemTag, NumItems);
		return;
	}

	// server side
	if (InventoryTagMap.Contains(ItemTag))
	{
		InventoryTagMap[ItemTag] += NumItems;
	}
	else
	{
		InventoryTagMap.Emplace(ItemTag, NumItems);
	}
	
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan,
	                                 FString::Printf(
		                                 TEXT("Server - %d item added to inventory: %s"), NumItems,
		                                 *ItemTag.ToString()));

	PackageInventory(CachedInventory);
}

void UInventoryComponent::UseItem(const FGameplayTag& ItemTag, int32 NumItems)
{
	AActor* Owner = GetOwner();

	if (!IsValid(Owner))
	{
		return;
	}

	// delegate to server if called on client
	if (!Owner->HasAuthority())
	{
		ServerUseItem(ItemTag, NumItems);
		return;
	}

	// server side
	const FMasterItemDefinition Item = GetItemDefinitionByTag(ItemTag);
	
	if (UAbilitySystemComponent* OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
	{
		if (IsValid(Item.ConsumableProps.ItemEffectClass))
		{
			const FGameplayEffectContextHandle ContextHandle = OwnerASC->MakeEffectContext();
			const FGameplayEffectSpecHandle SpecHandle = OwnerASC->MakeOutgoingSpec(
				Item.ConsumableProps.ItemEffectClass, Item.ConsumableProps.ItemEffectLevel, ContextHandle);
			OwnerASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

			AddItem(ItemTag, -1);

			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
			                                 FString::Printf(TEXT("Server - Item used: %s"), *Item.ItemTag.ToString()));
		}
	}
}

FMasterItemDefinition UInventoryComponent::GetItemDefinitionByTag(const FGameplayTag& ItemTag) const
{
	checkf(InventoryDefinitions, TEXT("No inventory definitions for component %s"), *GetNameSafe(this));

	for (const auto& Pair : InventoryDefinitions->TagsToTables)
	{
		if (ItemTag.MatchesTag(Pair.Key))
		{
			return *UTPSCoreAbilitySystemLibrary::GetDataTableRowByTag<FMasterItemDefinition>(Pair.Value, ItemTag);
		}
	}
	return FMasterItemDefinition();
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, CachedInventory);
}


void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
}

// rpc call to server for adding item
void UInventoryComponent::ServerAddItem_Implementation(const FGameplayTag& ItemTag, int32 NumItems)
{
	AddItem(ItemTag, NumItems);
}

// rpc call to server for using item
void UInventoryComponent::ServerUseItem_Implementation(const FGameplayTag& ItemTag, int32 NumItems)
{
	UseItem(ItemTag, NumItems);
}


void UInventoryComponent::PackageInventory(FPackagedInventory& OutInventory)
{
	OutInventory.ItemTags.Empty();
	OutInventory.ItemQuantities.Empty();

	for (const auto& Pair : InventoryTagMap)
	{
		if (Pair.Value > 0)
		{
			OutInventory.ItemTags.Add(Pair.Key);
			OutInventory.ItemQuantities.Add(Pair.Value);
		}
	}
}

void UInventoryComponent::ReconstructInventoryMap(const FPackagedInventory& Inventory)
{
	InventoryTagMap.Empty();
	for (int32 i = 0; i < Inventory.ItemTags.Num(); ++i)
	{
		InventoryTagMap.Emplace(Inventory.ItemTags[i], Inventory.ItemQuantities[i]);
	}
}

void UInventoryComponent::OnRep_CachedInventory()
{
	ReconstructInventoryMap(CachedInventory);
}

bool FPackagedInventory::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	MySafeNetSerializeTArray_WithNetSerialize<MAX_INVENTORY_SIZE>(Ar, ItemTags, Map);
	MySafeNetSerializeTArray_Default<MAX_INVENTORY_SIZE>(Ar, ItemQuantities, Map);
	bOutSuccess = true;
	return true;
}
