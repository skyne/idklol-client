// Fill out your copyright notice in the Description page of Project Settings.

#include "Server/ServerNpcManagerSubsystem.h"
#include "NPC/NPCCharacter.h"
#include "Game/GameMode/TPSCoreGameMode.h"
#include "NatsClientSubsystem.h"
#include "Json.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogServerNpcManager, Log, All);

// ─── Lifecycle ────────────────────────────────────────────────────────────────

bool UServerNpcManagerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return IsRunningDedicatedServer();
}

void UServerNpcManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &UServerNpcManagerSubsystem::OnPostLoadMap);

	UE_LOG(LogServerNpcManager, Log, TEXT("ServerNpcManagerSubsystem initialized"));
}

void UServerNpcManagerSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	SpawnedNpcs.Empty();
	Super::Deinitialize();
}

// ─── World load ───────────────────────────────────────────────────────────────

void UServerNpcManagerSubsystem::OnPostLoadMap(UWorld* World)
{
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	const FString ZoneId = ResolveZoneId(World);
	if (ZoneId.IsEmpty())
	{
		UE_LOG(LogServerNpcManager, Warning, TEXT("OnPostLoadMap: could not resolve ZoneId for world '%s', skipping NPC load"),
			*World->GetMapName());
		return;
	}

	UE_LOG(LogServerNpcManager, Log, TEXT("OnPostLoadMap: loading NPCs for zone '%s'"), *ZoneId);
	LoadNpcsForZone(World, ZoneId);
}

// ─── Zone id resolution ───────────────────────────────────────────────────────

FString UServerNpcManagerSubsystem::ResolveZoneId(UWorld* World)
{
	if (!World)
	{
		return FString();
	}

	// Prefer the explicitly authored ZoneId on ATPSCoreGameMode.
	if (AGameModeBase* GM = World->GetAuthGameMode())
	{
		if (ATPSCoreGameMode* TpsGM = Cast<ATPSCoreGameMode>(GM))
		{
			const FString& ConfigZone = TpsGM->GetZoneId();
			if (!ConfigZone.IsEmpty())
			{
				return ConfigZone;
			}
		}
	}

	// Fall back to map leaf name (strips package path separators).
	FString MapName = World->GetMapName();
	MapName.RemoveFromStart(World->StreamingLevelsPrefix);
	return MapName;
}

// ─── NPC loading ──────────────────────────────────────────────────────────────

void UServerNpcManagerSubsystem::LoadNpcsForZone(UWorld* World, const FString& ZoneId)
{
	UNatsClientSubsystem* Nats = GetGameInstance()->GetSubsystem<UNatsClientSubsystem>();
	if (!Nats || !Nats->IsConnected())
	{
		UE_LOG(LogServerNpcManager, Warning,
			TEXT("LoadNpcsForZone: NATS not connected, cannot load NPCs for zone '%s'"), *ZoneId);
		return;
	}

	TWeakObjectPtr<UWorld> WeakWorld(World);
	const FString Payload = FString::Printf(TEXT("{\"zone_id\":\"%s\"}"), *ZoneId);

	FOnNatsReply Reply;
	Reply.BindLambda([this, WeakWorld, ZoneId](bool bSuccess, const FNatsMessage& Msg)
	{
		UWorld* W = WeakWorld.Get();
		if (!W)
		{
			UE_LOG(LogServerNpcManager, Warning, TEXT("LoadNpcsForZone '%s': world gone before reply"), *ZoneId);
			return;
		}

		if (!bSuccess)
		{
			UE_LOG(LogServerNpcManager, Warning, TEXT("LoadNpcsForZone '%s': NATS request timed out"), *ZoneId);
			return;
		}

		const FString Json = Msg.PayloadAsString();
		TSharedPtr<FJsonObject> Root;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
		if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		{
			UE_LOG(LogServerNpcManager, Error, TEXT("LoadNpcsForZone '%s': failed to parse JSON response"), *ZoneId);
			return;
		}

		const TArray<TSharedPtr<FJsonValue>>* NpcsArray;
		if (!Root->TryGetArrayField(TEXT("npcs"), NpcsArray))
		{
			UE_LOG(LogServerNpcManager, Warning, TEXT("LoadNpcsForZone '%s': response has no 'npcs' array"), *ZoneId);
			return;
		}

		UE_LOG(LogServerNpcManager, Log, TEXT("LoadNpcsForZone '%s': received %d NPC definitions"), *ZoneId, NpcsArray->Num());

		TArray<TWeakObjectPtr<ANPCCharacter>>& WorldNpcs = SpawnedNpcs.FindOrAdd(WeakWorld);

		for (const TSharedPtr<FJsonValue>& Entry : *NpcsArray)
		{
			const TSharedPtr<FJsonObject>* NpcObj;
			if (!Entry->TryGetObject(NpcObj))
			{
				continue;
			}

			FNpcMeta Meta = ParseNpcMeta(*NpcObj);
			if (Meta.NpcId.IsEmpty())
			{
				continue;
			}

			if (Meta.SpawnPoints.IsEmpty())
			{
				UE_LOG(LogServerNpcManager, Warning,
					TEXT("LoadNpcsForZone: NPC '%s' has no spawn points, skipping"), *Meta.DisplayName);
				continue;
			}

			// Resolve NPC class: ATPSCoreGameMode may specify a Blueprint subclass.
			TSubclassOf<ANPCCharacter> NpcClass = ANPCCharacter::StaticClass();
			if (AGameModeBase* GM = W->GetAuthGameMode())
			{
				if (ATPSCoreGameMode* TpsGM = Cast<ATPSCoreGameMode>(GM))
				{
					TSubclassOf<ANPCCharacter> Override = TpsGM->GetNpcClass();
					if (Override)
					{
						NpcClass = Override;
					}
				}
			}

			// Spawn one actor per spawn point.
			for (const FNpcSpawnPoint& SP : Meta.SpawnPoints)
			{
				const FVector Location(SP.X, SP.Y, SP.Z);
				const FRotator Rotation(0.f, SP.Yaw, 0.f);
				const FTransform SpawnTransform(Rotation, Location);

				FActorSpawnParameters Params;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

				ANPCCharacter* NPC = W->SpawnActor<ANPCCharacter>(NpcClass, SpawnTransform, Params);
				if (!NPC)
				{
					UE_LOG(LogServerNpcManager, Error,
						TEXT("LoadNpcsForZone: failed to spawn NPC '%s' at [%.1f, %.1f, %.1f]"),
						*Meta.DisplayName, SP.X, SP.Y, SP.Z);
					continue;
				}

				NPC->InitializeFromNpcMeta(Meta, SP);
				WorldNpcs.Add(NPC);

				UE_LOG(LogServerNpcManager, Log,
					TEXT("Spawned NPC '%s' (%s) at [%.1f, %.1f, %.1f]"),
					*Meta.DisplayName, *Meta.NpcId, SP.X, SP.Y, SP.Z);
			}
		}

		UE_LOG(LogServerNpcManager, Log, TEXT("LoadNpcsForZone '%s': spawned %d NPC actors"), *ZoneId, WorldNpcs.Num());
	});

	Nats->RequestJson(TEXT("npc.meta.by_zone"), Payload, NatsTimeoutSeconds, Reply);
}

