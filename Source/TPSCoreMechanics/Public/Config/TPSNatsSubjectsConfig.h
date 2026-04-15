#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TPSNatsSubjectsConfig.generated.h"

/**
 * Centralized NATS subject configuration loaded from DefaultGame.ini.
 */
UCLASS(Config=Game, DefaultConfig)
class TPSCOREMECHANICS_API UTPSNatsSubjectsConfig : public UObject
{
	GENERATED_BODY()

public:
	static const UTPSNatsSubjectsConfig& Get();

	UPROPERTY(Config, EditAnywhere, Category = "Characters")
	FString CharactersGetSubject = TEXT("characters.get");

	UPROPERTY(Config, EditAnywhere, Category = "NPC")
	FString NpcSpawnRequestSubject = TEXT("npc.spawn.request");

	UPROPERTY(Config, EditAnywhere, Category = "NPC")
	FString PlayerContextRequestSubject = TEXT("server.player_context.get");

	UPROPERTY(Config, EditAnywhere, Category = "NPC")
	FString NpcMetaByZoneSubject = TEXT("npc.meta.by_zone");

	UPROPERTY(Config, EditAnywhere, Category = "NPC")
	FString NpcMetaListSubject = TEXT("npc.meta.list");

	UPROPERTY(Config, EditAnywhere, Category = "NPC")
	FString NpcMetaGetSubject = TEXT("npc.meta.get");

	UPROPERTY(Config, EditAnywhere, Category = "NPC")
	FString NpcInteractionRequestSubject = TEXT("npc.interactions.request");

	UPROPERTY(Config, EditAnywhere, Category = "NPC")
	FString NpcInteractionResponseSubject = TEXT("npc.interactions.response");

	UPROPERTY(Config, EditAnywhere, Category = "Server")
	FString ServerMapSubjectTemplate = TEXT("server.%s.map");

	UPROPERTY(Config, EditAnywhere, Category = "Server")
	FString ServerStatusSubjectTemplate = TEXT("server.%s.status");

	FString MakeServerMapSubject(const FString& InstanceId) const;
	FString MakeServerStatusSubject(const FString& InstanceId) const;
};
