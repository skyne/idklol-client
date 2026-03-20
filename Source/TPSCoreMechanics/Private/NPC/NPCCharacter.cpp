// Fill out your copyright notice in the Description page of Project Settings.

#include "NPC/NPCCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SkeletalMeshComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogNPCCharacter, Log, All);

namespace
{
	constexpr TCHAR NpcMeshFolder[] = TEXT("/Game/Characters/NPC");
}

ANPCCharacter::ANPCCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);
}

void ANPCCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANPCCharacter, NpcData);
}

void ANPCCharacter::InitializeFromNpcMeta(const FNpcMeta& Meta, const FNpcSpawnPoint& SpawnPoint)
{
	NpcData.NpcId             = Meta.NpcId;
	NpcData.DisplayName       = Meta.DisplayName;
	NpcData.Role              = Meta.Role;
	NpcData.Faction           = Meta.Faction;
	NpcData.ModelId           = Meta.ModelId;
	NpcData.SkeletalMeshId    = Meta.SkeletalMeshId;
	NpcData.ActorClassId      = Meta.ActorClassId;
	NpcData.InteractionRadius = Meta.BehaviorConfig.InteractionRadius;

	// Also fire on the server so server-side logic (AI, etc.) can react.
	OnNpcInitialized(NpcData);

	UE_LOG(LogNPCCharacter, Log, TEXT("NPC '%s' (%s) initialized at [%.1f, %.1f, %.1f]"),
		*NpcData.DisplayName, *NpcData.NpcId,
		SpawnPoint.X, SpawnPoint.Y, SpawnPoint.Z);
}

void ANPCCharacter::BeginPlay()
{
	Super::BeginPlay();

	// On clients the data may already have been replicated before BeginPlay fires
	// (fast-path for actors that existed before the client fully joined). Fire the
	// event now if we already have valid data so the BP override runs in every path.
	if (!NpcData.NpcId.IsEmpty())
	{
		OnNpcInitialized(NpcData);
	}
}

void ANPCCharacter::OnRep_NpcData()
{
	OnNpcInitialized(NpcData);
}

void ANPCCharacter::OnNpcInitialized_Implementation(const FNpcReplicatedData& Data)
{
	// Dedicated servers don't render meshes.
	if (IsNetMode(NM_DedicatedServer) || Data.SkeletalMeshId.IsEmpty())
	{
		return;
	}

	const FString MeshPath = FString::Printf(TEXT("%s/%s.%s"),
		NpcMeshFolder, *Data.SkeletalMeshId, *Data.SkeletalMeshId);

	if (USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *MeshPath))
	{
		GetMesh()->SetSkeletalMeshAsset(Mesh);
		UE_LOG(LogNPCCharacter, Log, TEXT("NPC '%s': applied mesh '%s'"),
			*Data.NpcId, *Data.SkeletalMeshId);
	}
	else
	{
		UE_LOG(LogNPCCharacter, Warning,
			TEXT("NPC '%s': could not load mesh '%s' (path '%s')"),
			*Data.NpcId, *Data.SkeletalMeshId, *MeshPath);
	}
}
