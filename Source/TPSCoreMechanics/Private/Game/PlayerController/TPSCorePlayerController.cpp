// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/PlayerController/TPSCorePlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Inventory/InventoryComponent.h"
#include "NPC/NPCCharacter.h"
#include "UI/NpcInteractionPromptWidget.h"
#include "UI/NpcInteractionResultWidget.h"
#include "EngineUtils.h"
#include "InputCoreTypes.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogTPSCorePlayerController, Log, All);

ATPSCorePlayerController::ATPSCorePlayerController()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>("InventoryComponent");
	InventoryComponent->SetIsReplicated(true);

	NpcInteractionPromptWidgetClass = UNpcInteractionPromptWidget::StaticClass();
}

void ATPSCorePlayerController::BeginPlay()
{
	Super::BeginPlay();

	NpcPromptSearchElapsed = NpcPromptSearchInterval;
}

void ATPSCorePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	HideNpcPromptWidget();
	CloseActiveNpcInteractionWidget();
	ActiveNearbyNpc.Reset();

	Super::EndPlay(EndPlayReason);
}

void ATPSCorePlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TickNpcPrompt(DeltaSeconds);
}

void ATPSCorePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::E, IE_Pressed, this, &ATPSCorePlayerController::HandleNpcInteractInput);
		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ATPSCorePlayerController::HandleNpcInteractionCloseInput);
	}
}

UInventoryComponent* ATPSCorePlayerController::GetInventoryComponent_Implementation()
{
	return InventoryComponent;
}

UAbilitySystemComponent* ATPSCorePlayerController::GetAbilitySystemComponent() const
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn());
}

void ATPSCorePlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATPSCorePlayerController, InventoryComponent);
}

UObject* ATPSCorePlayerController::GetInventoryWidgetController()
{
	if (!IsValid(InventoryWidgetController))
	{
		InventoryWidgetController = NewObject<UObject>(this, InventoryWidgetControllerClass);

		if (UFunction* SetOwningActorFn = InventoryWidgetController->FindFunction(TEXT("SetOwningActor")))
		{
			struct FSetOwningActorParams
			{
				AActor* InOwningActor;
			};
			FSetOwningActorParams Params{ this };
			InventoryWidgetController->ProcessEvent(SetOwningActorFn, &Params);
		}

		if (UFunction* BindCallbacksFn = InventoryWidgetController->FindFunction(TEXT("BindCallbacksToDependencies")))
		{
			InventoryWidgetController->ProcessEvent(BindCallbacksFn, nullptr);
		}
	}
	return InventoryWidgetController;
}

void ATPSCorePlayerController::CreateInventoryWidget()
{
	if (UUserWidget* Widget = CreateWidget<UUserWidget>(this, InventoryWidgetClass))
	{
		InventoryWidget = Widget;

		if (UFunction* SetWidgetControllerFn = InventoryWidget->FindFunction(TEXT("SetWidgetController")))
		{
			struct FSetWidgetControllerParams
			{
				UObject* InWidgetController;
			};
			FSetWidgetControllerParams Params{ GetInventoryWidgetController() };
			InventoryWidget->ProcessEvent(SetWidgetControllerFn, &Params);
		}

		if (IsValid(InventoryWidgetController))
		{
			if (UFunction* BroadcastInitialValuesFn = InventoryWidgetController->FindFunction(TEXT("BradcastInitialValues")))
			{
				InventoryWidgetController->ProcessEvent(BroadcastInitialValuesFn, nullptr);
			}
		}

		InventoryWidget->AddToViewport();
	}
}

void ATPSCorePlayerController::SetNpcPromptEnabled(bool bEnabled)
{
	bNpcPromptEnabled = bEnabled;
	if (!bNpcPromptEnabled)
	{
		HideNpcPromptWidget();
		ActiveNearbyNpc.Reset();
	}
}

void ATPSCorePlayerController::TickNpcPrompt(float DeltaSeconds)
{
	if (!bNpcPromptEnabled || !IsLocalController())
	{
		HideNpcPromptWidget();
		ActiveNearbyNpc.Reset();
		return;
	}

	APawn* ControlledPawn = GetPawn();
	if (!IsValid(ControlledPawn) || !IsValid(GetWorld()))
	{
		HideNpcPromptWidget();
		ActiveNearbyNpc.Reset();
		return;
	}

	NpcPromptSearchElapsed += DeltaSeconds;
	if (NpcPromptSearchElapsed >= NpcPromptSearchInterval)
	{
		NpcPromptSearchElapsed = 0.f;
		ActiveNearbyNpc = FindNearestNpcInRange(ControlledPawn->GetActorLocation());
	}

	if (!ActiveNearbyNpc.IsValid())
	{
		HideNpcPromptWidget();
		return;
	}

	UpdateNpcPromptWidgetFor(ActiveNearbyNpc.Get());
}

