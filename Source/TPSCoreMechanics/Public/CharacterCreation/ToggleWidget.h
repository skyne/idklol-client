// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ToggleWidget.generated.h"

UENUM(BlueprintType)
enum  class FToggleOptionMode : uint8
{
	LoopAround UMETA(DisplayName = "LoopAround"),
	TwoEnded UMETA(DisplayName = "TwoEnded"),
};

USTRUCT(Blueprintable, BlueprintType)
struct FToggleOption
{
	GENERATED_BODY()
	
	FString OptionName;
	uint8 OptionValue;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPreviousOptionSelected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNextOptionSelected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChanged, FToggleOption, Value);

UCLASS()
class TPSCOREMECHANICS_API UToggleWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Toggle Widget")
	void SetTitle(FString title);
	
	UFUNCTION(BlueprintCallable, Category = "Toggle Widget")
	void SetOptions(TArray<FToggleOption> option);
	
	UFUNCTION(BlueprintCallable, Category = "Toggle Widget")
	FToggleOption GetCurrentValue() const;
	
	UFUNCTION(BlueprintCallable, Category = "Toggle Widget")
	bool SetSelectedValue(uint8 Value);

	UPROPERTY(BlueprintAssignable, Category = "Toggle Widget")
	FOnNextOptionSelected OnNextOptionSelected;
	
	UPROPERTY(BlueprintAssignable, Category = "Toggle Widget")
	FOnPreviousOptionSelected OnPreviousOptionSelected;
	
	UPROPERTY(BlueprintAssignable, Category = "Toggle Widget")
	FOnChanged OnChanged;
	
protected:
	
	UPROPERTY(BlueprintReadOnly, Category = "Toggle Widget")
	TArray<FToggleOption> Options;
	
	UPROPERTY(BlueprintReadOnly, Category = "Toggle Widget")
	FToggleOptionMode Mode = FToggleOptionMode::LoopAround;
	
	UPROPERTY(BlueprintReadOnly, Category = "Toggle Widget")
	FToggleOption CurrentValue;
	
	UPROPERTY(BlueprintReadOnly, Category = "Toggle Widget")
	FString Title;

	UPROPERTY(BlueprintReadOnly, Category = "Toggle Widget")
	FString ValueLabel;
	
	UFUNCTION(BlueprintCallable, Category = "Toggle Widget")
	void TriggerOnNextOptionSelected();
	UFUNCTION(BlueprintCallable, Category = "Toggle Widget")
	void TriggerOnPreviousOptionSelected();
	UFUNCTION(BlueprintCallable, Category = "Toggle Widget")
	void TriggerOnChanged(const FToggleOption& value);

private:
	
	UPROPERTY()
	int selectedIndex = 0;
	
	void TriggerOnChanged();
};
