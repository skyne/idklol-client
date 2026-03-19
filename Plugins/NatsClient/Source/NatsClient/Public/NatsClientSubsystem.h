// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NatsMessage.h"
#include "NatsSubscription.h"
#include "NatsClientSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNatsConnected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNatsConnectionLost, FString, Reason);

/**
 * NATS client subsystem — wraps nats.c for Unreal Engine.
 *
 * Usage:
 *   auto* Nats = GetGameInstance()->GetSubsystem<UNatsClientSubsystem>();
 *   Nats->Connect("nats://localhost:4222", "");
 *   Nats->PublishJson("server.evt.player", "{ \"id\": \"...\"}");
 *   auto Handle = Nats->Subscribe("server.srv1.map", MyDelegate);
 *
 * When NATS_CLIENT_NOT_AVAILABLE is defined (nats.c not yet vendored),
 * all methods compile and log a warning but are no-ops.
 */
UCLASS()
class NATSCLIENT_API UNatsClientSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ── USubsystem ────────────────────────────────────────────────────────────

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ── Connection ────────────────────────────────────────────────────────────

	/**
	 * Connect to a NATS server.
	 * @param Url            e.g. "nats://localhost:4222"
	 * @param CredentialsOrNKey  Path to a .creds/chained creds file; empty for anonymous
	 */
	UFUNCTION(BlueprintCallable, Category = "NATS")
	void Connect(const FString& Url, const FString& CredentialsOrNKey = TEXT(""));

	UFUNCTION(BlueprintCallable, Category = "NATS")
	void Disconnect();

	UFUNCTION(BlueprintCallable, Category = "NATS")
	bool IsConnected() const;

	/** Fired on the game thread when the connection is established. */
	UPROPERTY(BlueprintAssignable, Category = "NATS")
	FOnNatsConnected OnConnected;

	/** Fired on the game thread when the connection drops. */
	UPROPERTY(BlueprintAssignable, Category = "NATS")
	FOnNatsConnectionLost OnConnectionLost;

	// ── Publish ───────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "NATS")
	void PublishJson(const FString& Subject, const FString& JsonPayload);

	UFUNCTION(BlueprintCallable, Category = "NATS")
	void PublishBytes(const FString& Subject, const TArray<uint8>& Payload);

	// ── Request/Reply ─────────────────────────────────────────────────────────

	/**
	 * Send a request and invoke Callback with the reply on the game thread.
	 * @param TimeoutSeconds  How long to wait before firing with bSuccess=false
	 */
	void RequestJson(
		const FString& Subject,
		const FString& JsonPayload,
		float TimeoutSeconds,
		const FOnNatsReply& Callback
	);

	// ── Subscribe ─────────────────────────────────────────────────────────────

	/**
	 * Subscribe to a subject (wildcards supported: * and >).
	 * The delegate is invoked on the game thread.
	 * @return Handle to pass to Unsubscribe when done.
	 */
	UFUNCTION(BlueprintCallable, Category = "NATS")
	FNatsSubscriptionHandle Subscribe(const FString& Subject, const FOnNatsMessage& Delegate);

	UFUNCTION(BlueprintCallable, Category = "NATS")
	void Unsubscribe(const FNatsSubscriptionHandle& Handle);

private:
	struct FSubscriptionEntry
	{
		FString Subject;
		FOnNatsMessage Delegate;
		void* NativeSubscription = nullptr; // natsSubscription*
	};

	TMap<int64, FSubscriptionEntry> Subscriptions;
	int64 NextSubscriptionId = 1;

	void* NatsConnection = nullptr; // natsConnection*
	bool bConnected = false;

	/** Called by nats.c on its thread — marshals to game thread. */
	static void NatsMsgHandler(void* NatsConn, void* NatsSub, void* NatsMsg, void* UserData);
};
