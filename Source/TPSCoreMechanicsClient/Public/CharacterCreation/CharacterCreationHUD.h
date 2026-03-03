// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ToggleWidget.h"
#include "InputBoxWidget.h"
#include "CharacterCreationUI.h"
#include "CommonTypes.h"
#include "CharacterCreationHUD.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSelectorChanged, uint8, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNameChanged, const FString&, NewText);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShowError, const FString&, ErrorMessage);

UCLASS()
class TPSCOREMECHANICSCLIENT_API ACharacterCreationHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character CreationHUD")
	TSubclassOf<UCharacterCreationUI> CharacterCreationUIClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character CreationHUD")
	TSubclassOf<UToggleWidget> RaceSelectorWidgetClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character CreationHUD")
	TSubclassOf<UToggleWidget> GenderSelectorWidgetClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character CreationHUD")
	TSubclassOf<UToggleWidget> ClassSelectorWidgetClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character CreationHUD")
	TSubclassOf<UToggleWidget> SkinColorSelectorWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character CreationHUD")
	TSubclassOf<UInputBoxWidget> NameInputBoxWidgetClass;
	
	void UpdateAvailableRaces(const TArray<FStringByteKVP>& AvailableRaces);

	void UpdateAvailableGenders(const TArray<FStringByteKVP>& AvailableGenders);

	void UpdateAvailableClasses(const TArray<FStringByteKVP>& AvailableClasses);

	void UpdateAvailableSkinColors(const TArray<FStringByteKVP>& AvailableSkinColors);
	
	// Show an error message to the user
	UFUNCTION(BlueprintCallable, Category="Character Creation")
	void ShowError(const FString& ErrorMessage);
	
	UPROPERTY(BlueprintAssignable, Category="Character Creation")
	FOnSelectorChanged OnSelectedRaceChanged;
	
	UPROPERTY(BlueprintAssignable, Category="Character Creation")
	FOnSelectorChanged OnSelectedGenderChanged;
	
	UPROPERTY(BlueprintAssignable, Category="Character Creation")
	FOnSelectorChanged OnSelectedClassChanged;
	
	UPROPERTY(BlueprintAssignable, Category="Character Creation")
	FOnSelectorChanged OnSelectedSkinColorChanged;

	UPROPERTY(BlueprintAssignable, Category="Character Creation")
	FOnNameChanged OnNameChanged;
	
	UPROPERTY(BlueprintAssignable, Category="Character Creation")
	FOnShowError OnShowError;
	
	UPROPERTY()
	UCharacterCreationUI* CharacterCreationUI = nullptr;
	
private:
	
	UPROPERTY()
	UToggleWidget* RaceSelectorWidget = nullptr;
	
	UPROPERTY()
	UToggleWidget* GenderSelectorWidget = nullptr;
	
	UPROPERTY()
	UToggleWidget* ClassSelectorWidget = nullptr;
	
	UPROPERTY()
	UToggleWidget* SkinColorSelectorWidget = nullptr;
	
	UPROPERTY()
	UInputBoxWidget* NameInputBoxWidget = nullptr;
	
	// Handler methods for widget OnChanged events
	UFUNCTION()
	void HandleRaceChanged(FToggleOption Value);
	
	UFUNCTION()
	void HandleGenderChanged(FToggleOption Value);
	
	UFUNCTION()
	void HandleClassChanged(FToggleOption Value);
	
	UFUNCTION()
	void HandleSkinColorChanged(FToggleOption Value);

	UFUNCTION()
	void HandleNameChanged(const FString& NewText);
	
protected:
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;
};


class USelectorWidgetHelpers
{

public:
	static UToggleWidget* InstantiateAndAttach(TSubclassOf<UToggleWidget> ToggleWidgetClass, const FString& Title, const TArray<FToggleOption>& Values, const
	                                           UCharacterCreationUI* CharacterCreationUI, UWorld* World, int32 SortIndex = -1);
};
