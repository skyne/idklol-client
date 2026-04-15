// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NatsMessage.generated.h"

/**
 * A single NATS message — subject + raw payload bytes.
 * The payload is typically UTF-8 JSON; use FString / TSharedPtr<FJsonObject>
 * for higher-level access via UNatsClientSubsystem helpers.
 */
USTRUCT(BlueprintType)
struct NATSCLIENT_API FNatsMessage
{
	GENERATED_BODY()

	/** Subject the message was published to / received on. */
	UPROPERTY(BlueprintReadOnly, Category = "NATS")
	FString Subject;

	/** Optional reply-to subject for the request/reply pattern. */
	UPROPERTY(BlueprintReadOnly, Category = "NATS")
	FString ReplyTo;

	/** Raw payload bytes. */
	UPROPERTY(BlueprintReadOnly, Category = "NATS")
	TArray<uint8> Payload;

	/** Convenience: interpret Payload as UTF-8 string. */
	FString PayloadAsString() const
	{
		if (Payload.Num() == 0)
		{
			return FString();
		}

		const ANSICHAR* PayloadData = reinterpret_cast<const ANSICHAR*>(Payload.GetData());
		FUTF8ToTCHAR Utf8ToTCHAR(PayloadData, Payload.Num());
		FString Result;
		Result.AppendChars(Utf8ToTCHAR.Get(), Utf8ToTCHAR.Length());
		return Result;
	}
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnNatsMessage, const FNatsMessage&, Message);
DECLARE_DELEGATE_TwoParams(FOnNatsReply, bool, const FNatsMessage&);
