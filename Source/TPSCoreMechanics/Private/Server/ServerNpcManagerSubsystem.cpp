// Fill out your copyright notice in the Description page of Project Settings.

#include "Server/ServerNpcManagerSubsystem.h"
#include "NPC/NPCCharacter.h"
#include "Game/GameMode/TPSCoreGameMode.h"
#include "NatsClientSubsystem.h"
#include "Config/TPSNatsSubjectsConfig.h"
#include "Helpers/JsonObjectUtils.h"
#include "TPSCoreMechanics/TPSCoreMechanicsCharacter.h"
#include "Json.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Guid.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogServerNpcManager, Log, All);

namespace
{
	constexpr TCHAR NpcActorClassFolder[] = TEXT("/Game/Characters/NPC");

	FString BuildNpcActorClassPathFromId(const FString& ActorClassId)
	{
		return FString::Printf(TEXT("%s/%s.%s_C"), NpcActorClassFolder, *ActorClassId, *ActorClassId);
	}

	FString ExtractAssetIdFromObjectPath(const FString& ObjectPath)
	{
		if (ObjectPath.IsEmpty())
		{
			return FString();
		}

		FString AssetName = ObjectPath;
		int32 DotIndex = INDEX_NONE;
		if (ObjectPath.FindLastChar(TEXT('.'), DotIndex))
		{
			AssetName = ObjectPath.Mid(DotIndex + 1);
		}
		else
		{
			int32 SlashIndex = INDEX_NONE;
			if (ObjectPath.FindLastChar(TEXT('/'), SlashIndex))
			{
				AssetName = ObjectPath.Mid(SlashIndex + 1);
			}
		}

		if (AssetName.EndsWith(TEXT("_C")))
		{
			AssetName.LeftChopInline(2, EAllowShrinking::No);
		}

		return AssetName;
	}

	TSubclassOf<ANPCCharacter> ResolveNpcClass(UWorld* World, const FNpcMeta& Meta)
	{
		if (!Meta.ActorClassId.IsEmpty())
		{
			const FString ResolvedPath = BuildNpcActorClassPathFromId(Meta.ActorClassId);
			if (UClass* LoadedClass = LoadClass<ANPCCharacter>(nullptr, *ResolvedPath))
			{
				return LoadedClass;
			}

			UE_LOG(LogServerNpcManager, Warning,
				TEXT("ResolveNpcClass: failed to load actor class id '%s' (resolved '%s') for NPC '%s'; falling back"),
				*Meta.ActorClassId,
				*ResolvedPath,
				*Meta.NpcId);
		}

		if (World)
		{
			if (AGameModeBase* GM = World->GetAuthGameMode())
			{
				if (ATPSCoreGameMode* TpsGM = Cast<ATPSCoreGameMode>(GM))
				{
					if (TSubclassOf<ANPCCharacter> Override = TpsGM->GetNpcClass())
					{
						return Override;
					}
				}
			}
		}

		return ANPCCharacter::StaticClass();
	}

#if WITH_EDITOR
	UServerNpcManagerSubsystem* ResolveNpcManager(UWorld* World)
	{
		if (!World)
		{
			UE_LOG(LogServerNpcManager, Warning, TEXT("Editor NPC command: no world context"));
			return nullptr;
		}

		UGameInstance* GameInstance = World->GetGameInstance();
		if (!GameInstance)
		{
			UE_LOG(LogServerNpcManager, Warning, TEXT("Editor NPC command: no GameInstance for world '%s'"), *World->GetMapName());
			return nullptr;
		}

		UServerNpcManagerSubsystem* Subsystem = GameInstance->GetSubsystem<UServerNpcManagerSubsystem>();
		if (!Subsystem)
		{
			UE_LOG(LogServerNpcManager, Warning,
				TEXT("Editor NPC command: ServerNpcManagerSubsystem unavailable; run in PIE/server world"));
		}

		return Subsystem;
	}

	void EditorNpcLookupCommand(const TArray<FString>& Args, UWorld* World)
	{
		UServerNpcManagerSubsystem* Subsystem = ResolveNpcManager(World);
		if (!Subsystem)
		{
			return;
		}

		const FString Filter = Args.Num() > 0 ? FString::Join(Args, TEXT(" ")) : FString();
		Subsystem->EditorLookupNpcs(Filter);
	}

