// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterCreation/ToggleWidget.h"

void UToggleWidget::SetTitle(FString title)
{
	Title = title;
}

void UToggleWidget::SetOptions(TArray<FToggleOption> option)
{
	Options = option;
	selectedIndex = 0;
	TriggerOnChanged();
}

void UToggleWidget::TriggerOnNextOptionSelected()
{
	UE_LOG(LogTemp, Log, TEXT("[ToggleWidget] TriggerOnNextOptionSelected called - currentIndex: %d, Options.Num: %d"), selectedIndex, Options.Num());
	
	if (Options.Num() == 0)
	{
		return;
	}
	
	if (selectedIndex == Options.Num() -1)
	{
		if (Mode == FToggleOptionMode::LoopAround)
		{
			selectedIndex = 0;
			UE_LOG(LogTemp, Log, TEXT("[ToggleWidget] At end, looping to 0"));
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[ToggleWidget] At end, staying at %d (TwoEnded mode)"), selectedIndex);
		}
	}
	else
	{
		selectedIndex++;
		UE_LOG(LogTemp, Log, TEXT("[ToggleWidget] Incremented to %d"), selectedIndex);
	}
	OnNextOptionSelected.Broadcast();
	TriggerOnChanged();
}

void UToggleWidget::TriggerOnPreviousOptionSelected()
{
	UE_LOG(LogTemp, Log, TEXT("[ToggleWidget] TriggerOnPreviousOptionSelected called - currentIndex: %d, Options.Num: %d"), selectedIndex, Options.Num());
	
	if (Options.Num() == 0)
	{
		return;
	}
	
	if (selectedIndex == 0 )
	{
		if (Mode == FToggleOptionMode::LoopAround)
		{
			selectedIndex = Options.Num() - 1;
			UE_LOG(LogTemp, Log, TEXT("[ToggleWidget] At start, looping to %d"), selectedIndex);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[ToggleWidget] At start, staying at 0 (TwoEnded mode)"));
		}
	}
	else
	{
		selectedIndex--;
		UE_LOG(LogTemp, Log, TEXT("[ToggleWidget] Decremented to %d"), selectedIndex);
	}
	OnPreviousOptionSelected.Broadcast();
	TriggerOnChanged();
}

void UToggleWidget::TriggerOnChanged(const FToggleOption& value)
{
	OnChanged.Broadcast(value);
}

void UToggleWidget::TriggerOnChanged()
{
	if (Options.Num() == 0)
	{
		ValueLabel = FString();
		return;
	}
	
	const FToggleOption selectedValue = Options[selectedIndex];
	ValueLabel = selectedValue.OptionName;
	CurrentValue = selectedValue;
	UE_LOG(LogTemp, Log, TEXT("[ToggleWidget] Changed to: %s (value: %d) at index %d"), *selectedValue.OptionName, selectedValue.OptionValue, selectedIndex);
	TriggerOnChanged(selectedValue);
}

FToggleOption UToggleWidget::GetCurrentValue() const
{
	return CurrentValue;
}

bool UToggleWidget::SetSelectedValue(uint8 Value)
{
	for (int32 i = 0; i < Options.Num(); i++)
	{
		if (Options[i].OptionValue == Value)
		{
			selectedIndex = i;
			CurrentValue = Options[i];
			ValueLabel = CurrentValue.OptionName;
			return true;
		}
	}
	return false;
}
