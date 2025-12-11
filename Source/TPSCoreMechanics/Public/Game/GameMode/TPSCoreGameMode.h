// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "TPSCoreGameMode.generated.h"

class UCharacterClassInfo;

UCLASS()
class TPSCOREMECHANICS_API ATPSCoreGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	UCharacterClassInfo* GetCharacterClassDefaultInfo() const;

private:
	UPROPERTY(EditDefaultsOnly, Category="Custom Values|Class Defaults")
	TObjectPtr<UCharacterClassInfo> ClassDefaults;
};