	void EditorNpcSpawnCommand(const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogServerNpcManager, Warning,
				TEXT("Usage: idk.npc.spawn <npc_id>"));
			return;
		}

		UServerNpcManagerSubsystem* Subsystem = ResolveNpcManager(World);
		if (!Subsystem)
		{
			return;
		}

		Subsystem->EditorSpawnNpcById(Args[0]);
	}

	static FAutoConsoleCommandWithWorldAndArgs GEditorNpcLookupCommand(
		TEXT("idk.npc.lookup"),
		TEXT("Lookup NPC definitions from npc-metadata-service. Usage: idk.npc.lookup [filter]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&EditorNpcLookupCommand));

	static FAutoConsoleCommandWithWorldAndArgs GEditorNpcSpawnCommand(
		TEXT("idk.npc.spawn"),
		TEXT("Spawn an NPC by id near the first player in PIE/server. Usage: idk.npc.spawn <npc_id>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&EditorNpcSpawnCommand));
#endif
}

// ─── Lifecycle ────────────────────────────────────────────────────────────────

bool UServerNpcManagerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	#if WITH_EDITOR
	if (GIsEditor)
	{
		return true;
	}
	#endif

	return IsRunningDedicatedServer();
}

void UServerNpcManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &UServerNpcManagerSubsystem::OnPostLoadMap);
	EnsureSpawnRequestSubscription();

	UE_LOG(LogServerNpcManager, Log, TEXT("ServerNpcManagerSubsystem initialized"));
}

void UServerNpcManagerSubsystem::Deinitialize()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UNatsClientSubsystem* Nats = GameInstance->GetSubsystem<UNatsClientSubsystem>())
		{
			if (SpawnRequestSubscription.IsValid())
			{
				Nats->Unsubscribe(SpawnRequestSubscription);
				SpawnRequestSubscription = FNatsSubscriptionHandle();
			}

			if (PlayerContextRequestSubscription.IsValid())
			{
				Nats->Unsubscribe(PlayerContextRequestSubscription);
				PlayerContextRequestSubscription = FNatsSubscriptionHandle();
			}
		}
	}

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

	if (!IsRunningDedicatedServer())
	{
		return;
	}

	EnsureSpawnRequestSubscription();

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

	EnsureSpawnRequestSubscription();

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
		if (!TPSCoreJson::DeserializeObject(Json, Root))
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

			// Spawn one actor per spawn point.
			for (const FNpcSpawnPoint& SP : Meta.SpawnPoints)
			{
				SpawnNpcFromMeta(W, Meta, SP);
			}
		}

		TArray<TWeakObjectPtr<ANPCCharacter>>& WorldNpcs = SpawnedNpcs.FindOrAdd(WeakWorld);
		UE_LOG(LogServerNpcManager, Log, TEXT("LoadNpcsForZone '%s': spawned %d NPC actors"), *ZoneId, WorldNpcs.Num());
	});

	Nats->RequestJson(UTPSNatsSubjectsConfig::Get().NpcMetaByZoneSubject, Payload, NatsTimeoutSeconds, Reply);
}

