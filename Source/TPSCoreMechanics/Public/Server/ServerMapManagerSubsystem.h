// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NatsMessage.h"
#include "NatsSubscription.h"
#include "ServerMapManagerSubsystem.generated.h"

/**
 * Server-only subsystem that manages map assignments via NATS.
 *
 * On startup it:
 * 1. Reads -InstanceId=<id> from the command line (falls back to a generated GUID).
 * 2. Subscribes to "server.<InstanceId>.map" — payload: {"map":"/Game/Maps/Zone1"}
 *    → calls GetWorld()->ServerTravel(MapURL)
 * 3. Subscribes to "server.<InstanceId>.status"
 *    → replies with {"map":"<current>","players":<n>,"uptime":<s>}
 *
 * Only active on dedicated server (ShouldCreateSubsystem guard).
 */
UCLASS()
class TPSCOREMECHANICS_API UServerMapManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	UFUNCTION(BlueprintCallable, Category = "Server Map Manager")
	FString GetInstanceId() const { return InstanceId; }

private:
	FString InstanceId;

	FNatsSubscriptionHandle MapSubscriptionHandle;
	FNatsSubscriptionHandle StatusSubscriptionHandle;

	UFUNCTION()
	void OnMapMessage(const FNatsMessage& Message);

	UFUNCTION()
	void OnStatusMessage(const FNatsMessage& Message);

	void TravelToMap(const FString& MapPath);
};