void ATPSCorePlayerController::HandleNpcInteractInput()
{
	if (!bNpcPromptEnabled || !IsLocalController())
	{
		return;
	}

	if (IsValid(ActiveNpcInteractionWidget) && ActiveNpcInteractionWidget->GetVisibility() != ESlateVisibility::Collapsed)
	{
		CloseActiveNpcInteractionWidget();
		return;
	}

	APawn* ControlledPawn = GetPawn();
	ANPCCharacter* Npc = ActiveNearbyNpc.Get();
	if (!IsValid(ControlledPawn) || !IsValid(Npc))
	{
		return;
	}

	const float AllowedRadius = GetInteractionRadiusForNpc(Npc);
	const float Distance = FVector::Distance(ControlledPawn->GetActorLocation(), Npc->GetActorLocation());
	if (Distance > AllowedRadius)
	{
		return;
	}

	const FNpcReplicatedData& NpcData = Npc->GetNpcData();
	UE_LOG(LogTPSCorePlayerController, Log,
		TEXT("NPC interaction happened: player='%s' npc_id='%s' npc_name='%s' distance=%.1f"),
		*GetNameSafe(ControlledPawn),
		*NpcData.NpcId,
		*NpcData.DisplayName,
		Distance);

	ServerInteractWithNpc(Npc);
}

void ATPSCorePlayerController::HandleNpcInteractionCloseInput()
{
	CloseActiveNpcInteractionWidget();
}

void ATPSCorePlayerController::ServerInteractWithNpc_Implementation(ANPCCharacter* Npc)
{
	if (!IsValid(Npc))
	{
		return;
	}

	APawn* ControlledPawn = GetPawn();
	if (!IsValid(ControlledPawn))
	{
		return;
	}

	const float AllowedRadius = GetInteractionRadiusForNpc(Npc);
	const float Distance = FVector::Distance(ControlledPawn->GetActorLocation(), Npc->GetActorLocation());
	if (Distance > AllowedRadius)
	{
		return;
	}

	const FNpcReplicatedData& NpcData = Npc->GetNpcData();
	ClientHandleNpcInteraction(NpcData.NpcId, NpcData.DisplayName, NpcData.Role);
}

void ATPSCorePlayerController::ClientHandleNpcInteraction_Implementation(
	const FString& NpcId,
	const FString& NpcName,
	const FString& NpcRole)
{
	if (IsValid(ActiveNpcInteractionWidget) && ActiveNpcInteractionWidget->GetVisibility() != ESlateVisibility::Collapsed)
	{
		return;
	}

	const FText Message = BuildNpcInteractionMessage(NpcName, NpcRole);
	ShowNpcInteractionWidget(NpcId, NpcName, NpcRole, Message);
}

void ATPSCorePlayerController::EnsureNpcPromptWidget()
{
	if (IsValid(NpcInteractionPromptWidget) || !NpcInteractionPromptWidgetClass)
	{
		return;
	}

	if (UNpcInteractionPromptWidget* PromptWidget = CreateWidget<UNpcInteractionPromptWidget>(this, NpcInteractionPromptWidgetClass))
	{
		NpcInteractionPromptWidget = PromptWidget;
		NpcInteractionPromptWidget->SetPromptText(NpcPromptLabel);
		NpcInteractionPromptWidget->SetVisibility(ESlateVisibility::Collapsed);
		NpcInteractionPromptWidget->AddToViewport();
	}
}

