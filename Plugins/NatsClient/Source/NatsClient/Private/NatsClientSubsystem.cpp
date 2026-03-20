// Fill out your copyright notice in the Description page of Project Settings.

#include "NatsClientSubsystem.h"
#include "Async/Async.h"
#include "Misc/Paths.h"

#ifndef NATS_CLIENT_NOT_AVAILABLE
#define NATS_CLIENT_NOT_AVAILABLE 1
#endif

#if !NATS_CLIENT_NOT_AVAILABLE
extern "C"
{
#include "nats/nats.h"
}

static FString NatsStatusToString(const natsStatus Status)
{
	const char* StatusText = natsStatus_GetText(Status);
	if (StatusText == nullptr)
	{
		return FString::Printf(TEXT("Unknown natsStatus (%d)"), static_cast<int32>(Status));
	}
	return UTF8_TO_TCHAR(StatusText);
}
#endif

DEFINE_LOG_CATEGORY_STATIC(LogNatsClient, Log, All);

// ─── USubsystem ──────────────────────────────────────────────────────────────

void UNatsClientSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogNatsClient, Log, TEXT("NatsClientSubsystem initialized"));

	// Auto-connect on dedicated server (or editor, for dev console commands).
	// Client processes must call Connect() explicitly.
	const bool bShouldAutoConnect = IsRunningDedicatedServer()
#if WITH_EDITOR
		|| GIsEditor
#endif
		;

	if (!bShouldAutoConnect)
	{
		return;
	}

	// 1. Prefer -NATSUrl= on the command line (set by Docker / launch script).
	FString Url;
	if (!FParse::Value(FCommandLine::Get(), TEXT("NATSUrl="), Url) || Url.IsEmpty())
	{
		// 2. Fall back to [NatsClient] DefaultUrl in the game ini.
		GConfig->GetString(TEXT("NatsClient"), TEXT("DefaultUrl"), Url, GGameIni);
	}

	if (Url.IsEmpty())
	{
		UE_LOG(LogNatsClient, Warning, TEXT("Auto-connect skipped: no NATS URL (set -NATSUrl= or [NatsClient] DefaultUrl in ini)"));
		return;
	}

	FString Credentials;
	GConfig->GetString(TEXT("NatsClient"), TEXT("CredentialsOrNKey"), Credentials, GGameIni);

	UE_LOG(LogNatsClient, Log, TEXT("Auto-connecting to NATS: %s"), *Url);
	Connect(Url, Credentials);
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
   if (Url.IsEmpty())
   {
      UE_LOG(LogNatsClient, Warning, TEXT("Connect failed: URL is empty"));
      return;
   }

   Disconnect();

   natsOptions* Options = nullptr;
   natsStatus Status = natsOptions_Create(&Options);
   if (Status != NATS_OK || Options == nullptr)
   {
      UE_LOG(LogNatsClient, Error, TEXT("natsOptions_Create failed: %s"), *NatsStatusToString(Status));
      return;
   }

   FTCHARToUTF8 UrlUtf8(*Url);
   Status = natsOptions_SetURL(Options, UrlUtf8.Get());
   if (Status != NATS_OK)
   {
      UE_LOG(LogNatsClient, Error, TEXT("natsOptions_SetURL failed: %s"), *NatsStatusToString(Status));
      natsOptions_Destroy(Options);
      return;
   }

   if (!CredentialsOrNKey.IsEmpty())
   {
      FString CredentialsPath = CredentialsOrNKey;
      if (!FPaths::FileExists(CredentialsPath))
      {
         CredentialsPath = FPaths::ConvertRelativePathToFull(CredentialsOrNKey);
      }

      if (!FPaths::FileExists(CredentialsPath))
      {
         UE_LOG(
            LogNatsClient,
            Error,
            TEXT("CredentialsOrNKey must be a valid credentials file path for now. Inline NKey seed is not yet supported: %s"),
            *CredentialsOrNKey);
         natsOptions_Destroy(Options);
         return;
      }

      FTCHARToUTF8 CredentialsUtf8(*CredentialsPath);
      Status = natsOptions_SetUserCredentialsFromFiles(Options, CredentialsUtf8.Get(), nullptr);
      if (Status != NATS_OK)
      {
         UE_LOG(LogNatsClient, Error, TEXT("natsOptions_SetUserCredentialsFromFiles failed: %s"), *NatsStatusToString(Status));
         natsOptions_Destroy(Options);
         return;
      }
   }

   natsConnection* NewConnection = nullptr;
   Status = natsConnection_Connect(&NewConnection, Options);
   natsOptions_Destroy(Options);

   if (Status != NATS_OK || NewConnection == nullptr)
   {
      UE_LOG(LogNatsClient, Error, TEXT("natsConnection_Connect failed: %s"), *NatsStatusToString(Status));
      return;
   }

   NatsConnection = NewConnection;
   bConnected = true;
   AsyncTask(ENamedThreads::GameThread, [this]() { OnConnected.Broadcast(); });
