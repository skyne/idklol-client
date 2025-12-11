// Fill out your copyright notice in the Description page of Project Settings.


#include "Libraries/TPSCoreAbilitySystemLibrary.h"
#include "Game/GameMode/TPSCoreGameMode.h"
#include "Kismet/GameplayStatics.h"

UCharacterClassInfo* UTPSCoreAbilitySystemLibrary::GetCharacterClassDefaultInfo(const UObject* WorldContextObject)
{
	if (const ATPSCoreGameMode* GameMode = Cast<ATPSCoreGameMode>(UGameplayStatics::GetGameMode(WorldContextObject)))
	{
		return GameMode->GetCharacterClassDefaultInfo();
	}

	return nullptr;
}