void ATPSCorePlayerController::HideNpcPromptWidget()
{
	if (IsValid(NpcInteractionPromptWidget))
	{
		NpcInteractionPromptWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ATPSCorePlayerController::CloseActiveNpcInteractionWidget()
{
	if (!IsValid(ActiveNpcInteractionWidget))
	{
		return;
	}

	ActiveNpcInteractionWidget->RemoveFromParent();
	ActiveNpcInteractionWidget = nullptr;
}

void ATPSCorePlayerController::UpdateNpcPromptWidgetFor(ANPCCharacter* Npc)
{
	if (!IsValid(Npc))
	{
		HideNpcPromptWidget();
		return;
	}

	APawn* ControlledPawn = GetPawn();
	if (!IsValid(ControlledPawn))
	{
		HideNpcPromptWidget();
		return;
	}

	FVector2D ScreenPosition;
	const FVector WorldAnchor = ControlledPawn->GetActorLocation() + FVector(0.f, 0.f, NpcPromptVerticalWorldOffset);
	if (!ProjectWorldLocationToScreen(WorldAnchor, ScreenPosition, true))
	{
		HideNpcPromptWidget();
		return;
	}

	ScreenPosition += NpcPromptPlayerScreenOffset;

	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	GetViewportSize(ViewportWidth, ViewportHeight);
	if (ViewportWidth > 0 && ViewportHeight > 0)
	{
		ScreenPosition.X = FMath::Clamp(ScreenPosition.X, NpcPromptScreenEdgePadding,
			static_cast<float>(ViewportWidth) - NpcPromptScreenEdgePadding);
		ScreenPosition.Y = FMath::Clamp(ScreenPosition.Y, NpcPromptScreenEdgePadding,
			static_cast<float>(ViewportHeight) - NpcPromptScreenEdgePadding);
	}

	EnsureNpcPromptWidget();
	if (!IsValid(NpcInteractionPromptWidget))
	{
		return;
	}

	NpcInteractionPromptWidget->SetPromptText(NpcPromptLabel);
	NpcInteractionPromptWidget->SetPromptScreenPosition(ScreenPosition);
	NpcInteractionPromptWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void ATPSCorePlayerController::ShowNpcInteractionWidget(
	const FString& NpcId,
	const FString& NpcName,
	const FString& NpcRole,
	const FText& Message)
{
	if (IsValid(ActiveNpcInteractionWidget))
	{
		ActiveNpcInteractionWidget->RemoveFromParent();
		ActiveNpcInteractionWidget = nullptr;
	}

	TSubclassOf<UUserWidget> WidgetClass;
	if (NpcRole.Equals(TEXT("merchant"), ESearchCase::IgnoreCase) && NpcMerchantWidgetClass)
	{
		WidgetClass = NpcMerchantWidgetClass;
	}
	else if (NpcDialogueWidgetClass)
	{
		WidgetClass = NpcDialogueWidgetClass;
	}
	else
	{
		WidgetClass = UNpcInteractionResultWidget::StaticClass();
	}

	if (!WidgetClass)
	{
		return;
	}

	UUserWidget* Widget = CreateWidget<UUserWidget>(this, WidgetClass);
	if (!IsValid(Widget))
	{
		return;
	}

	if (UNpcInteractionResultWidget* ResultWidget = Cast<UNpcInteractionResultWidget>(Widget))
	{
		ResultWidget->SetContext(FText::FromString(NpcName), Message);
	}
	else if (UFunction* SetContextFunction = Widget->FindFunction(TEXT("SetNpcInteractionContext")))
	{
		struct FNpcInteractionContextParams
		{
			FString InNpcId;
			FString InNpcName;
			FString InNpcRole;
			FText InMessage;
		};

		FNpcInteractionContextParams Params;
		Params.InNpcId = NpcId;
		Params.InNpcName = NpcName;
		Params.InNpcRole = NpcRole;
		Params.InMessage = Message;
		Widget->ProcessEvent(SetContextFunction, &Params);
	}

	Widget->AddToViewport(30);
	Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
	ActiveNpcInteractionWidget = Widget;
}

FText ATPSCorePlayerController::BuildNpcInteractionMessage(const FString& NpcName, const FString& NpcRole)
{
	if (NpcRole.Equals(TEXT("merchant"), ESearchCase::IgnoreCase))
	{
		return FText::FromString(FString::Printf(TEXT("%s opens their wares."), *NpcName));
	}

	if (!NpcRole.IsEmpty())
	{
		return FText::FromString(FString::Printf(TEXT("%s (%s): Greetings, traveler."), *NpcName, *NpcRole));
	}

	return FText::FromString(FString::Printf(TEXT("%s: Greetings, traveler."), *NpcName));
}

ANPCCharacter* ATPSCorePlayerController::FindNearestNpcInRange(const FVector& PlayerLocation) const
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	ANPCCharacter* BestNpc = nullptr;
	float BestDistanceSq = TNumericLimits<float>::Max();

	for (TActorIterator<ANPCCharacter> It(World); It; ++It)
	{
		ANPCCharacter* Candidate = *It;
		if (!IsValid(Candidate))
		{
			continue;
		}

		const float InteractionRadius = GetInteractionRadiusForNpc(Candidate);
		if (InteractionRadius <= 0.f)
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared(PlayerLocation, Candidate->GetActorLocation());
		if (DistanceSq <= FMath::Square(InteractionRadius) && DistanceSq < BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			BestNpc = Candidate;
		}
	}

	return BestNpc;
}

float ATPSCorePlayerController::GetInteractionRadiusForNpc(const ANPCCharacter* Npc)
{
	if (!IsValid(Npc))
	{
		return 0.f;
	}

	const FNpcReplicatedData& NpcData = Npc->GetNpcData();
	return FMath::Max(50.f, NpcData.InteractionRadius);
}
