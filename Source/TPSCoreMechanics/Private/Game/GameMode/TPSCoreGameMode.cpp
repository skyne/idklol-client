// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/GameMode/TPSCoreGameMode.h"
#include "TPSCoreMechanics/TPSCoreMechanicsCharacter.h"
#include "Server/ServerCharacterLoaderSubsystem.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogTPSCoreGameMode, Log, All);

ATPSCoreGameMode::ATPSCoreGameMode()
{
	// Default pawn class should be ATPSCoreMechanicsCharacter or a subclass
	// This is required for the character appearance system to work
	DefaultPawnClass = ATPSCoreMechanicsCharacter::StaticClass();
}

void ATPSCoreGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	// Validate that the DefaultPawnClass is ATPSCoreMechanicsCharacter or a subclass
	if (DefaultPawnClass && !DefaultPawnClass->IsChildOf(ATPSCoreMechanicsCharacter::StaticClass()))
	{
		UE_LOG(LogTemp, Error, 
			TEXT("TPSCoreGameMode: DefaultPawnClass must be ATPSCoreMechanicsCharacter or a subclass for character appearance system to work! Current class: %s"),
			*DefaultPawnClass->GetName());
	}
}

UCharacterClassInfo* ATPSCoreGameMode::GetCharacterClassDefaultInfo() const
{
	return ClassDefaults;
}

APlayerController* ATPSCoreGameMode::Login(
	UPlayer* NewPlayer,
	ENetRole InRemoteRole,
	const FString& Portal,
	const FString& Options,
	const FUniqueNetIdRepl& UniqueId,
	FString& ErrorMessage)
{
	APlayerController* PC = Super::Login(NewPlayer, InRemoteRole, Portal, Options, UniqueId, ErrorMessage);

	if (PC && IsRunningDedicatedServer())
	{
		const FString CharacterId = UGameplayStatics::ParseOption(Options, TEXT("CharacterId"));
		if (!CharacterId.IsEmpty())
		{
			PendingCharacterIds.Emplace(FObjectKey(PC), CharacterId);
			UE_LOG(LogTPSCoreGameMode, Log, TEXT("Login: stored CharacterId=%s for controller %s"), *CharacterId, *PC->GetName());
		}
		else
		{
			UE_LOG(LogTPSCoreGameMode, Warning, TEXT("Login: no CharacterId in travel options for %s"), *PC->GetName());
		}
	}

	return PC;
}

void ATPSCoreGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!IsRunningDedicatedServer() || !NewPlayer)
	{
		return;
	}

	FString CharacterId;
	if (!PendingCharacterIds.RemoveAndCopyValue(FObjectKey(NewPlayer), CharacterId) || CharacterId.IsEmpty())
	{
		UE_LOG(LogTPSCoreGameMode, Warning, TEXT("PostLogin: no CharacterId for %s, skipping character initialization"), *NewPlayer->GetName());
		return;
	}

	UServerCharacterLoaderSubsystem* Loader = GetGameInstance()->GetSubsystem<UServerCharacterLoaderSubsystem>();
	if (!Loader)
	{
		UE_LOG(LogTPSCoreGameMode, Error, TEXT("PostLogin: UServerCharacterLoaderSubsystem not available"));
		return;
	}

	UE_LOG(LogTPSCoreGameMode, Log, TEXT("PostLogin: fetching character %s for %s"), *CharacterId, *NewPlayer->GetName());

	// Capture a weak ref so the callback is safe if the controller goes away before NATS replies
	TWeakObjectPtr<APlayerController> WeakPC(NewPlayer);

	FOnCharacterLoaded LoadedDelegate;
	LoadedDelegate.BindLambda([WeakPC, CharacterId](bool bSuccess, const FCharacterData& CharacterData)
	{
		if (!bSuccess)
		{
			UE_LOG(LogTPSCoreGameMode, Error, TEXT("PostLogin: failed to load character %s"), *CharacterId);
			return;
		}

		APlayerController* PC = WeakPC.Get();
		if (!PC)
		{
			UE_LOG(LogTPSCoreGameMode, Warning, TEXT("PostLogin: controller gone before character %s arrived"), *CharacterId);
			return;
		}

		ATPSCoreMechanicsCharacter* Character = Cast<ATPSCoreMechanicsCharacter>(PC->GetPawn());
		if (!Character)
		{
			UE_LOG(LogTPSCoreGameMode, Error,
				TEXT("PostLogin: pawn for %s is not ATPSCoreMechanicsCharacter (class: %s)"),
				*PC->GetName(),
				PC->GetPawn() ? *PC->GetPawn()->GetClass()->GetName() : TEXT("nullptr"));
			return;
		}

		Character->InitializeFromCharacterData(CharacterData);
		UE_LOG(LogTPSCoreGameMode, Log, TEXT("PostLogin: initialized character '%s' (%s)"), *CharacterData.Name, *CharacterId);
	});

	Loader->FetchCharacter(CharacterId, LoadedDelegate);
}