#if WITH_EDITOR
void UServerNpcManagerSubsystem::EditorLookupNpcs(const FString& Filter)
{
	UNatsClientSubsystem* Nats = GetGameInstance() ? GetGameInstance()->GetSubsystem<UNatsClientSubsystem>() : nullptr;
	if (!Nats || !Nats->IsConnected())
	{
		UE_LOG(LogServerNpcManager, Warning, TEXT("EditorLookupNpcs: NATS not connected"));
		return;
	}

	UE_LOG(LogServerNpcManager, Log,
		TEXT("EditorLookupNpcs: requesting npc.meta.list (filter='%s')"),
		*Filter);

	FOnNatsReply Reply;
	Reply.BindLambda([this, Filter](bool bSuccess, const FNatsMessage& Msg)
	{
		if (!bSuccess)
		{
			UE_LOG(LogServerNpcManager, Warning, TEXT("EditorLookupNpcs: request timed out"));
			return;
		}

		TSharedPtr<FJsonObject> Root;
		if (!TPSCoreJson::DeserializeObject(Msg.PayloadAsString(), Root))
		{
			UE_LOG(LogServerNpcManager, Warning, TEXT("EditorLookupNpcs: invalid JSON payload"));
			return;
		}

		if (Root->HasField(TEXT("error")))
		{
			FString ErrorMessage;
			Root->TryGetStringField(TEXT("error"), ErrorMessage);
			UE_LOG(LogServerNpcManager, Warning, TEXT("EditorLookupNpcs: service error: %s"), *ErrorMessage);
			return;
		}

		const TArray<TSharedPtr<FJsonValue>>* NpcsArray = nullptr;
		if (!Root->TryGetArrayField(TEXT("npcs"), NpcsArray) || !NpcsArray)
		{
			UE_LOG(LogServerNpcManager, Warning, TEXT("EditorLookupNpcs: response missing 'npcs' array"));
			return;
		}

		int32 MatchCount = 0;
		for (const TSharedPtr<FJsonValue>& Entry : *NpcsArray)
		{
			const TSharedPtr<FJsonObject>* NpcObj;
			if (!Entry->TryGetObject(NpcObj) || !NpcObj)
			{
				continue;
			}

			const FNpcMeta Meta = ParseNpcMeta(*NpcObj);
			if (Meta.NpcId.IsEmpty())
			{
				continue;
			}

			if (!Filter.IsEmpty())
			{
				const bool bMatches =
					Meta.NpcId.Contains(Filter, ESearchCase::IgnoreCase) ||
					Meta.DisplayName.Contains(Filter, ESearchCase::IgnoreCase) ||
					Meta.TemplateKey.Contains(Filter, ESearchCase::IgnoreCase) ||
					Meta.Role.Contains(Filter, ESearchCase::IgnoreCase);

				if (!bMatches)
				{
					continue;
				}
			}

			++MatchCount;
			UE_LOG(LogServerNpcManager, Log,
				TEXT("NPC[%d]: id=%s name='%s' role='%s' template='%s' spawns=%d"),
				MatchCount,
				*Meta.NpcId,
				*Meta.DisplayName,
				*Meta.Role,
				*Meta.TemplateKey,
				Meta.SpawnPoints.Num());
		}

		UE_LOG(LogServerNpcManager, Log, TEXT("EditorLookupNpcs: %d matches"), MatchCount);
	});

	Nats->RequestJson(UTPSNatsSubjectsConfig::Get().NpcMetaListSubject, TEXT("{}"), NatsTimeoutSeconds, Reply);
}

void UServerNpcManagerSubsystem::EditorSpawnNpcById(const FString& NpcId)
{
	if (NpcId.IsEmpty())
	{
		UE_LOG(LogServerNpcManager, Warning, TEXT("EditorSpawnNpcById: npc_id is required"));
		return;
	}

	UWorld* World = ResolveTargetWorld(TEXT(""));
	if (!World)
	{
		World = GetWorld();
	}

	if (!World)
	{
		UE_LOG(LogServerNpcManager, Warning, TEXT("EditorSpawnNpcById: no game world resolved"));
		return;
	}

	FVector SpawnLocation = FVector::ZeroVector;
	float SpawnYaw = 0.f;

	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0))
	{
		if (APawn* Pawn = PlayerController->GetPawn())
		{
			SpawnLocation = Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 200.f;
			SpawnYaw = Pawn->GetActorRotation().Yaw;
		}
	}

	const FString ZoneId = ResolveZoneId(World);
	UE_LOG(LogServerNpcManager, Log,
		TEXT("EditorSpawnNpcById: request spawn for '%s' at [%.1f, %.1f, %.1f], zone '%s'"),
		*NpcId,
		SpawnLocation.X,
		SpawnLocation.Y,
		SpawnLocation.Z,
		*ZoneId);

	RequestAndSpawnNpc(
		NpcId,
		ZoneId,
		SpawnLocation.X,
		SpawnLocation.Y,
		SpawnLocation.Z,
		SpawnYaw);
}
#endif

void UServerNpcManagerSubsystem::EnsureSpawnRequestSubscription()
{
	UNatsClientSubsystem* Nats = GetGameInstance() ? GetGameInstance()->GetSubsystem<UNatsClientSubsystem>() : nullptr;
	if (!Nats || !Nats->IsConnected())
	{
		return;
	}

	if (!SpawnRequestSubscription.IsValid())
	{
		FOnNatsMessage Delegate;
		Delegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UServerNpcManagerSubsystem, HandleSpawnRequest));
		SpawnRequestSubscription = Nats->Subscribe(UTPSNatsSubjectsConfig::Get().NpcSpawnRequestSubject, Delegate);
	}

	if (!PlayerContextRequestSubscription.IsValid())
	{
		FOnNatsMessage Delegate;
		Delegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UServerNpcManagerSubsystem, HandlePlayerContextRequest));
		PlayerContextRequestSubscription = Nats->Subscribe(UTPSNatsSubjectsConfig::Get().PlayerContextRequestSubject, Delegate);
	}
}