#endif
}

void UNatsClientSubsystem::Disconnect()
{
#if !NATS_CLIENT_NOT_AVAILABLE
	if (NatsConnection)
	{
      for (auto& Pair : Subscriptions)
      {
         if (Pair.Value.NativeSubscription != nullptr)
         {
            natsSubscription_Unsubscribe(static_cast<natsSubscription*>(Pair.Value.NativeSubscription));
            natsSubscription_Destroy(static_cast<natsSubscription*>(Pair.Value.NativeSubscription));
            Pair.Value.NativeSubscription = nullptr;
         }
      }

      natsConnection_Close(static_cast<natsConnection*>(NatsConnection));
      natsConnection_Destroy(static_cast<natsConnection*>(NatsConnection));
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
   if (NatsConnection == nullptr)
   {
      UE_LOG(LogNatsClient, Warning, TEXT("PublishJson failed: not connected. Subject=%s"), *Subject);
      return;
   }

   FTCHARToUTF8 SubjectUtf8(*Subject);
   FTCHARToUTF8 PayloadUtf8(*JsonPayload);
   natsStatus Status = natsConnection_PublishString(
      static_cast<natsConnection*>(NatsConnection),
      SubjectUtf8.Get(),
      PayloadUtf8.Get());

   if (Status != NATS_OK)
   {
      UE_LOG(LogNatsClient, Error, TEXT("PublishJson failed for [%s]: %s"), *Subject, *NatsStatusToString(Status));
   }
#endif
}

void UNatsClientSubsystem::PublishBytes(const FString& Subject, const TArray<uint8>& Payload)
{
#if NATS_CLIENT_NOT_AVAILABLE
	UE_LOG(LogNatsClient, Warning, TEXT("PublishBytes: nats.c not available. Subject=%s"), *Subject);
	return;
#else
   if (NatsConnection == nullptr)
   {
      UE_LOG(LogNatsClient, Warning, TEXT("PublishBytes failed: not connected. Subject=%s"), *Subject);
      return;
   }

   FTCHARToUTF8 SubjectUtf8(*Subject);
   const void* DataPtr = Payload.Num() > 0 ? static_cast<const void*>(Payload.GetData()) : nullptr;
   natsStatus Status = natsConnection_Publish(
      static_cast<natsConnection*>(NatsConnection),
      SubjectUtf8.Get(),
      DataPtr,
      Payload.Num());

   if (Status != NATS_OK)
   {
      UE_LOG(LogNatsClient, Error, TEXT("PublishBytes failed for [%s]: %s"), *Subject, *NatsStatusToString(Status));
   }
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
   if (NatsConnection == nullptr)
   {
      UE_LOG(LogNatsClient, Warning, TEXT("RequestJson failed: not connected. Subject=%s"), *Subject);
      FNatsMessage EmptyReply;
      Callback.ExecuteIfBound(false, EmptyReply);
      return;
   }

   const int64 TimeoutMs = FMath::Max<int64>(1, static_cast<int64>(TimeoutSeconds * 1000.0f));
   FTCHARToUTF8 SubjectUtf8(*Subject);
   FTCHARToUTF8 PayloadUtf8(*JsonPayload);

   natsMsg* ReplyMsg = nullptr;
   const natsStatus Status = natsConnection_RequestString(
      &ReplyMsg,
      static_cast<natsConnection*>(NatsConnection),
      SubjectUtf8.Get(),
      PayloadUtf8.Get(),
      TimeoutMs);

   FNatsMessage Reply;
   const bool bOk = (Status == NATS_OK && ReplyMsg != nullptr);
   if (bOk)
   {
      const char* ReplySubject = natsMsg_GetSubject(ReplyMsg);
      const char* ReplyTo = natsMsg_GetReply(ReplyMsg);
      const char* ReplyData = natsMsg_GetData(ReplyMsg);
      const int32 ReplyDataLen = natsMsg_GetDataLength(ReplyMsg);

      if (ReplySubject != nullptr)
      {
         Reply.Subject = UTF8_TO_TCHAR(ReplySubject);
      }
      if (ReplyTo != nullptr)
      {
         Reply.ReplyTo = UTF8_TO_TCHAR(ReplyTo);
      }
      if (ReplyData != nullptr && ReplyDataLen > 0)
      {
         Reply.Payload.Append(reinterpret_cast<const uint8*>(ReplyData), ReplyDataLen);
      }
   }
   else
   {
      UE_LOG(LogNatsClient, Warning, TEXT("RequestJson failed for [%s]: %s"), *Subject, *NatsStatusToString(Status));
   }

   if (ReplyMsg != nullptr)
   {
      natsMsg_Destroy(ReplyMsg);
   }

   AsyncTask(ENamedThreads::GameThread, [Callback, bOk, Reply]() mutable
   {
      Callback.ExecuteIfBound(bOk, Reply);
   });
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
   if (NatsConnection == nullptr)
   {
      UE_LOG(LogNatsClient, Warning, TEXT("Subscribe failed: not connected. Subject=%s"), *Subject);
      return Handle;
   }

   natsSubscription* NativeSubscription = nullptr;
   FTCHARToUTF8 SubjectUtf8(*Subject);

   auto NativeHandler = +[](natsConnection* InConn, natsSubscription* InSub, natsMsg* InMsg, void* InUserData)
   {
      UNatsClientSubsystem::NatsMsgHandler(InConn, InSub, InMsg, InUserData);
   };

   const natsStatus Status = natsConnection_Subscribe(
      &NativeSubscription,
      static_cast<natsConnection*>(NatsConnection),
      SubjectUtf8.Get(),
      NativeHandler,
      this);

   if (Status != NATS_OK || NativeSubscription == nullptr)
   {
      UE_LOG(LogNatsClient, Error, TEXT("Subscribe failed for [%s]: %s"), *Subject, *NatsStatusToString(Status));
      return Handle;
   }

   Handle.Id = NextSubscriptionId++;
   FSubscriptionEntry Entry;
   Entry.Subject = Subject;
   Entry.Delegate = Delegate;
   Entry.NativeSubscription = NativeSubscription;
   Subscriptions.Add(Handle.Id, MoveTemp(Entry));

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
		if (Entry->NativeSubscription != nullptr)
		{
		 natsSubscription_Unsubscribe(static_cast<natsSubscription*>(Entry->NativeSubscription));
		 natsSubscription_Destroy(static_cast<natsSubscription*>(Entry->NativeSubscription));
		 Entry->NativeSubscription = nullptr;
		}
#endif
		Subscriptions.Remove(Handle.Id);
	}
}

