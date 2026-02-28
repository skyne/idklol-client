// Fill out your copyright notice in the Description page of Project Settings.

#include "CharacterCreation/InputBoxWidget.h"

void UInputBoxWidget::SetTitle(FString title)
{
	Title = title;
}

void UInputBoxWidget::SetText(const FString& text)
{
	if (MaxLength > 0 && text.Len() > MaxLength)
	{
		CurrentText = text.Left(MaxLength);
	}
	else
	{
		CurrentText = text;
	}
}

FString UInputBoxWidget::GetText() const
{
	return CurrentText;
}

void UInputBoxWidget::SetPlaceholder(const FString& placeholder)
{
	PlaceholderText = placeholder;
}

void UInputBoxWidget::SetMaxLength(int32 maxLength)
{
	MaxLength = maxLength;
	
	// Truncate current text if it exceeds new max length
	if (MaxLength > 0 && CurrentText.Len() > MaxLength)
	{
		CurrentText = CurrentText.Left(MaxLength);
	}
}

bool UInputBoxWidget::Validate()
{
	if (OnValidateText.IsBound())
	{
		FValidationResult result = OnValidateText.Execute(CurrentText);
		bIsValid = result.bIsValid;
		ErrorMessage = result.ErrorMessage;
	}
	else
	{
		// No validation delegate bound, consider valid
		bIsValid = true;
		ErrorMessage = FString();
	}
	
	return bIsValid;
}

void UInputBoxWidget::TriggerOnTextChanged(const FString& NewText)
{
	SetText(NewText);
	Validate();
	OnTextChanged.Broadcast(CurrentText);
}

void UInputBoxWidget::TriggerOnTextCommitted(const FString& CommittedText)
{
	SetText(CommittedText);
	Validate();
	OnTextCommitted.Broadcast(CurrentText);
}

FValidationResult UInputBoxWidget::ValidateCharacterName(const FString& Name)
{
	FValidationResult Result;
	Result.bIsValid = true;
	
	// Check length (3-20 characters)
	if (Name.Len() < 3)
	{
		Result.bIsValid = false;
		Result.ErrorMessage = TEXT("Name must be at least 3 characters");
		return Result;
	}
	
	if (Name.Len() > 20)
	{
		Result.bIsValid = false;
		Result.ErrorMessage = TEXT("Name must be no more than 20 characters");
		return Result;
	}
	
	// Check for leading/trailing spaces
	FString TrimmedName = Name.TrimStartAndEnd();
	if (TrimmedName != Name)
	{
		Result.bIsValid = false;
		Result.ErrorMessage = TEXT("Name cannot have leading or trailing spaces");
		return Result;
	}
	
	// Check for valid characters (alphanumeric, space, hyphen)
	// Also check for consecutive spaces
	bool bPreviousWasSpace = false;
	for (int32 i = 0; i < Name.Len(); i++)
	{
		TCHAR c = Name[i];
		bool bIsAlphanumeric = FChar::IsAlnum(c);
		bool bIsSpace = (c == ' ');
		bool bIsHyphen = (c == '-');
		
		if (!bIsAlphanumeric && !bIsSpace && !bIsHyphen)
		{
			Result.bIsValid = false;
			Result.ErrorMessage = TEXT("Name can only contain letters, numbers, spaces, and hyphens");
			return Result;
		}
		
		// Check for consecutive spaces
		if (bIsSpace && bPreviousWasSpace)
		{
			Result.bIsValid = false;
			Result.ErrorMessage = TEXT("Name cannot contain consecutive spaces");
			return Result;
		}
		
		bPreviousWasSpace = bIsSpace;
	}
	
	Result.ErrorMessage = TEXT("");
	return Result;
}