void UServerNpcManagerSubsystem::SpawnNpcFromMeta(UWorld* World, const FNpcMeta& Meta, const FNpcSpawnPoint& SpawnPoint)
{
	if (!World)
	{
		return;
	}

	// Snap spawn location to ground using a downward trace
	FVector RawLocation(SpawnPoint.X, SpawnPoint.Y, SpawnPoint.Z);
	FVector TraceStart = RawLocation + FVector(0.f, 0.f, 200.f);
	FVector TraceEnd = RawLocation - FVector(0.f, 0.f, 2000.f);
	FHitResult Hit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GroundedNpcSpawn), false);
	FVector GroundedLocation = RawLocation;
	if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
	{
		GroundedLocation = Hit.ImpactPoint + FVector(0.f, 0.f, 2.f); // Slight offset above ground
	}

	const FRotator Rotation(0.f, SpawnPoint.Yaw, 0.f);
	const FTransform SpawnTransform(Rotation, GroundedLocation);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	const TSubclassOf<ANPCCharacter> NpcClass = ResolveNpcClass(World, Meta);
	ANPCCharacter* NPC = World->SpawnActor<ANPCCharacter>(NpcClass, SpawnTransform, Params);
	if (!NPC)
	{
		UE_LOG(LogServerNpcManager, Error,
			TEXT("SpawnNpcFromMeta: failed to spawn NPC '%s' at [%.1f, %.1f, %.1f]"),
			*Meta.DisplayName, SpawnPoint.X, SpawnPoint.Y, SpawnPoint.Z);
		return;
	}

	NPC->InitializeFromNpcMeta(Meta, SpawnPoint);
	SpawnedNpcs.FindOrAdd(TWeakObjectPtr<UWorld>(World)).Add(NPC);

	UE_LOG(LogServerNpcManager, Log,
		TEXT("Spawned NPC '%s' (%s) at [%.1f, %.1f, %.1f] (grounded at [%.1f, %.1f, %.1f])"),
		*Meta.DisplayName, *Meta.NpcId, SpawnPoint.X, SpawnPoint.Y, SpawnPoint.Z, GroundedLocation.X, GroundedLocation.Y, GroundedLocation.Z);
}

UWorld* UServerNpcManagerSubsystem::ResolveTargetWorld(const FString& MapOrZoneId)
{
	if (!GEngine)
	{
		return nullptr;
	}

	UWorld* FallbackWorld = nullptr;

	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		UWorld* World = WorldContext.World();
		if (!World || !World->IsGameWorld())
		{
			continue;
		}

		if (!FallbackWorld)
		{
			FallbackWorld = World;
		}

		if (MapOrZoneId.IsEmpty())
		{
			continue;
		}

		if (ResolveZoneId(World).Equals(MapOrZoneId, ESearchCase::IgnoreCase))
		{
			return World;
		}

		FString MapName = World->GetMapName();
		MapName.RemoveFromStart(World->StreamingLevelsPrefix);
		if (MapName.Equals(MapOrZoneId, ESearchCase::IgnoreCase))
		{
			return World;
		}
	}

	return FallbackWorld;
}

void UServerNpcManagerSubsystem::HandleSpawnRequest(const FNatsMessage& Message)
{
	TSharedPtr<FJsonObject> Root;
	if (!TPSCoreJson::DeserializeObject(Message.PayloadAsString(), Root))
	{
		UE_LOG(LogServerNpcManager, Warning, TEXT("HandleSpawnRequest: invalid JSON payload"));
		return;
	}

	FString NpcId;
	FString ZoneId;
	double PositionX = 0.0;
	double PositionY = 0.0;
	double PositionZ = 0.0;
	double Yaw = 0.0;

	Root->TryGetStringField(TEXT("npc_id"), NpcId);
	Root->TryGetStringField(TEXT("zone_id"), ZoneId);
	if (ZoneId.IsEmpty())
	{
		Root->TryGetStringField(TEXT("map_id"), ZoneId);
	}
	Root->TryGetNumberField(TEXT("position_x"), PositionX);
	Root->TryGetNumberField(TEXT("position_y"), PositionY);
	Root->TryGetNumberField(TEXT("position_z"), PositionZ);
	Root->TryGetNumberField(TEXT("yaw"), Yaw);

	if (NpcId.IsEmpty())
	{
		UE_LOG(LogServerNpcManager, Warning, TEXT("HandleSpawnRequest: missing npc_id"));
		return;
	}

	RequestAndSpawnNpc(NpcId, ZoneId, PositionX, PositionY, PositionZ, Yaw);
}