// ─── JSON parsers ─────────────────────────────────────────────────────────────

FNpcMeta UServerNpcManagerSubsystem::ParseNpcMeta(const TSharedPtr<FJsonObject>& Obj)
{
	FNpcMeta Meta;
	if (!Obj.IsValid())
	{
		return Meta;
	}

	Obj->TryGetStringField(TEXT("npc_id"),      Meta.NpcId);
	Obj->TryGetStringField(TEXT("display_name"), Meta.DisplayName);
	Obj->TryGetStringField(TEXT("role"),         Meta.Role);
	Obj->TryGetStringField(TEXT("model_id"),     Meta.ModelId);
	Obj->TryGetStringField(TEXT("faction"),      Meta.Faction);
	Obj->TryGetStringField(TEXT("template_key"), Meta.TemplateKey);
	Obj->TryGetStringField(TEXT("tone"),         Meta.Tone);
	Obj->TryGetBoolField(TEXT("is_persistent"),  Meta.bIsPersistent);

	const TArray<TSharedPtr<FJsonValue>>* RulesArr;
	if (Obj->TryGetArrayField(TEXT("rules"), RulesArr))
	{
		for (const TSharedPtr<FJsonValue>& R : *RulesArr)
		{
			FString Rule;
			if (R->TryGetString(Rule))
			{
				Meta.Rules.Add(Rule);
			}
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* SpawnArr;
	if (Obj->TryGetArrayField(TEXT("spawn_points"), SpawnArr))
	{
		for (const TSharedPtr<FJsonValue>& S : *SpawnArr)
		{
			const TSharedPtr<FJsonObject>* SpObj;
			if (S->TryGetObject(SpObj))
			{
				Meta.SpawnPoints.Add(ParseSpawnPoint(*SpObj));
			}
		}
	}

	const TSharedPtr<FJsonObject>* BcObj;
	if (Obj->TryGetObjectField(TEXT("behavior_config"), BcObj))
	{
		Meta.BehaviorConfig = ParseBehaviorConfig(*BcObj);
	}

	return Meta;
}

FNpcSpawnPoint UServerNpcManagerSubsystem::ParseSpawnPoint(const TSharedPtr<FJsonObject>& Obj)
{
	FNpcSpawnPoint SP;
	if (!Obj.IsValid())
	{
		return SP;
	}

	Obj->TryGetStringField(TEXT("id"),           SP.Id);
	Obj->TryGetStringField(TEXT("zone_id"),       SP.ZoneId);
	Obj->TryGetStringField(TEXT("spawn_policy"),  SP.SpawnPolicy);

	// JSON stores coordinates as numbers; TryGetNumberField handles int/float/double.
	double Tmp = 0.0;
	if (Obj->TryGetNumberField(TEXT("x"),   Tmp)) { SP.X   = static_cast<float>(Tmp); }
	if (Obj->TryGetNumberField(TEXT("y"),   Tmp)) { SP.Y   = static_cast<float>(Tmp); }
	if (Obj->TryGetNumberField(TEXT("z"),   Tmp)) { SP.Z   = static_cast<float>(Tmp); }
	if (Obj->TryGetNumberField(TEXT("yaw"), Tmp)) { SP.Yaw = static_cast<float>(Tmp); }

	return SP;
}

FNpcBehaviorConfig UServerNpcManagerSubsystem::ParseBehaviorConfig(const TSharedPtr<FJsonObject>& Obj)
{
	FNpcBehaviorConfig BC;
	if (!Obj.IsValid())
	{
		return BC;
	}

	double Tmp = 0.0;
	if (Obj->TryGetNumberField(TEXT("interaction_radius"), Tmp))
	{
		BC.InteractionRadius = static_cast<float>(Tmp);
	}

	if (Obj->TryGetNumberField(TEXT("cooldown_ms"), Tmp))
	{
		BC.CooldownMs = static_cast<int32>(Tmp);
	}
	if (Obj->TryGetNumberField(TEXT("max_concurrent_interactions"), Tmp))
	{
		BC.MaxConcurrentInteractions = static_cast<int32>(Tmp);
	}

	return BC;
}
