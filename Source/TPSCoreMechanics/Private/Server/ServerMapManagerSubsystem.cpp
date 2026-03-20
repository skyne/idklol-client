// Fill out your copyright notice in the Description page of Project Settings.

#include "Server/ServerMapManagerSubsystem.h"
#include "NatsClientSubsystem.h"
#include "Config/TPSNatsSubjectsConfig.h"
#include "Helpers/JsonObjectUtils.h"
#include "Json.h"
#include "Misc/CommandLine.h"
#include "Misc/Guid.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogServerMapManager, Log, All);

bool UServerMapManagerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return IsRunningDedicatedServer();
}

void UServerMapManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Resolve instance ID: -InstanceId=<id> or generated GUID
	if (!FParse::Value(FCommandLine::Get(), TEXT("InstanceId="), InstanceId) || InstanceId.IsEmpty())
	{
		InstanceId = FGuid::NewGuid().ToString(EGuidFormats::Short);
		UE_LOG(LogServerMapManager, Warning, TEXT("No -InstanceId supplied, generated: %s"), *InstanceId);
	}
	UE_LOG(LogServerMapManager, Log, TEXT("ServerMapManagerSubsystem initialized — InstanceId=%s"), *InstanceId);

	// Subscribe once NATS is connected
	UNatsClientSubsystem* Nats = GetGameInstance()->GetSubsystem<UNatsClientSubsystem>();
	if (!Nats)
	{
		UE_LOG(LogServerMapManager, Error, TEXT("NatsClientSubsystem not found"));
		return;
	}

	// Bind UFUNCTION delegates
	const UTPSNatsSubjectsConfig& Subjects = UTPSNatsSubjectsConfig::Get();
	const FString MapSubject = Subjects.MakeServerMapSubject(InstanceId);
	const FString StatusSubject = Subjects.MakeServerStatusSubject(InstanceId);

	FOnNatsMessage MapDelegate;
	MapDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UServerMapManagerSubsystem, OnMapMessage));
	MapSubscriptionHandle = Nats->Subscribe(MapSubject, MapDelegate);

	FOnNatsMessage StatusDelegate;
	StatusDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UServerMapManagerSubsystem, OnStatusMessage));
	StatusSubscriptionHandle = Nats->Subscribe(StatusSubject, StatusDelegate);

	UE_LOG(LogServerMapManager, Log, TEXT("Subscribed to %s and %s"), *MapSubject, *StatusSubject);
}

void UServerMapManagerSubsystem::Deinitialize()
{
	if (UNatsClientSubsystem* Nats = GetGameInstance()->GetSubsystem<UNatsClientSubsystem>())
	{
		Nats->Unsubscribe(MapSubscriptionHandle);
		Nats->Unsubscribe(StatusSubscriptionHandle);
	}
	Super::Deinitialize();
}

void UServerMapManagerSubsystem::OnMapMessage(const FNatsMessage& Message)
{
	const FString Json = Message.PayloadAsString();
	UE_LOG(LogServerMapManager, Log, TEXT("Map message received: %s"), *Json);

	TSharedPtr<FJsonObject> JsonObj;
	if (!TPSCoreJson::DeserializeObject(Json, JsonObj))
	{
		UE_LOG(LogServerMapManager, Error, TEXT("Failed to parse map message JSON: %s"), *Json);
		return;
	}

	FString MapPath;
	if (!JsonObj->TryGetStringField(TEXT("map"), MapPath) || MapPath.IsEmpty())
	{
		UE_LOG(LogServerMapManager, Error, TEXT("Map message missing 'map' field"));
		return;
	}

	TravelToMap(MapPath);
}

void UServerMapManagerSubsystem::OnStatusMessage(const FNatsMessage& Message)
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	const FString CurrentMap = World ? World->GetMapName() : TEXT("unknown");
	const int32 PlayerCount  = World ? World->GetNumPlayerControllers() : 0;

	TSharedRef<FJsonObject> ResponseObject = MakeShared<FJsonObject>();
	ResponseObject->SetStringField(TEXT("instance"), InstanceId);
	ResponseObject->SetStringField(TEXT("map"), CurrentMap);
	ResponseObject->SetNumberField(TEXT("players"), PlayerCount);
	const FString Response = TPSCoreJson::SerializeObject(ResponseObject);

	// Reply on the NATS reply-to subject if present
	if (!Message.ReplyTo.IsEmpty())
	{
		if (UNatsClientSubsystem* Nats = GetGameInstance()->GetSubsystem<UNatsClientSubsystem>())
		{
			Nats->PublishJson(Message.ReplyTo, Response);
		}
	}

	UE_LOG(LogServerMapManager, Verbose, TEXT("Status reply: %s"), *Response);
}

void UServerMapManagerSubsystem::TravelToMap(const FString& MapPath)
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World)
	{
		UE_LOG(LogServerMapManager, Error, TEXT("TravelToMap: no world"));
		return;
	}

	UE_LOG(LogServerMapManager, Log, TEXT("ServerTravel → %s"), *MapPath);
	World->ServerTravel(MapPath, /*bAbsolute=*/true);
}
