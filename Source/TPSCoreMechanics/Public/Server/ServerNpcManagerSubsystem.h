// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NPC/NpcTypes.h"
#include "NatsSubscription.h"
#include "ServerNpcManagerSubsystem.generated.h"

class ANPCCharacter;
struct FNatsMessage;

/**
 * Server-only subsystem that, on each world BeginPlay, fetches NPC metadata
 * for the current zone from npc-metadata-service via NATS and spawns one
 * ANPCCharacter per spawn-point entry.
 *
 * NATS subject defaults are configured in /Script/TPSCoreMechanics.TPSNatsSubjectsConfig
 * (e.g. NpcMetaByZoneSubject="npc.meta.by_zone").
 *   Request:  {"zone_id":"<id>"}
 *   Response: {"npcs":[{ ...NpcMetaFull... }]}
 *
 * The zone_id is read from ATPSCoreGameMode::ZoneId. If it is empty, the
 * map leaf name is used as a fallback so levels work without explicit config.
 *
 * Spawned ANPCCharacter actors are replicated automatically to all clients.
 * Only active on dedicated servers (ShouldCreateSubsystem guard).
 */
UCLASS()
class TPSCOREMECHANICS_API UServerNpcManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

#if WITH_EDITOR
	void EditorLookupNpcs(const FString& Filter);
	void EditorSpawnNpcById(const FString& NpcId);
#endif

private:
	FDelegateHandle PostLoadMapHandle;
	FNatsSubscriptionHandle SpawnRequestSubscription;
	FNatsSubscriptionHandle PlayerContextRequestSubscription;

	/** Alive NPC actors keyed by the world they belong to. */
	TMap<TWeakObjectPtr<UWorld>, TArray<TWeakObjectPtr<ANPCCharacter>>> SpawnedNpcs;

	void OnPostLoadMap(UWorld* World);
	void LoadNpcsForZone(UWorld* World, const FString& ZoneId);
	void EnsureSpawnRequestSubscription();
	void SpawnNpcFromMeta(UWorld* World, const FNpcMeta& Meta, const FNpcSpawnPoint& SpawnPoint);
	static UWorld* ResolveTargetWorld(const FString& MapOrZoneId);
	void RequestAndSpawnNpc(
		const FString& NpcId,
		const FString& ZoneId,
		double PositionX,
		double PositionY,
		double PositionZ,
		double Yaw
	);

	/** Determine zone id: reads ATPSCoreGameMode::ZoneId, falls back to map leaf name. */
	static FString ResolveZoneId(UWorld* World);

	static FNpcMeta      ParseNpcMeta(const TSharedPtr<FJsonObject>& Obj);
	static FNpcSpawnPoint ParseSpawnPoint(const TSharedPtr<FJsonObject>& Obj);
	static FNpcBehaviorConfig ParseBehaviorConfig(const TSharedPtr<FJsonObject>& Obj);

	UFUNCTION()
	void HandleSpawnRequest(const FNatsMessage& Message);

	UFUNCTION()
	void HandlePlayerContextRequest(const FNatsMessage& Message);

};
