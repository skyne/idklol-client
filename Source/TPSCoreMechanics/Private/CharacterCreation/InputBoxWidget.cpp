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