// ─── Internal ────────────────────────────────────────────────────────────────

void UNatsClientSubsystem::NatsMsgHandler(void* NatsConn, void* NatsSub, void* NatsMsg, void* UserData)
{
   (void)NatsConn;

   if (UserData == nullptr || NatsMsg == nullptr)
   {
      return;
   }

   UNatsClientSubsystem* Self = static_cast<UNatsClientSubsystem*>(UserData);
   natsMsg* NativeMsg = static_cast<natsMsg*>(NatsMsg);

   FNatsMessage Message;
   const char* Subject = natsMsg_GetSubject(NativeMsg);
   const char* ReplyTo = natsMsg_GetReply(NativeMsg);
   const char* Data = natsMsg_GetData(NativeMsg);
   const int32 DataLen = natsMsg_GetDataLength(NativeMsg);

   if (Subject != nullptr)
   {
      Message.Subject = UTF8_TO_TCHAR(Subject);
   }
   if (ReplyTo != nullptr)
   {
      Message.ReplyTo = UTF8_TO_TCHAR(ReplyTo);
   }
   if (Data != nullptr && DataLen > 0)
   {
      Message.Payload.Append(reinterpret_cast<const uint8*>(Data), DataLen);
   }

   natsMsg_Destroy(NativeMsg);

   void* NativeSub = NatsSub;
   AsyncTask(ENamedThreads::GameThread, [Self, NativeSub, Message]() mutable
   {
      if (!IsValid(Self))
      {
         return;
      }

      for (const TPair<int64, FSubscriptionEntry>& Pair : Self->Subscriptions)
      {
         if (Pair.Value.NativeSubscription == NativeSub)
         {
            Pair.Value.Delegate.ExecuteIfBound(Message);
            break;
         }
      }
   });
}
