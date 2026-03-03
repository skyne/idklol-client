// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NatsSubscription.generated.h"

/**
 * Opaque handle returned by UNatsClientSubsystem::Subscribe.
 * Pass back to Unsubscribe to cancel the subscription.
 * When the handle goes out of scope the subscription is NOT automatically cancelled —
 * call Unsubscribe explicitly unless the subsystem is shutting down.
 */
USTRUCT(BlueprintType)
struct NATSCLIENT_API FNatsSubscriptionHandle
{
	GENERATED_BODY()

	/** Internal subscription ID — 0 means invalid / not subscribed. */
	UPROPERTY()
	int64 Id = 0;

	bool IsValid() const { return Id != 0; }
};
