// Copyright Epic Games, Inc. All Rights Reserved.

#include "TPSCoreMechanicsCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Data/CharacterClassInfo.h"
#include "Libraries/TPSCoreAbilitySystemLibrary.h"
#include "Public/Game/PlayerState/TPSCorePlayerState.h"
#include "Public/AbilitySystem/TPSCoreAbilitySystemComponent.h"
#include "Public/AbilitySystem/Attributes/TPSCoreAttributeSet.h"
#include "Helpers/CharacterAppearanceHelper.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "CharacterCreation/SelectedCharacterSubsystem.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

ATPSCoreMechanicsCharacter::ATPSCoreMechanicsCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void ATPSCoreMechanicsCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// Check if there's a selected character to apply appearance
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USelectedCharacterSubsystem* SelectedCharacterSubsystem = GameInstance->GetSubsystem<USelectedCharacterSubsystem>())
		{
			if (SelectedCharacterSubsystem->ShouldApplyAppearance())
			{
				FCharacterData SelectedCharacter = SelectedCharacterSubsystem->GetSelectedCharacter();
				UE_LOG(LogTemplateCharacter, Log, TEXT("Applying appearance for character: %s"), *SelectedCharacter.Name);
				
				InitializeFromCharacterData(SelectedCharacter);
				
				// Mark appearance as applied (but keep character data for other systems like chat)
				SelectedCharacterSubsystem->MarkAppearanceApplied();
			}
		}
	}
}

void ATPSCoreMechanicsCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (HasAuthority())
	{
		InitAbilityActorInfo();
	}
}

void ATPSCoreMechanicsCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	InitAbilityActorInfo();
}

UAbilitySystemComponent* ATPSCoreMechanicsCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ATPSCoreMechanicsCharacter::InitAbilityActorInfo()
{
	if (ATPSCorePlayerState* TPSPlayerState = GetPlayerState<ATPSCorePlayerState>())
	{
		AbilitySystemComponent = TPSPlayerState->GetTPSCoreAbilitySystemComponent();
		Attributes = TPSPlayerState->GetAttributes();

		if (IsValid(AbilitySystemComponent))
		{
			AbilitySystemComponent->InitAbilityActorInfo(TPSPlayerState, this);

			BindCallbacksToDependencies();

			if (HasAuthority())
			{
				InitClassDefaults();
			}
		}
	}
}

void ATPSCoreMechanicsCharacter::InitClassDefaults()
{
	if (!CharacterTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("No character tag selected for this character: %s"), *GetNameSafe(this));
	}
	else if (const UCharacterClassInfo* ClassInfo = UTPSCoreAbilitySystemLibrary::GetCharacterClassDefaultInfo(this))
	{
		if (const FCharacterClassDefaultInfo* SelectedClassInfo = ClassInfo->ClassDefaultInfoMap.Find(CharacterTag))
		{
			if (IsValid(AbilitySystemComponent))
			{
				AbilitySystemComponent->AddCharacterAbilities(SelectedClassInfo->StartingAbilities);
				AbilitySystemComponent->AddCharacterPassiveAbilities(SelectedClassInfo->StartingPassives);
				AbilitySystemComponent->InitializeDefaultAttributes(SelectedClassInfo->DefaultAttributes);
			}
		}
	}
}

void ATPSCoreMechanicsCharacter::BindCallbacksToDependencies()
{
	if (IsValid(AbilitySystemComponent) && IsValid(Attributes))
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Attributes->GetHealthAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnHealthChanged(Data.NewValue, Attributes->GetMaxHealth());
			}
		);

		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Attributes->GetManaAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnManaChanged(Data.NewValue, Attributes->GetMaxMana());
			}
		);
	}
}

void ATPSCoreMechanicsCharacter::BroadcastInitialValues()
{
	if (IsValid(Attributes))
	{
		OnHealthChanged(Attributes->GetHealth(), Attributes->GetMaxHealth());
		OnManaChanged(Attributes->GetMana(), Attributes->GetMaxMana());
	}
}

void ATPSCoreMechanicsCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	if (const APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ATPSCoreMechanicsCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this,
		                                   &ATPSCoreMechanicsCharacter::Move);

		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this,
		                                   &ATPSCoreMechanicsCharacter::Look);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error,
		       TEXT(
			       "'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."
		       ), *GetNameSafe(this));
	}
}

void ATPSCoreMechanicsCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ATPSCoreMechanicsCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ATPSCoreMechanicsCharacter::InitializeFromCharacterData(const FCharacterData& CharacterData)
{
	ApplyCharacterAppearance(CharacterData.Gender, CharacterData.SkinColor);
}

void ATPSCoreMechanicsCharacter::ApplyCharacterAppearance(ECharacterGender Gender, ECharacterSkinColor SkinColor)
{
	TSoftObjectPtr<USkeletalMesh> TargetMeshPtr = UCharacterAppearanceHelper::GetMeshForGender(Gender, MaleMesh, FemaleMesh);
	
	if (TargetMeshPtr.IsNull())
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Target mesh is null for gender %d"), static_cast<int32>(Gender));
		
		// Still apply skin color to current mesh if available
		if (GetMesh())
		{
			UCharacterAppearanceHelper::ApplySkinColor(SkinColor, GetMesh());
		}
		return;
	}
	
	// Get the global Streamable Manager
	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
	
	// Create a delegate that points to our callback function
	FStreamableDelegate Delegate = FStreamableDelegate::CreateUObject(
		this, 
		&ATPSCoreMechanicsCharacter::OnCharacterMeshLoaded, 
		TargetMeshPtr,
		SkinColor
	);
	
	// Start the asynchronous load
	Streamable.RequestAsyncLoad(TargetMeshPtr.ToSoftObjectPath(), Delegate);
}

void ATPSCoreMechanicsCharacter::OnCharacterMeshLoaded(TSoftObjectPtr<USkeletalMesh> LoadedMeshPtr, ECharacterSkinColor SkinColor)
{
	USkeletalMesh* NewMesh = LoadedMeshPtr.Get();
	
	if (NewMesh && GetMesh())
	{
		GetMesh()->SetSkeletalMesh(NewMesh);
		UCharacterAppearanceHelper::ApplySkinColor(SkinColor, GetMesh());
		
		UE_LOG(LogTemplateCharacter, Log, TEXT("Character mesh loaded and appearance applied"));
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Failed to load character mesh or GetMesh() is null"));
	}
}
