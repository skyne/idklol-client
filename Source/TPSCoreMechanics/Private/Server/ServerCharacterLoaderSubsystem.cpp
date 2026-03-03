// Fill out your copyright notice in the Description page of Project Settings.

#include "Server/ServerCharacterLoaderSubsystem.h"
#include "NatsClientSubsystem.h"
#include "Json.h"

DEFINE_LOG_CATEGORY_STATIC(LogServerCharacterLoader, Log, All);

const TCHAR* UServerCharacterLoaderSubsystem::CharactersGetSubject = TEXT("characters.get");

bool UServerCharacterLoaderSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// Only instantiate on dedicated server processes
	return IsRunningDedicatedServer();
}

void UServerCharacterLoaderSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogServerCharacterLoader, Log, TEXT("ServerCharacterLoaderSubsystem initialized (server)"));
}

void UServerCharacterLoaderSubsystem::FetchCharacter(const FString& CharacterId, FOnCharacterLoaded Callback)
{
	UNatsClientSubsystem* Nats = GetGameInstance()->GetSubsystem<UNatsClientSubsystem>();
	if (!Nats || !Nats->IsConnected())
	{
		UE_LOG(LogServerCharacterLoader, Warning, TEXT("FetchCharacter: NATS not connected, cannot fetch character %s"), *CharacterId);
		Callback.ExecuteIfBound(false, FCharacterData{});
		return;
	}

	const FString Payload = FString::Printf(TEXT("{\"id\":\"%s\"}"), *CharacterId);
	UE_LOG(LogServerCharacterLoader, Log, TEXT("Fetching character %s via NATS"), *CharacterId);

	FOnNatsReply ReplyDelegate;
	ReplyDelegate.BindLambda([Callback, CharacterId](bool bSuccess, const FNatsMessage& Reply)
	{
		if (!bSuccess)
		{
			UE_LOG(LogServerCharacterLoader, Warning, TEXT("FetchCharacter timed out for %s"), *CharacterId);
			Callback.ExecuteIfBound(false, FCharacterData{});
			return;
		}

		// Parse JSON response into FCharacterData
		const FString Json = Reply.PayloadAsString();
		TSharedPtr<FJsonObject> JsonObj;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);

		if (!FJsonSerializer::Deserialize(Reader, JsonObj) || !JsonObj.IsValid())
		{
			UE_LOG(LogServerCharacterLoader, Error, TEXT("FetchCharacter: failed to parse JSON for %s: %s"), *CharacterId, *Json);
			Callback.ExecuteIfBound(false, FCharacterData{});
			return;
		}

		FCharacterData Data;
		Data.CharacterId  = JsonObj->GetStringField(TEXT("id"));
		Data.Name         = JsonObj->GetStringField(TEXT("name"));
		Data.Race         = static_cast<ECharacterRace>((uint8)JsonObj->GetIntegerField(TEXT("race")));
		Data.Gender       = static_cast<ECharacterGender>((uint8)JsonObj->GetIntegerField(TEXT("gender")));
		Data.SkinColor    = static_cast<ECharacterSkinColor>((uint8)JsonObj->GetIntegerField(TEXT("skin_color")));
		Data.CharacterClass = static_cast<ECharacterClass>((uint8)JsonObj->GetIntegerField(TEXT("character_class")));
		Data.CreatedAt    = JsonObj->GetStringField(TEXT("created_at"));

		UE_LOG(LogServerCharacterLoader, Log, TEXT("Character loaded: %s (%s)"), *Data.Name, *Data.CharacterId);
		Callback.ExecuteIfBound(true, Data);
	});

	Nats->RequestJson(CharactersGetSubject, Payload, DefaultTimeoutSeconds, ReplyDelegate);
}
