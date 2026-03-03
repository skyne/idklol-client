// Fill out your copyright notice in the Description page of Project Settings.

#include "NatsClientSubsystem.h"
#include "Async/Async.h"

#if !NATS_CLIENT_NOT_AVAILABLE
// When nats.c is vendored, include its header here:
// #include "nats/nats.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogNatsClient, Log, All);

// ─── USubsystem ──────────────────────────────────────────────────────────────

void UNatsClientSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogNatsClient, Log, TEXT("NatsClientSubsystem initialized"));
}

void UNatsClientSubsystem::Deinitialize()
{
	Disconnect();
	Super::Deinitialize();
}

// ─── Connection ──────────────────────────────────────────────────────────────

void UNatsClientSubsystem::Connect(const FString& Url, const FString& CredentialsOrNKey)
{
#if NATS_CLIENT_NOT_AVAILABLE
	UE_LOG(LogNatsClient, Warning, TEXT("Connect called but nats.c is not vendored. Place prebuilt libs under Plugins/NatsClient/Source/NatsClient/ThirdParty/nats.c/"));
	return;
#else
	// TODO: implement with nats.c
	// natsOptions* opts = nullptr;
	// natsOptions_Create(&opts);
	// natsOptions_SetURL(opts, TCHAR_TO_UTF8(*Url));
	// if (!CredentialsOrNKey.IsEmpty())
	//     natsOptions_SetNKey(opts, TCHAR_TO_UTF8(*CredentialsOrNKey), nullptr, nullptr);
	// natsStatus s = natsConnection_Connect((natsConnection**)&NatsConnection, opts);
	// natsOptions_Destroy(opts);
	// if (s == NATS_OK)
	// {
	//     bConnected = true;
	//     AsyncTask(ENamedThreads::GameThread, [this]() { OnConnected.Broadcast(); });
	// }
	UE_LOG(LogNatsClient, Log, TEXT("Connect: %s (nats.c stub)"), *Url);
#endif
}

void UNatsClientSubsystem::Disconnect()
{
#if !NATS_CLIENT_NOT_AVAILABLE
	if (NatsConnection)
	{
		// natsConnection_Destroy((natsConnection*)NatsConnection);
		NatsConnection = nullptr;
	}
#endif
	bConnected = false;
	Subscriptions.Empty();
}

bool UNatsClientSubsystem::IsConnected() const
{
	return bConnected;
}

// ─── Publish ─────────────────────────────────────────────────────────────────

void UNatsClientSubsystem::PublishJson(const FString& Subject, const FString& JsonPayload)
{
#if NATS_CLIENT_NOT_AVAILABLE
	UE_LOG(LogNatsClient, Warning, TEXT("PublishJson: nats.c not available. Subject=%s"), *Subject);
	return;
#else
	// TODO: implement with nats.c
	// natsConnection_PublishString((natsConnection*)NatsConnection,
	//     TCHAR_TO_UTF8(*Subject), TCHAR_TO_UTF8(*JsonPayload));
	UE_LOG(LogNatsClient, Verbose, TEXT("PublishJson [%s]: %s"), *Subject, *JsonPayload);
#endif
}

void UNatsClientSubsystem::PublishBytes(const FString& Subject, const TArray<uint8>& Payload)
{
#if NATS_CLIENT_NOT_AVAILABLE
	UE_LOG(LogNatsClient, Warning, TEXT("PublishBytes: nats.c not available. Subject=%s"), *Subject);
	return;
#else
	// TODO: implement with nats.c
	// natsConnection_Publish((natsConnection*)NatsConnection,
	//     TCHAR_TO_UTF8(*Subject), Payload.GetData(), Payload.Num());
#endif
}

// ─── Request/Reply ────────────────────────────────────────────────────────────

