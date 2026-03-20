// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/InventoryInterface.h"
#include "TPSCorePlayerController.generated.h"

class UInventoryComponent;
class ANPCCharacter;
class UNpcInteractionPromptWidget;


UCLASS()
class TPSCOREMECHANICS_API ATPSCorePlayerController : public APlayerController, public IAbilitySystemInterface,
                                                      public IInventoryInterface
{
	GENERATED_BODY()

public:
	ATPSCorePlayerController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupInputComponent() override;

	virtual UInventoryComponent* GetInventoryComponent_Implementation() override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UObject* GetInventoryWidgetController();

	UFUNCTION(BlueprintCallable)
	void CreateInventoryWidget();

	UFUNCTION(BlueprintCallable, Category = "NPC|Interaction")
	void SetNpcPromptEnabled(bool bEnabled);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true), Replicated)
	TObjectPtr<UInventoryComponent> InventoryComponent;

	UPROPERTY()
	TObjectPtr<UObject> InventoryWidgetController;

	UPROPERTY(EditDefaultsOnly, Category="Custom Values|Widgets")
	TSubclassOf<UObject> InventoryWidgetControllerClass;

	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess = true))
	TObjectPtr<UUserWidget> InventoryWidget;

	UPROPERTY(EditDefaultsOnly, Category="Custom Values|Widgets")
	TSubclassOf<UUserWidget> InventoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "NPC|Interaction")
	bool bNpcPromptEnabled = true;

	UPROPERTY(EditDefaultsOnly, Category = "NPC|Interaction", meta = (ClampMin = "0.02", UIMin = "0.02"))
	float NpcPromptSearchInterval = 0.10f;

	UPROPERTY(EditDefaultsOnly, Category = "NPC|Interaction")
	float NpcPromptVerticalWorldOffset = 110.f;

	UPROPERTY(EditDefaultsOnly, Category = "NPC|Interaction")
	FText NpcPromptLabel = FText::FromString(TEXT("Interact"));

	UPROPERTY(EditDefaultsOnly, Category = "NPC|Interaction")
	TSubclassOf<UNpcInteractionPromptWidget> NpcInteractionPromptWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UNpcInteractionPromptWidget> NpcInteractionPromptWidget;

	UPROPERTY(Transient)
	TWeakObjectPtr<ANPCCharacter> ActiveNearbyNpc;

	float NpcPromptSearchElapsed = 0.f;

	void TickNpcPrompt(float DeltaSeconds);
	void HandleNpcInteractInput();
	void EnsureNpcPromptWidget();
	void HideNpcPromptWidget();
	void UpdateNpcPromptWidgetFor(ANPCCharacter* Npc);
	ANPCCharacter* FindNearestNpcInRange(const FVector& PlayerLocation) const;
	static float GetInteractionRadiusForNpc(const ANPCCharacter* Npc);
};
