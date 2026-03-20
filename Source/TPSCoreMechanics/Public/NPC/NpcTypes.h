// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NpcTypes.generated.h"

/**
 * A single spawn-point entry as returned by npc-metadata-service.
 * Coordinates are in Unreal world-space (centimetres). Yaw in degrees.
 */
USTRUCT(BlueprintType)
struct TPSCOREMECHANICS_API FNpcSpawnPoint
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	FString Id;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	FString ZoneId;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	float X = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	float Y = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	float Z = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	float Yaw = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	FString SpawnPolicy;
};

/** Behavioral limits for an NPC, from npc-metadata-service. */
USTRUCT(BlueprintType)
struct TPSCOREMECHANICS_API FNpcBehaviorConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	float InteractionRadius = 300.f;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	int32 CooldownMs = 5000;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	int32 MaxConcurrentInteractions = 1;
};

/**
 * Full NPC metadata fetched from npc-metadata-service via npc.meta.by_zone.
 * SpawnPoints may contain one or more entries; the manager spawns one actor per point.
 */
USTRUCT(BlueprintType)
struct TPSCOREMECHANICS_API FNpcMeta
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	FString NpcId;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	FString DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	FString Role;

	/** Asset key used by the client to resolve the skeletal mesh (e.g. "human_guard_m"). */
	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	FString ModelId;

	/** Optional skeletal mesh asset id under the fixed NPC mesh folder (e.g. "SKM_Guard_M"). */
	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	FString SkeletalMeshId;

	/** Optional actor class asset id under the fixed NPC actor folder (e.g. "BP_Guard_NPC"). */
	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	FString ActorClassId;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	FString Faction;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	FString TemplateKey;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	FString Tone;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	TArray<FString> Rules;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	bool bIsPersistent = false;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	TArray<FNpcSpawnPoint> SpawnPoints;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	FNpcBehaviorConfig BehaviorConfig;
};

/**
 * Subset of FNpcMeta that is replicated from server to clients. Packed into
 * a single struct so one OnRep covers all identity fields.
 */
USTRUCT(BlueprintType)
struct TPSCOREMECHANICS_API FNpcReplicatedData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	FString NpcId;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	FString DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	FString Role;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	FString Faction;

	/** Resolved by the client to select the actual skeletal mesh. */
	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	FString ModelId;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	FString SkeletalMeshId;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	FString ActorClassId;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	float InteractionRadius = 300.f;
};
