// Fill out your copyright notice in the Description page of Project Settings.

#include "NPC/NPCCharacter.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogNPCCharacter, Log, All);

namespace
{
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

	void AddAnimBlueprintCandidatesFromAssetId(const FString& AssetId, TArray<FString>& OutClassPaths)
	{
		if (AssetId.IsEmpty())
		{
			return;
		}

		auto AddPathIfMissing = [&OutClassPaths](const FString& Path)
		{
			if (!Path.IsEmpty())
			{
				OutClassPaths.AddUnique(Path);
			}
		};

		AddPathIfMissing(FString::Printf(TEXT("/Game/Characters/NPC/ABP_%s.ABP_%s_C"), *AssetId, *AssetId));
		AddPathIfMissing(FString::Printf(TEXT("/Game/Characters/NPC/Animations/ABP_%s.ABP_%s_C"), *AssetId, *AssetId));

		FString Normalized = AssetId;
		if (Normalized.StartsWith(TEXT("SKM_")))
		{
			Normalized = Normalized.Mid(4);
		}
		if (Normalized.StartsWith(TEXT("NPC_")))
		{
			Normalized = Normalized.Mid(4);
		}

		AddPathIfMissing(FString::Printf(TEXT("/Game/Characters/NPC/ABP_%s.ABP_%s_C"), *Normalized, *Normalized));
		AddPathIfMissing(FString::Printf(TEXT("/Game/Characters/NPC/Animations/ABP_%s.ABP_%s_C"), *Normalized, *Normalized));

		AddPathIfMissing(TEXT("/Game/Characters/Mannequins/Animations/ABP_Quinn.ABP_Quinn_C"));
		AddPathIfMissing(TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny_C"));

		if (Normalized.Contains(TEXT("_F"), ESearchCase::IgnoreCase) || Normalized.Contains(TEXT("Female"), ESearchCase::IgnoreCase))
		{
			OutClassPaths.Insert(TEXT("/Game/Characters/Mannequins/Animations/ABP_Quinn.ABP_Quinn_C"), 0);
		}
		else if (Normalized.Contains(TEXT("_M"), ESearchCase::IgnoreCase) || Normalized.Contains(TEXT("Male"), ESearchCase::IgnoreCase))
		{
			OutClassPaths.Insert(TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny_C"), 0);
		}
	}

	TSubclassOf<UAnimInstance> ResolveFallbackAnimClass(const FNpcReplicatedData& Data)
	{
		const FString RawId = !Data.SkeletalMeshId.IsEmpty() ? Data.SkeletalMeshId : Data.ModelId;
		const FString AssetId = ExtractAssetIdFromObjectPath(RawId);

		TArray<FString> Candidates;
		AddAnimBlueprintCandidatesFromAssetId(AssetId, Candidates);

		for (const FString& CandidatePath : Candidates)
		{
			if (UClass* AnimClass = LoadClass<UAnimInstance>(nullptr, *CandidatePath))
			{
				return AnimClass;
			}
		}

		return nullptr;
	}
}

ANPCCharacter::ANPCCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bRunPhysicsWithNoController = true;
	}
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
	ApplyDefaultMeshIfNeeded();
	ApplyDefaultAnimationIfNeeded();

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
		ApplyDefaultMeshIfNeeded();
		ApplyDefaultAnimationIfNeeded();
	}
}

void ANPCCharacter::OnRep_NpcData()
{
	OnNpcInitialized(NpcData);
	ApplyDefaultMeshIfNeeded();
	ApplyDefaultAnimationIfNeeded();
}

void ANPCCharacter::ApplyDefaultMeshIfNeeded()
{
	if (IsNetMode(NM_DedicatedServer))
	{
		return;
	}

	USkeletalMeshComponent* MeshComponent = GetMesh();
	if (!MeshComponent)
	{
		return;
	}

	if (MeshComponent->GetSkeletalMeshAsset() != nullptr)
	{
		return;
	}

	const FString RawId = !NpcData.SkeletalMeshId.IsEmpty() ? NpcData.SkeletalMeshId : NpcData.ModelId;
	const FString AssetId = ExtractAssetIdFromObjectPath(RawId);

	if (AssetId.IsEmpty())
	{
		UE_LOG(LogNPCCharacter, Warning,
			TEXT("NPC '%s': no SkeletalMeshId/ModelId available for mesh fallback"),
			*NpcData.NpcId);
		return;
	}

	TArray<FString> CandidateMeshPaths;
	if (RawId.StartsWith(TEXT("/Game/")) && RawId.Contains(TEXT(".")))
	{
		CandidateMeshPaths.AddUnique(RawId);
	}
	CandidateMeshPaths.AddUnique(FString::Printf(TEXT("/Game/Characters/NPC/%s.%s"), *AssetId, *AssetId));
	CandidateMeshPaths.AddUnique(FString::Printf(TEXT("/Game/Characters/NPC/Meshes/%s.%s"), *AssetId, *AssetId));

	for (const FString& MeshPath : CandidateMeshPaths)
	{
		if (USkeletalMesh* SkeletalMesh = LoadObject<USkeletalMesh>(nullptr, *MeshPath))
		{
			MeshComponent->SetSkeletalMeshAsset(SkeletalMesh);
			UE_LOG(LogNPCCharacter, Log,
				TEXT("NPC '%s': applied default mesh '%s'"),
				*NpcData.NpcId,
				*MeshPath);
			return;
		}
	}

	UE_LOG(LogNPCCharacter, Warning,
		TEXT("NPC '%s': failed to resolve mesh from SkeletalMeshId='%s', ModelId='%s'"),
		*NpcData.NpcId,
		*NpcData.SkeletalMeshId,
		*NpcData.ModelId);
}

void ANPCCharacter::ApplyDefaultAnimationIfNeeded()
{
	if (IsNetMode(NM_DedicatedServer))
	{
		return;
	}

	USkeletalMeshComponent* MeshComponent = GetMesh();
	if (!MeshComponent)
	{
		return;
	}

	if (MeshComponent->GetSkeletalMeshAsset() == nullptr)
	{
		return;
	}

	if (MeshComponent->GetAnimClass() != nullptr)
	{
		return;
	}

	TSubclassOf<UAnimInstance> AnimClassToApply = DefaultNpcAnimClass;
	if (!AnimClassToApply)
	{
		AnimClassToApply = ResolveFallbackAnimClass(NpcData);
	}

	if (!AnimClassToApply)
	{
		UE_LOG(LogNPCCharacter, Warning,
			TEXT("NPC '%s': no default AnimBP resolved (SkeletalMeshId='%s', ModelId='%s')"),
			*NpcData.NpcId,
			*NpcData.SkeletalMeshId,
			*NpcData.ModelId);
		return;
	}

	MeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	MeshComponent->SetAnimInstanceClass(AnimClassToApply);

	UE_LOG(LogNPCCharacter, Log,
		TEXT("NPC '%s': applied default anim class '%s'"),
		*NpcData.NpcId,
		*GetNameSafe(AnimClassToApply));
}
