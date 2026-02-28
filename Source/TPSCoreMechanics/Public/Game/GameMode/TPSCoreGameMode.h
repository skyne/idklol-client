// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "TPSCoreGameMode.generated.h"

class UCharacterClassInfo;
class ATPSCoreMechanicsCharacter;

UCLASS()
class TPSCOREMECHANICS_API ATPSCoreGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ATPSCoreGameMode();
	
	virtual void BeginPlay() override;
	
	UCharacterClassInfo* GetCharacterClassDefaultInfo() const;

private:
	UPROPERTY(EditDefaultsOnly, Category="Custom Values|Class Defaults")
	TObjectPtr<UCharacterClassInfo> ClassDefaults;
};
