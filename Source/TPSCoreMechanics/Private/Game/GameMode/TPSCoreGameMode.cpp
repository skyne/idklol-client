// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/GameMode/TPSCoreGameMode.h"
#include "Auth/JwtClaimsHelper.h"
#include "Game/PlayerController/TPSCorePlayerController.h"
#include "TPSCoreMechanics/TPSCoreMechanicsCharacter.h"
#include "TPSCoreMechanics/TPSCoreMechanics.h"
#include "Server/ServerCharacterLoaderSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameSession.h"

ATPSCoreGameMode::ATPSCoreGameMode()
{
	// Default pawn class should be ATPSCoreMechanicsCharacter or a subclass
	// This is required for the character appearance system to work
	DefaultPawnClass = ATPSCoreMechanicsCharacter::StaticClass();
	PlayerControllerClass = ATPSCorePlayerController::StaticClass();
}

void ATPSCoreGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	// Validate that the DefaultPawnClass is ATPSCoreMechanicsCharacter or a subclass
	if (DefaultPawnClass && !DefaultPawnClass->IsChildOf(ATPSCoreMechanicsCharacter::StaticClass()))
	{
		LOG_ERROR("TPSCoreGameMode: DefaultPawnClass must be ATPSCoreMechanicsCharacter or a subclass for character appearance system to work! Current class: %s",
			*DefaultPawnClass->GetName());
	}
}

UCharacterClassInfo* ATPSCoreGameMode::GetCharacterClassDefaultInfo() const
{
	return ClassDefaults;
}

// ─── PreLogin ────────────────────────────────────────────────────────────────

void ATPSCoreGameMode::PreLogin(const FString& Options, const FString& Address,
	const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (!ErrorMessage.IsEmpty())
	{
		return; // parent already rejected
	}

	if (!IsRunningDedicatedServer())
	{
		return; // auth check only applies on the dedicated server
	}

	const FString AuthToken = UGameplayStatics::ParseOption(Options, TEXT("AuthToken"));
	if (AuthToken.IsEmpty())
	{
		ErrorMessage = TEXT("AuthToken required");
		LOG_WARNING("[TPSCoreGameMode] PreLogin rejected %s — no AuthToken", *Address);
		return;
	}

	FString Email;
	if (!TPSCoreAuth::ExtractJwtEmailClaim(AuthToken, Email) || Email.IsEmpty())
	{
		ErrorMessage = TEXT("Invalid AuthToken");
		LOG_WARNING("[TPSCoreGameMode] PreLogin rejected %s — could not parse email from JWT payload", *Address);
		return;
	}

	// TODO: validate the RS256 JWT signature against Keycloak's public key before trusting
	// claims in production. Until then, ownership is still enforced in PostLogin by
	// cross-checking the email against the character's user_email DB column.
	LOG("[TPSCoreGameMode] PreLogin accepted %s (email=%s)", *Address, *Email);
}

// ─── Login ───────────────────────────────────────────────────────────────────

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
			LOG("[TPSCoreGameMode] Login stored CharacterId=%s for %s", *CharacterId, *PC->GetName());
		}
		else
		{
			LOG_WARNING("[TPSCoreGameMode] Login has no CharacterId for %s", *PC->GetName());
		}

		// Re-parse JWT claims to keep owner identity attached to this controller.
		const FString AuthToken = UGameplayStatics::ParseOption(Options, TEXT("AuthToken"));
		FString OwnerEmail;
		if (!AuthToken.IsEmpty() && TPSCoreAuth::ExtractJwtEmailClaim(AuthToken, OwnerEmail) && !OwnerEmail.IsEmpty())
		{
			PendingOwnerEmails.Emplace(FObjectKey(PC), OwnerEmail);
			LOG_DEBUG("[TPSCoreGameMode] Login stored owner email for %s", *PC->GetName());
		}
	}

	return PC;
}

void ATPSCoreGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// GameMode only exists on authority; we enforce dedicated-server-only behavior here.
	if (!IsRunningDedicatedServer() || !NewPlayer)
	{
		return;
	}

	FString CharacterId;
	if (!PendingCharacterIds.RemoveAndCopyValue(FObjectKey(NewPlayer), CharacterId) || CharacterId.IsEmpty())
	{
		LOG_WARNING("[TPSCoreGameMode] PostLogin missing CharacterId for %s", *NewPlayer->GetName());
		return;
	}

	FString OwnerEmail;
	PendingOwnerEmails.RemoveAndCopyValue(FObjectKey(NewPlayer), OwnerEmail);

	UServerCharacterLoaderSubsystem* Loader = GetGameInstance()->GetSubsystem<UServerCharacterLoaderSubsystem>();
	if (!Loader)
	{
		LOG_ERROR("[TPSCoreGameMode] PostLogin missing UServerCharacterLoaderSubsystem");
		return;
	}

	LOG("[TPSCoreGameMode] PostLogin fetching character %s for %s (owner=%s)",
		*CharacterId, *NewPlayer->GetName(), *OwnerEmail);

	TWeakObjectPtr<APlayerController> WeakPC(NewPlayer);
	TWeakObjectPtr<ATPSCoreGameMode> WeakGM(this);

	FOnCharacterLoaded LoadedDelegate;
	LoadedDelegate.BindLambda([WeakPC, WeakGM, CharacterId, OwnerEmail](bool bSuccess, const FCharacterData& CharacterData)
	{
		if (!bSuccess)
		{
			LOG_ERROR("[TPSCoreGameMode] PostLogin failed to load character %s", *CharacterId);
			return;
		}

		APlayerController* PC = WeakPC.Get();
		if (!PC)
		{
			LOG_WARNING("[TPSCoreGameMode] PostLogin controller gone before character %s arrived", *CharacterId);
			return;
		}

		// Ownership check: the character's user_email (from DB) must match the email from the JWT.
		// Both fields must be non-empty to enforce the check (graceful degradation if either is absent).
		if (!OwnerEmail.IsEmpty() && !CharacterData.OwnerEmail.IsEmpty() &&
			!CharacterData.OwnerEmail.Equals(OwnerEmail, ESearchCase::IgnoreCase))
		{
			LOG_ERROR("[TPSCoreGameMode] Ownership mismatch for character %s — JWT=%s DB=%s. Kicking player.",
				*CharacterId, *OwnerEmail, *CharacterData.OwnerEmail);

			if (ATPSCoreGameMode* GM = WeakGM.Get())
			{
				if (GM->GameSession)
				{
					GM->GameSession->KickPlayer(PC, FText::FromString(TEXT("Character ownership mismatch")));
				}
			}
			return;
		}

		ATPSCoreMechanicsCharacter* Character = Cast<ATPSCoreMechanicsCharacter>(PC->GetPawn());
		if (!Character)
		{
			LOG_ERROR("[TPSCoreGameMode] Pawn for %s is not ATPSCoreMechanicsCharacter (class: %s)",
				*PC->GetName(),
				PC->GetPawn() ? *PC->GetPawn()->GetClass()->GetName() : TEXT("nullptr"));
			return;
		}

		Character->InitializeFromCharacterData(CharacterData);
		LOG("[TPSCoreGameMode] PostLogin initialized character '%s' (%s)", *CharacterData.Name, *CharacterId);
	});

	Loader->FetchCharacter(CharacterId, LoadedDelegate);
}
