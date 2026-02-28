// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterCreation/CharacterCreationHUD.h"

#include "Blueprint/UserWidget.h"
#include "Components/PanelWidget.h"
#include "TPSCoreMechanics/TPSCoreMechanics.h"


void ACharacterCreationHUD::UpdateAvailableRaces(const TArray<FStringByteKVP>& AvailableRaces)
{
	LOG("[CharacterCreationHUD] UpdateAvailableRaces called with %d options", AvailableRaces.Num());
	LOG_DEBUG("[CharacterCreationHUD] UpdateAvailableRaces called with %d options", AvailableRaces.Num());
	
	TArray<FToggleOption> Options;
	for (auto race : AvailableRaces)
	{
		Options.Add({race.Key, race.Value});
		LOG_DEBUG("[CharacterCreationHUD]   Race: %s = %d", *race.Key, race.Value);
	}
	
	if (!RaceSelectorWidget)
	{
		LOG("[CharacterCreationHUD] Creating Race selector widget...");
		RaceSelectorWidget = USelectorWidgetHelpers::InstantiateAndAttach(RaceSelectorWidgetClass, "Race", Options, CharacterCreationUI, GetWorld(), 0);
		if (RaceSelectorWidget)
		{
			LOG("[CharacterCreationHUD] Race selector widget created successfully");
			RaceSelectorWidget->OnChanged.AddUniqueDynamic(this, &ACharacterCreationHUD::HandleRaceChanged);
			// Auto-select first option
			if (Options.Num() > 0)
			{
				RaceSelectorWidget->SetSelectedValue(Options[0].OptionValue);
				HandleRaceChanged(Options[0]);
			}
		}
		else
		{
			LOG("[CharacterCreationHUD] Race selector widget creation FAILED");
		}
	}
	else
	{
		LOG("[CharacterCreationHUD] Race selector already exists, updating options");
		uint8 CurrentRace = RaceSelectorWidget->GetCurrentValue().OptionValue;
		RaceSelectorWidget->SetOptions(Options);
		
		// Validate current selection is still valid
		if (!RaceSelectorWidget->SetSelectedValue(CurrentRace) && Options.Num() > 0)
		{
			LOG("[CharacterCreationHUD] Current race %d is no longer valid, selecting first available", CurrentRace);
			RaceSelectorWidget->SetSelectedValue(Options[0].OptionValue);
			HandleRaceChanged(Options[0]);
		}
	}
}

void ACharacterCreationHUD::UpdateAvailableGenders(const TArray<FStringByteKVP>& AvailableGenders)
{
	LOG("[CharacterCreationHUD] UpdateAvailableGenders called with %d options", AvailableGenders.Num());
	LOG_DEBUG("[CharacterCreationHUD] UpdateAvailableGenders called with %d options", AvailableGenders.Num());
	
	TArray<FToggleOption> Options;
	for (auto gender : AvailableGenders)
	{
		Options.Add({gender.Key, gender.Value});
		LOG_DEBUG("[CharacterCreationHUD]   Gender: %s = %d", *gender.Key, gender.Value);
	}
	
	if (!GenderSelectorWidget)
	{
		LOG("[CharacterCreationHUD] Creating Gender selector widget...");
		GenderSelectorWidget = USelectorWidgetHelpers::InstantiateAndAttach(GenderSelectorWidgetClass, "Gender", Options, CharacterCreationUI, GetWorld(), 1);
		if (GenderSelectorWidget)
		{
			LOG("[CharacterCreationHUD] Gender selector widget created successfully");
			GenderSelectorWidget->OnChanged.AddUniqueDynamic(this, &ACharacterCreationHUD::HandleGenderChanged);
			// Auto-select first option
			if (Options.Num() > 0)
			{
				GenderSelectorWidget->SetSelectedValue(Options[0].OptionValue);
				HandleGenderChanged(Options[0]);
			}
		}
		else
		{
			LOG("[CharacterCreationHUD] Gender selector widget creation FAILED");
		}
	}
	else
	{
		LOG("[CharacterCreationHUD] Gender selector already exists, updating options");
		uint8 CurrentGender = GenderSelectorWidget->GetCurrentValue().OptionValue;
		GenderSelectorWidget->SetOptions(Options);
		
		// Validate current selection is still valid
		if (!GenderSelectorWidget->SetSelectedValue(CurrentGender) && Options.Num() > 0)
		{
			LOG("[CharacterCreationHUD] Current gender %d is no longer valid, selecting first available", CurrentGender);
			GenderSelectorWidget->SetSelectedValue(Options[0].OptionValue);
			HandleGenderChanged(Options[0]);
		}
	}
}