void UNatsClientSubsystem::RequestJson(
	const FString& Subject,
	const FString& JsonPayload,
	float TimeoutSeconds,
	const FOnNatsReply& Callback)
{
#if NATS_CLIENT_NOT_AVAILABLE
	UE_LOG(LogNatsClient, Warning, TEXT("RequestJson: nats.c not available. Subject=%s"), *Subject);
	FNatsMessage EmptyReply;
	Callback.ExecuteIfBound(false, EmptyReply);
	return;
#else
	// TODO: implement with nats.c
	// natsMsg* replyMsg = nullptr;
	// int64_t timeoutMs = static_cast<int64_t>(TimeoutSeconds * 1000.0f);
	// natsStatus s = natsConnection_RequestString(&replyMsg, (natsConnection*)NatsConnection,
	//     TCHAR_TO_UTF8(*Subject), TCHAR_TO_UTF8(*JsonPayload), timeoutMs);
	// FNatsMessage Reply;
	// bool bOk = (s == NATS_OK);
	// if (bOk)
	// {
	//     Reply.Subject = UTF8_TO_TCHAR(natsMsg_GetSubject(replyMsg));
	//     const char* data = natsMsg_GetData(replyMsg);
	//     int dataLen = natsMsg_GetDataLength(replyMsg);
	//     Reply.Payload.Append((const uint8*)data, dataLen);
	//     natsMsg_Destroy(replyMsg);
	// }
	// AsyncTask(ENamedThreads::GameThread, [Callback, bOk, Reply]()
	// {
	//     Callback.ExecuteIfBound(bOk, Reply);
	// });
	UE_LOG(LogNatsClient, Verbose, TEXT("RequestJson [%s] (stub)"), *Subject);
#endif
}

// ─── Subscribe ────────────────────────────────────────────────────────────────

FNatsSubscriptionHandle UNatsClientSubsystem::Subscribe(const FString& Subject, const FOnNatsMessage& Delegate)
{
	FNatsSubscriptionHandle Handle;

#if NATS_CLIENT_NOT_AVAILABLE
	UE_LOG(LogNatsClient, Warning, TEXT("Subscribe: nats.c not available. Subject=%s"), *Subject);
	return Handle;
#else
	Handle.Id = NextSubscriptionId++;
	FSubscriptionEntry Entry;
	Entry.Subject = Subject;
	Entry.Delegate = Delegate;
	// TODO: implement with nats.c
	// natsSubscription* sub = nullptr;
	// natsConnection_Subscribe(&sub, (natsConnection*)NatsConnection,
	//     TCHAR_TO_UTF8(*Subject), &UNatsClientSubsystem::NatsMsgHandler, this);
	// Entry.NativeSubscription = sub;
	Subscriptions.Add(Handle.Id, MoveTemp(Entry));
	UE_LOG(LogNatsClient, Log, TEXT("Subscribe [%s] id=%lld (stub)"), *Subject, Handle.Id);
	return Handle;
#endif
}

void UNatsClientSubsystem::Unsubscribe(const FNatsSubscriptionHandle& Handle)
{
	if (!Handle.IsValid())
	{
		return;
	}

	if (FSubscriptionEntry* Entry = Subscriptions.Find(Handle.Id))
	{
#if !NATS_CLIENT_NOT_AVAILABLE
		// natsSubscription_Unsubscribe((natsSubscription*)Entry->NativeSubscription);
		// natsSubscription_Destroy((natsSubscription*)Entry->NativeSubscription);
#endif
		Subscriptions.Remove(Handle.Id);
	}
}

// ─── Internal ────────────────────────────────────────────────────────────────

void UNatsClientSubsystem::NatsMsgHandler(void* NatsConn, void* NatsSub, void* NatsMsg, void* UserData)
{
	// Called on nats.c internal thread — must marshal to game thread.
	// UNatsClientSubsystem* Self = static_cast<UNatsClientSubsystem*>(UserData);
	// const char* subj = natsMsg_GetSubject((natsMsg*)NatsMsg);
	// const char* data = natsMsg_GetData((natsMsg*)NatsMsg);
	// int dataLen = natsMsg_GetDataLength((natsMsg*)NatsMsg);
	// FNatsMessage Msg;
	// Msg.Subject = UTF8_TO_TCHAR(subj);
	// Msg.Payload.Append((const uint8*)data, dataLen);
	// natsMsg_Destroy((natsMsg*)NatsMsg);
	// AsyncTask(ENamedThreads::GameThread, [Self, Msg]()
	// {
	//     for (auto& Pair : Self->Subscriptions)
	//         if (Pair.Value.Subject == Msg.Subject || /* wildcard match */)
	//             Pair.Value.Delegate.Broadcast(Msg);
	// });
}
