#include "UI/Chat/ChatWindowWidget.h"

#include "Components/PanelWidget.h"
#include "UI/Chat/ChatMessageRowWidget.h"

void UChatWindowWidget::SetMessageHistory(const TArray<FChatMessageEnvelope>& InMessages)
{
	MessageHistory = InMessages;
	RebuildMessageList();
}

void UChatWindowWidget::AppendChatMessage(const FChatMessageEnvelope& InMessage)
{
	MessageHistory.Add(InMessage);
	RebuildMessageList();
}

void UChatWindowWidget::ClearChatMessages()
{
	MessageHistory.Reset();
	RebuildMessageList();
}

void UChatWindowWidget::SetActiveChannelId(const FString& InChannelId)
{
	ActiveChannelId = InChannelId;
	RebuildMessageList();
}

void UChatWindowWidget::SetVisibleMessageKinds(const TArray<EChatMessageKind>& InVisibleMessageKinds)
{
	VisibleMessageKinds = InVisibleMessageKinds;
	RebuildMessageList();
}

void UChatWindowWidget::SetChatWindowFocused(bool bFocused)
{
	bChatWindowFocused = bFocused;
	SetWindowActive(bFocused);
	OnChatWindowStateChanged();
}

void UChatWindowWidget::SetWindowTitleText(const FText& InWindowTitleText)
{
	WindowTitleText = InWindowTitleText;
	OnChatWindowStateChanged();
}

void UChatWindowWidget::SetChatModeDisplayText(const FText& InChatModeDisplayText)
{
	ChatModeDisplayText = InChatModeDisplayText;
	OnChatWindowStateChanged();
}

void UChatWindowWidget::SubmitPendingMessage(const FString& Message)
{
	OnMessageSubmitted.Broadcast(Message);
}

void UChatWindowWidget::RebuildMessageList()
{
	UPanelWidget* MessageListContainer = GetMessageListContainer();
	if (!MessageListContainer)
	{
		OnChatWindowStateChanged();
		return;
	}

	MessageListContainer->ClearChildren();
	if (!ChatMessageRowWidgetClass)
	{
		OnChatWindowStateChanged();
		return;
	}

	for (const FChatMessageEnvelope& Message : MessageHistory)
	{
		if (!ShouldDisplayMessage(Message))
		{
			continue;
		}

		UChatMessageRowWidget* RowWidget = CreateWidget<UChatMessageRowWidget>(GetOwningPlayer(), ChatMessageRowWidgetClass);
		if (!IsValid(RowWidget))
		{
			continue;
		}

		RowWidget->SetMessageEnvelope(Message);
		MessageListContainer->AddChild(RowWidget);
	}

	OnChatWindowStateChanged();
}

bool UChatWindowWidget::ShouldDisplayMessage(const FChatMessageEnvelope& Message) const
{
	if (!ActiveChannelId.IsEmpty() && Message.Channel.ChannelId != ActiveChannelId)
	{
		return false;
	}

	if (VisibleMessageKinds.Num() == 0)
	{
		return true;
	}

	return VisibleMessageKinds.Contains(Message.MessageKind);
}