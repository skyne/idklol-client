// Fill out your copyright notice in the Description page of Project Settings.

#include "Server/ServerCharacterLoaderSubsystem.h"
#include "NatsClientSubsystem.h"
#include "Config/TPSNatsSubjectsConfig.h"
#include "Helpers/JsonObjectUtils.h"
#include "Json.h"
#include "Misc/ConfigCacheIni.h"
#include "TPSCoreMechanics/TPSCoreMechanics.h"

namespace
{
	float GetCharacterLoaderNatsRequestTimeoutSeconds()
	{
		float TimeoutSeconds = 60.0f;
		GConfig->GetFloat(TEXT("NatsClient"), TEXT("RequestTimeoutSeconds"), TimeoutSeconds, GGameIni);
		return FMath::Max(1.0f, TimeoutSeconds);
	}
}

bool UServerCharacterLoaderSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// Only instantiate on dedicated server processes
	return IsRunningDedicatedServer();
}

void UServerCharacterLoaderSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LOG("[ServerCharacterLoaderSubsystem] initialized (server)");
}

void UServerCharacterLoaderSubsystem::FetchCharacter(const FString& CharacterId, FOnCharacterLoaded Callback)
{
	UNatsClientSubsystem* Nats = GetGameInstance()->GetSubsystem<UNatsClientSubsystem>();
	if (!Nats || !Nats->IsConnected())
	{
		LOG_WARNING("[ServerCharacterLoaderSubsystem] FetchCharacter NATS not connected for %s", *CharacterId);
		Callback.ExecuteIfBound(false, FCharacterData{});
		return;
	}

	const FString Payload = FString::Printf(TEXT("{\"id\":\"%s\"}"), *CharacterId);
	LOG("[ServerCharacterLoaderSubsystem] Fetching character %s via NATS", *CharacterId);

	FOnNatsReply ReplyDelegate;
	ReplyDelegate.BindLambda([Callback, CharacterId](bool bSuccess, const FNatsMessage& Reply)
	{
		if (!bSuccess)
		{
			LOG_WARNING("[ServerCharacterLoaderSubsystem] FetchCharacter timed out for %s", *CharacterId);
			Callback.ExecuteIfBound(false, FCharacterData{});
			return;
		}

		// Parse JSON response into FCharacterData
		const FString Json = Reply.PayloadAsString();
		TSharedPtr<FJsonObject> JsonObj;

		if (!TPSCoreJson::DeserializeObject(Json, JsonObj))
		{
			LOG_ERROR("[ServerCharacterLoaderSubsystem] FetchCharacter failed to parse JSON for %s: %s", *CharacterId, *Json);
			Callback.ExecuteIfBound(false, FCharacterData{});
			return;
		}

		FCharacterData Data;
		Data.CharacterId = JsonObj->GetStringField(TEXT("id"));
		Data.Name = JsonObj->GetStringField(TEXT("name"));
		Data.Race = static_cast<ECharacterRace>((uint8)JsonObj->GetIntegerField(TEXT("race")));
		Data.Gender = static_cast<ECharacterGender>((uint8)JsonObj->GetIntegerField(TEXT("gender")));
		Data.SkinColor = static_cast<ECharacterSkinColor>((uint8)JsonObj->GetIntegerField(TEXT("skin_color")));
		Data.CharacterClass = static_cast<ECharacterClass>((uint8)JsonObj->GetIntegerField(TEXT("character_class")));
		Data.CreatedAt = JsonObj->GetStringField(TEXT("created_at"));

		// user_email was added in a later server build; graceful fallback if absent.
		FString OwnerEmail;
		if (JsonObj->TryGetStringField(TEXT("user_email"), OwnerEmail))
		{
			Data.OwnerEmail = OwnerEmail;
		}

		LOG("[ServerCharacterLoaderSubsystem] Character loaded: %s (%s)", *Data.Name, *Data.CharacterId);
		Callback.ExecuteIfBound(true, Data);
	});

	Nats->RequestJson(UTPSNatsSubjectsConfig::Get().CharactersGetSubject, Payload, GetCharacterLoaderNatsRequestTimeoutSeconds(), ReplyDelegate);
}
