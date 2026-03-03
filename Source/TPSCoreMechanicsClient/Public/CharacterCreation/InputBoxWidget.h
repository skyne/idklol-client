// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputBoxWidget.generated.h"

USTRUCT(BlueprintType)
struct FValidationResult
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, Category = "Validation")
	bool bIsValid = true;
	
	UPROPERTY(BlueprintReadWrite, Category = "Validation")
	FString ErrorMessage;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInputTextChanged, const FString&, NewText);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInputTextCommitted, const FString&, CommittedText);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(FValidationResult, FOnValidateText, const FString&, TextToValidate);

UCLASS()
class TPSCOREMECHANICSCLIENT_API UInputBoxWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Input Box Widget")
	void SetTitle(FString title);
	
	UFUNCTION(BlueprintCallable, Category = "Input Box Widget")
	void SetText(const FString& text);
	
	UFUNCTION(BlueprintCallable, Category = "Input Box Widget")
	FString GetText() const;
	
	UFUNCTION(BlueprintCallable, Category = "Input Box Widget")
	void SetPlaceholder(const FString& placeholder);
	
	UFUNCTION(BlueprintCallable, Category = "Input Box Widget")
	void SetMaxLength(int32 maxLength);
	
	UFUNCTION(BlueprintCallable, Category = "Input Box Widget")
	bool Validate();
	
	// Static validation method for character names
	// Rules: 3-20 characters, alphanumeric + spaces/hyphens, no leading/trailing spaces, no consecutive spaces
	UFUNCTION(BlueprintCallable, Category = "Input Box Widget")
	static FValidationResult ValidateCharacterName(const FString& Name);
	
	UPROPERTY(BlueprintAssignable, Category = "Input Box Widget")
	FOnInputTextChanged OnTextChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "Input Box Widget")
	FOnInputTextCommitted OnTextCommitted;
	
	UPROPERTY()
	FOnValidateText OnValidateText;
	
protected:
	UPROPERTY(BlueprintReadOnly, Category = "Input Box Widget")
	FString Title;
	
	UPROPERTY(BlueprintReadOnly, Category = "Input Box Widget")
	FString CurrentText;
	
	UPROPERTY(BlueprintReadOnly, Category = "Input Box Widget")
	FString PlaceholderText;
	
	UPROPERTY(BlueprintReadOnly, Category = "Input Box Widget")
	int32 MaxLength = 0; // 0 means no limit
	
	UPROPERTY(BlueprintReadOnly, Category = "Input Box Widget")
	bool bIsValid = true;
	
	UPROPERTY(BlueprintReadOnly, Category = "Input Box Widget")
	FString ErrorMessage;
	
	UFUNCTION(BlueprintCallable, Category = "Input Box Widget")
	void TriggerOnTextChanged(const FString& NewText);
	
	UFUNCTION(BlueprintCallable, Category = "Input Box Widget")
	void TriggerOnTextCommitted(const FString& CommittedText);
};
