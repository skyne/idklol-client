#include "UI/NpcInteractionResultWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"

void UNpcInteractionResultWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* RootPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("NpcResultRoot"));
		WidgetTree->RootWidget = RootPanel;

		UBorder* Container = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("NpcResultContainer"));
		UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("NpcResultContent"));
		TitleTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NpcResultTitle"));
		MessageTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NpcResultMessage"));

		if (IsValid(TitleTextBlock))
		{
			TitleTextBlock->SetText(DefaultTitle);
			Content->AddChildToVerticalBox(TitleTextBlock);
		}

		if (IsValid(MessageTextBlock))
		{
			MessageTextBlock->SetText(DefaultMessage);
			Content->AddChildToVerticalBox(MessageTextBlock);
		}

		if (IsValid(Container) && IsValid(Content))
		{
			Container->SetContent(Content);
		}

		if (IsValid(RootPanel) && IsValid(Container))
		{
			if (UCanvasPanelSlot* CanvasSlot = RootPanel->AddChildToCanvas(Container))
			{
				CanvasSlot->SetAnchors(FAnchors(0.5f, 1.0f, 0.5f, 1.0f));
				CanvasSlot->SetAlignment(FVector2D(0.5f, 1.0f));
				CanvasSlot->SetPosition(FVector2D(0.f, -90.f));
				CanvasSlot->SetAutoSize(true);
			}
		}
	}

	SetContext(DefaultTitle, DefaultMessage);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UNpcInteractionResultWidget::SetContext(const FText& InTitle, const FText& InMessage)
{
	if (IsValid(TitleTextBlock))
	{
		TitleTextBlock->SetText(InTitle);
	}

	if (IsValid(MessageTextBlock))
	{
		MessageTextBlock->SetText(InMessage);
	}
}
