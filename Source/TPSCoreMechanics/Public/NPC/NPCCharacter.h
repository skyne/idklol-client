// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NPC/NpcTypes.h"
#include "NPCCharacter.generated.h"

/**
 * ANPCCharacter — server-spawned, fully replicated NPC actor.
 *
 * Instantiated by UServerNpcManagerSubsystem on BeginPlay of the gameplay
 * world (dedicated server only). Core identity—NpcId, DisplayName, Role,
 * Faction, ModelId—is replicated in a single FNpcReplicatedData struct so
 * clients receive one OnRep callback when any field changes.
 *
 * Override OnNpcInitialized in Blueprint to apply the skeletal mesh, set up
 * a nameplate widget, etc. It fires both on the server (immediately after
 * InitializeFromNpcMeta) and on every client after the first replication and
 * on every subsequent replication when the data changes.
 */
UCLASS(Blueprintable, BlueprintType)
class TPSCOREMECHANICS_API ANPCCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ANPCCharacter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Called on the server to stamp all identity and behavior data onto this actor. */
	void InitializeFromNpcMeta(const FNpcMeta& Meta, const FNpcSpawnPoint& SpawnPoint);

	UFUNCTION(BlueprintPure, Category = "NPC")
	const FNpcReplicatedData& GetNpcData() const { return NpcData; }

protected:
	virtual void BeginPlay() override;

	/**
	 * Fired on server (from InitializeFromNpcMeta) and on each client as soon
	 * as replicated data arrives or updates.
	 * Override in Blueprint to apply skeletal mesh, nameplate, etc.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "NPC")
	void OnNpcInitialized(const FNpcReplicatedData& Data);

private:
	UPROPERTY(ReplicatedUsing = OnRep_NpcData)
	FNpcReplicatedData NpcData;

	UFUNCTION()
	void OnRep_NpcData();
};
