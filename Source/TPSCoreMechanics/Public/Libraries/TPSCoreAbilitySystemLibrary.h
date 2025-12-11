// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TPSCoreAbilitySystemLibrary.generated.h"

class UCharacterClassInfo;
/**
 * 
 */
UCLASS()
class TPSCOREMECHANICS_API UTPSCoreAbilitySystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure)
	static UCharacterClassInfo* GetCharacterClassDefaultInfo(const UObject* WorldContextObject);

	template <typename T>
	static T* GetDataTableRowByTag(UDataTable* DataTable, FGameplayTag Tag);
};

template <typename T>
T* UTPSCoreAbilitySystemLibrary::GetDataTableRowByTag(UDataTable* DataTable, FGameplayTag Tag)
{
	return DataTable->FindRow<T>(Tag.GetTagName(), FString(""));
}