void UServerNpcManagerSubsystem::RequestAndSpawnNpc(
	const FString& NpcId,
	const FString& ZoneId,
	double PositionX,
	double PositionY,
	double PositionZ,
	double Yaw)
{
	if (NpcId.IsEmpty())
	{
		UE_LOG(LogServerNpcManager, Warning, TEXT("RequestAndSpawnNpc: missing npc_id"));
		return;
	}

	UWorld* World = ResolveTargetWorld(ZoneId);
	if (!World)
	{
		UE_LOG(LogServerNpcManager, Warning, TEXT("RequestAndSpawnNpc: no target world resolved for '%s'"), *ZoneId);
		return;
	}

	UNatsClientSubsystem* Nats = GetGameInstance() ? GetGameInstance()->GetSubsystem<UNatsClientSubsystem>() : nullptr;
	if (!Nats || !Nats->IsConnected())
	{
		UE_LOG(LogServerNpcManager, Warning, TEXT("RequestAndSpawnNpc: NATS not connected"));
		return;
	}

	const FString ResolvedZoneId = ZoneId.IsEmpty() ? ResolveZoneId(World) : ZoneId;
	const FString Payload = FString::Printf(TEXT("{\"npc_id\":\"%s\"}"), *NpcId);
	TWeakObjectPtr<UWorld> WeakWorld(World);

	FOnNatsReply Reply;
	Reply.BindLambda([this, WeakWorld, NpcId, ResolvedZoneId, PositionX, PositionY, PositionZ, Yaw](bool bSuccess, const FNatsMessage& ReplyMessage)
	{
		UWorld* TargetWorld = WeakWorld.Get();
		if (!TargetWorld)
		{
			UE_LOG(LogServerNpcManager, Warning, TEXT("RequestAndSpawnNpc reply: target world no longer valid"));
			return;
		}

		if (!bSuccess)
		{
			UE_LOG(LogServerNpcManager, Warning, TEXT("RequestAndSpawnNpc: npc.meta.get timed out for '%s'"), *NpcId);
			return;
		}

		TSharedPtr<FJsonObject> RootObject;
		if (!TPSCoreJson::DeserializeObject(ReplyMessage.PayloadAsString(), RootObject))
		{
			UE_LOG(LogServerNpcManager, Warning, TEXT("RequestAndSpawnNpc: invalid npc.meta.get response"));
			return;
		}

		if (RootObject->HasField(TEXT("error")))
		{
			FString ErrorMessage;
			RootObject->TryGetStringField(TEXT("error"), ErrorMessage);
			UE_LOG(LogServerNpcManager, Warning, TEXT("RequestAndSpawnNpc: npc '%s' lookup failed: %s"), *NpcId, *ErrorMessage);
			return;
		}

		FNpcMeta Meta = ParseNpcMeta(RootObject);
		if (Meta.NpcId.IsEmpty())
		{
			UE_LOG(LogServerNpcManager, Warning, TEXT("RequestAndSpawnNpc: resolved NPC metadata was empty for '%s'"), *NpcId);
			return;
		}

		FNpcSpawnPoint SpawnPoint;
		SpawnPoint.Id = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
		SpawnPoint.ZoneId = ResolvedZoneId;
		SpawnPoint.X = static_cast<float>(PositionX);
		SpawnPoint.Y = static_cast<float>(PositionY);
		SpawnPoint.Z = static_cast<float>(PositionZ);
		SpawnPoint.Yaw = static_cast<float>(Yaw);
		SpawnPoint.SpawnPolicy = TEXT("manual_command");

		SpawnNpcFromMeta(TargetWorld, Meta, SpawnPoint);
	});

	Nats->RequestJson(UTPSNatsSubjectsConfig::Get().NpcMetaGetSubject, Payload, NatsTimeoutSeconds, Reply);
}