void ACharacterCreationHUD::UpdateAvailableClasses(const TArray<FStringByteKVP>& AvailableClasses)
{
	LOG("[CharacterCreationHUD] UpdateAvailableClasses called with %d options", AvailableClasses.Num());
	LOG_DEBUG("[CharacterCreationHUD] UpdateAvailableClasses called with %d options", AvailableClasses.Num());
	
	TArray<FToggleOption> Options;
	for (auto classOption : AvailableClasses)
	{
		Options.Add({classOption.Key, classOption.Value});
		LOG_DEBUG("[CharacterCreationHUD]   Class: %s = %d", *classOption.Key, classOption.Value);
	}
	
	if (!ClassSelectorWidget)
	{
		LOG("[CharacterCreationHUD] Creating Class selector widget...");
		ClassSelectorWidget = USelectorWidgetHelpers::InstantiateAndAttach(ClassSelectorWidgetClass, "Class", Options, CharacterCreationUI, GetWorld(), 2);
		if (ClassSelectorWidget)
		{
			LOG("[CharacterCreationHUD] Class selector widget created successfully");
			ClassSelectorWidget->OnChanged.AddUniqueDynamic(this, &ACharacterCreationHUD::HandleClassChanged);
			// Auto-select first option
			if (Options.Num() > 0)
			{
				ClassSelectorWidget->SetSelectedValue(Options[0].OptionValue);
				HandleClassChanged(Options[0]);
			}
		}
		else
		{
			LOG("[CharacterCreationHUD] Class selector widget creation FAILED");
		}
	}
	else
	{
		LOG("[CharacterCreationHUD] Class selector already exists, updating options");
		uint8 CurrentClass = ClassSelectorWidget->GetCurrentValue().OptionValue;
		ClassSelectorWidget->SetOptions(Options);
		
		// Validate current selection is still valid
		if (!ClassSelectorWidget->SetSelectedValue(CurrentClass) && Options.Num() > 0)
		{
			LOG("[CharacterCreationHUD] Current class %d is no longer valid, selecting first available", CurrentClass);
			ClassSelectorWidget->SetSelectedValue(Options[0].OptionValue);
			HandleClassChanged(Options[0]);
		}
	}
}

void ACharacterCreationHUD::UpdateAvailableSkinColors(const TArray<FStringByteKVP>& AvailableSkinColors)
{
	LOG("[CharacterCreationHUD] UpdateAvailableSkinColors called with %d options", AvailableSkinColors.Num());
	LOG_DEBUG("[CharacterCreationHUD] UpdateAvailableSkinColors called with %d options", AvailableSkinColors.Num());
	
	TArray<FToggleOption> Options;
	for (auto skinColor : AvailableSkinColors)
	{
		Options.Add({skinColor.Key, skinColor.Value});
		LOG_DEBUG("[CharacterCreationHUD]   SkinColor: %s = %d", *skinColor.Key, skinColor.Value);
	}
	
	if (!SkinColorSelectorWidget)
	{
		LOG("[CharacterCreationHUD] Creating SkinColor selector widget...");
		SkinColorSelectorWidget = USelectorWidgetHelpers::InstantiateAndAttach(SkinColorSelectorWidgetClass, "Skin Color", Options, CharacterCreationUI, GetWorld(), 3);
		if (SkinColorSelectorWidget)
		{
			LOG("[CharacterCreationHUD] SkinColor selector widget created successfully");
			SkinColorSelectorWidget->OnChanged.AddUniqueDynamic(this, &ACharacterCreationHUD::HandleSkinColorChanged);
			// Auto-select first option
			if (Options.Num() > 0)
			{
				SkinColorSelectorWidget->SetSelectedValue(Options[0].OptionValue);
				HandleSkinColorChanged(Options[0]);
			}
		}
		else
		{
			LOG("[CharacterCreationHUD] SkinColor selector widget creation FAILED");
		}
	}
	else
	{
		LOG("[CharacterCreationHUD] SkinColor selector already exists, updating options");
		uint8 CurrentSkinColor = SkinColorSelectorWidget->GetCurrentValue().OptionValue;
		SkinColorSelectorWidget->SetOptions(Options);
		
		// Validate current selection is still valid
		if (!SkinColorSelectorWidget->SetSelectedValue(CurrentSkinColor) && Options.Num() > 0)
		{
			LOG("[CharacterCreationHUD] Current skin color %d is no longer valid, selecting first available", CurrentSkinColor);
			SkinColorSelectorWidget->SetSelectedValue(Options[0].OptionValue);
			HandleSkinColorChanged(Options[0]);
		}
	}
}

void ACharacterCreationHUD::HandleRaceChanged(FToggleOption Value)
{
	OnSelectedRaceChanged.Broadcast(Value.OptionValue);
}

void ACharacterCreationHUD::HandleGenderChanged(FToggleOption Value)
{
	OnSelectedGenderChanged.Broadcast(Value.OptionValue);
}

void ACharacterCreationHUD::HandleClassChanged(FToggleOption Value)
{
	OnSelectedClassChanged.Broadcast(Value.OptionValue);
}

void ACharacterCreationHUD::HandleSkinColorChanged(FToggleOption Value)
{
	OnSelectedSkinColorChanged.Broadcast(Value.OptionValue);
}

