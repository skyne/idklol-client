// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/NpcInteractionPromptWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"

void UNpcInteractionPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* RootPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("NpcPromptRoot"));
		WidgetTree->RootWidget = RootPanel;

		PromptButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("NpcPromptButton"));
		PromptTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NpcPromptText"));

		if (IsValid(PromptTextBlock))
		{
			PromptTextBlock->SetText(DefaultPromptText);
		}

		if (IsValid(PromptButton) && IsValid(PromptTextBlock))
		{
			PromptButton->AddChild(PromptTextBlock);
		}

		if (IsValid(PromptButton) && IsValid(RootPanel))
		{
			if (UCanvasPanelSlot* PromptSlot = RootPanel->AddChildToCanvas(PromptButton))
			{
				PromptSlot->SetAutoSize(true);
			}
		}
	}

	SetPromptText(DefaultPromptText);
	SetAlignmentInViewport(FVector2D(0.5f, 1.0f));
	SetDesiredSizeInViewport(FVector2D(140.f, 36.f));
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UNpcInteractionPromptWidget::SetPromptText(const FText& InText)
{
	if (IsValid(PromptTextBlock))
	{
		PromptTextBlock->SetText(InText);
	}
}

void UNpcInteractionPromptWidget::SetPromptScreenPosition(const FVector2D& InScreenPosition)
{
	SetPositionInViewport(InScreenPosition);
}
