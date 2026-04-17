#include "UI/Chat/ChatMessageRowWidget.h"

void UChatMessageRowWidget::SetMessageEnvelope(const FChatMessageEnvelope& InMessage)
{
	MessageEnvelope = InMessage;
	SpeakerText = FText::FromString(MessageEnvelope.SpeakerDisplayName);
	PayloadText = FText::FromString(MessageEnvelope.PayloadText);
	TimestampText = FText::FromString(MessageEnvelope.Timestamp);
	OnMessageEnvelopeSet();
}