void ACharacterCreationHUD::HandleNameChanged(const FString& NewText)
{
	OnNameChanged.Broadcast(NewText);
}

void ACharacterCreationHUD::ShowError(const FString& ErrorMessage)
{
	LOG("[CharacterCreationHUD] Error: %s", *ErrorMessage);
	OnShowError.Broadcast(ErrorMessage);
}

void ACharacterCreationHUD::BeginPlay()
{
	LOG("BeginPlay called on HUD: %s", *GetClass()->GetName());
	
	// Ensure all selector widgets start as null
	RaceSelectorWidget = nullptr;
	GenderSelectorWidget = nullptr;
	ClassSelectorWidget = nullptr;
	SkinColorSelectorWidget = nullptr;
	NameInputBoxWidget = nullptr;
	
	if (CharacterCreationUIClass)
	{
		LOG("CharacterCreationUIClass is set to:%s", *CharacterCreationUIClass->GetName());
		CharacterCreationUI = CreateWidget<UCharacterCreationUI>(GetWorld(), CharacterCreationUIClass);
		if (CharacterCreationUI)
		{
			CharacterCreationUI->AddToViewport();
			auto InputBoxContainer = CharacterCreationUI->GetInputBoxContainer();

			if (InputBoxContainer)
			{
				UInputBoxWidget* NameInputBox = CreateWidget<UInputBoxWidget>(GetWorld(), NameInputBoxWidgetClass);
				if (NameInputBox)				{
					NameInputBox->SetPlaceholder("Enter character name");
					NameInputBox->SetMaxLength(20);
					
					// Bind validation function (using BindUFunction for static UFUNCTION)
					NameInputBox->OnValidateText.BindUFunction(NameInputBox, FName("ValidateCharacterName"));
					
					InputBoxContainer->AddChild(NameInputBox);
					NameInputBoxWidget = NameInputBox;
					NameInputBoxWidget->OnTextChanged.AddUniqueDynamic(this, &ACharacterCreationHUD::HandleNameChanged);
				}
				else
				{
					LOG("Failed to create NameInputBoxWidget");
				}
			}
			else
			{
				LOG("InputBoxContainer is null in UI");
			}
			LOG("Added UI to HUD");
		}
	}
	else
	{
		LOG("Cannot add UI because NONE was set.")
	}
}

void ACharacterCreationHUD::BeginDestroy()
{
	// Clean up UI widget
	if (CharacterCreationUI)
	{
		CharacterCreationUI->RemoveFromParent();
		CharacterCreationUI = nullptr;
	}
	
	// Unbind widget event handlers
	if (RaceSelectorWidget)
	{
		RaceSelectorWidget->OnChanged.RemoveDynamic(this, &ACharacterCreationHUD::HandleRaceChanged);
	}
	
	if (GenderSelectorWidget)
	{
		GenderSelectorWidget->OnChanged.RemoveDynamic(this, &ACharacterCreationHUD::HandleGenderChanged);
	}
	
	if (ClassSelectorWidget)
	{
		ClassSelectorWidget->OnChanged.RemoveDynamic(this, &ACharacterCreationHUD::HandleClassChanged);
	}
	
	if (SkinColorSelectorWidget)
	{
		SkinColorSelectorWidget->OnChanged.RemoveDynamic(this, &ACharacterCreationHUD::HandleSkinColorChanged);
	}
	
	Super::BeginDestroy();
}

UToggleWidget* USelectorWidgetHelpers::InstantiateAndAttach(const TSubclassOf<UToggleWidget> ToggleWidgetClass, const FString& Title, const TArray<FToggleOption>& Values, const UCharacterCreationUI* CharacterCreationUI, UWorld* World, int32 SortIndex)
{
	if (!ToggleWidgetClass)
	{
		LOG("[CharacterCreationHUD] ToggleWidgetClass is None for '%s'", *Title);
		return nullptr;
	}
	
	if (!CharacterCreationUI)
	{
		LOG("[CharacterCreationHUD] CharacterCreationUI is null for '%s'", *Title);
		return nullptr;
	}
	
	if (UToggleWidget* Widget = CreateWidget<UToggleWidget>(World, ToggleWidgetClass))
	{
		UPanelWidget* selectorContainer = CharacterCreationUI->GetSelectorContainer();
		if (!selectorContainer)
		{
			LOG("[CharacterCreationHUD] SelectorContainer is null for '%s'", *Title);
			return nullptr;
		}
		
		Widget->SetTitle(Title);
		Widget->SetOptions(Values);
		
		// Insert at specific index to maintain dependency order (Race=0, Gender=1, Class=2, SkinColor=3)
		if (SortIndex >= 0 && SortIndex < selectorContainer->GetChildrenCount())
		{
			selectorContainer->InsertChildAt(SortIndex, Widget);
		}
		else
		{
			selectorContainer->AddChild(Widget);
		}
		
		return Widget;
	}
	
	LOG("[CharacterCreationHUD] Failed to create widget for '%s'", *Title);
	return nullptr;
}