void UServerNpcManagerSubsystem::HandlePlayerContextRequest(const FNatsMessage& Message)
{
	UNatsClientSubsystem* Nats = GetGameInstance() ? GetGameInstance()->GetSubsystem<UNatsClientSubsystem>() : nullptr;
	if (!Nats || !Nats->IsConnected() || Message.ReplyTo.IsEmpty())
	{
		return;
	}

	TSharedPtr<FJsonObject> Root;
	if (!TPSCoreJson::DeserializeObject(Message.PayloadAsString(), Root))
	{
		TSharedRef<FJsonObject> ErrorResponse = MakeShared<FJsonObject>();
		ErrorResponse->SetStringField(TEXT("error"), TEXT("invalid request"));
		Nats->PublishJson(Message.ReplyTo, TPSCoreJson::SerializeObject(ErrorResponse));
		return;
	}

	FString CharacterId;
	Root->TryGetStringField(TEXT("character_id"), CharacterId);
	if (CharacterId.IsEmpty())
	{
		TSharedRef<FJsonObject> ErrorResponse = MakeShared<FJsonObject>();
		ErrorResponse->SetStringField(TEXT("error"), TEXT("character_id required"));
		Nats->PublishJson(Message.ReplyTo, TPSCoreJson::SerializeObject(ErrorResponse));
		return;
	}

	ATPSCoreMechanicsCharacter* MatchedCharacter = nullptr;
	UWorld* MatchedWorld = nullptr;

	if (GEngine)
	{
		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			UWorld* World = WorldContext.World();
			if (!World || !World->IsGameWorld())
			{
				continue;
			}

			for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
			{
				APlayerController* PlayerController = It->Get();
				ATPSCoreMechanicsCharacter* Character = PlayerController ? Cast<ATPSCoreMechanicsCharacter>(PlayerController->GetPawn()) : nullptr;
				if (Character && Character->GetCharacterId().Equals(CharacterId, ESearchCase::IgnoreCase))
				{
					MatchedCharacter = Character;
					MatchedWorld = World;
					break;
				}
			}

			if (MatchedCharacter)
			{
				break;
			}
		}
	}

	TSharedRef<FJsonObject> Response = MakeShared<FJsonObject>();
	if (!MatchedCharacter || !MatchedWorld)
	{
		Response->SetStringField(TEXT("error"), TEXT("player context not found"));
		Response->SetStringField(TEXT("character_id"), CharacterId);
		Nats->PublishJson(Message.ReplyTo, TPSCoreJson::SerializeObject(Response));
		return;
	}

	const FVector Location = MatchedCharacter->GetActorLocation();
	const FRotator Rotation = MatchedCharacter->GetActorRotation();
	Response->SetStringField(TEXT("character_id"), CharacterId);
	Response->SetStringField(TEXT("zone_id"), ResolveZoneId(MatchedWorld));
	Response->SetNumberField(TEXT("position_x"), Location.X);
	Response->SetNumberField(TEXT("position_y"), Location.Y);
	Response->SetNumberField(TEXT("position_z"), Location.Z);
	Response->SetNumberField(TEXT("yaw"), Rotation.Yaw);
	Nats->PublishJson(Message.ReplyTo, TPSCoreJson::SerializeObject(Response));
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
	Obj->TryGetStringField(TEXT("skeletal_mesh_id"), Meta.SkeletalMeshId);
	Obj->TryGetStringField(TEXT("actor_class_id"), Meta.ActorClassId);

	if (Meta.SkeletalMeshId.IsEmpty())
	{
		FString LegacySkeletalMeshPath;
		if (Obj->TryGetStringField(TEXT("skeletal_mesh_path"), LegacySkeletalMeshPath))
		{
			Meta.SkeletalMeshId = ExtractAssetIdFromObjectPath(LegacySkeletalMeshPath);
		}
	}

	if (Meta.ActorClassId.IsEmpty())
	{
		FString LegacyActorClassPath;
		if (Obj->TryGetStringField(TEXT("actor_class_path"), LegacyActorClassPath))
		{
			Meta.ActorClassId = ExtractAssetIdFromObjectPath(LegacyActorClassPath);
		}
	}
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
