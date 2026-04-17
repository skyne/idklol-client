#include "Chat/ChatSubsystem.h"
#include "CharacterCreation/SelectedCharacterSubsystem.h"

void UChatSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PlayerControllerBindingTimerHandle,
			this,
			&UChatSubsystem::EnsureLocalPlayerControllerBinding,
			0.5f,
			true);
	}

	EnsureLocalPlayerControllerBinding();
}

void UChatSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PlayerControllerBindingTimerHandle);
	}

	UnbindLocalPlayerController();
	Super::Deinitialize();
}

void UChatSubsystem::OnServiceConnected(UObject* InService, UObject* InClient)
{
	UChatService* ChatService = GetService();
	UChatServiceClient* ChatClient = GetClient();
	
	if (!ChatService || !ChatClient)
	{
		UE_LOG(LogTemp, Error, TEXT("[ChatSubsystem] Invalid service or client"));
		return;
	}
	
	// Bind to stream responses
	ChatClient->OnStreamResponse.AddUniqueDynamic(this, &UChatSubsystem::HandleStreamResponse);
	
	// Initialize and start the chat stream
	FGrpcContextHandle Context = ChatClient->InitStream();
	FGrpcGoogleProtobufEmpty Request = {};
	ChatClient->Stream(Context, Request);
	
	UE_LOG(LogTemp, Log, TEXT("[ChatSubsystem] Chat stream started"));
}

void UChatSubsystem::OnServiceDisconnected()
{
	if (UChatServiceClient* ChatClient = GetClient())
	{
		ChatClient->OnStreamResponse.RemoveDynamic(this, &UChatSubsystem::HandleStreamResponse);
	}
}

void UChatSubsystem::HandleStreamResponse(FGrpcContextHandle Handle, const FGrpcResult& GrpcResult, const FGrpcChatChatMessage& Response)
{
	// Monitor stream result codes to detect disconnections or transport errors,
	// as service state change notifications may be delayed.
	if (GrpcResult.Code == EGrpcResultCode::Ok)
	{
		TriggerMessageEnvelopeReceived(BuildEnvelopeFromGrpcMessage(Response));
	}
	else
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[ChatSubsystem] Chat stream error. Code=%d, Message=%s"),
			static_cast<int32>(GrpcResult.Code),
			*GrpcResult.GetMessageString()
		);

		// Treat transport-level failures as a loss of connection and block further sends.
		switch (GrpcResult.Code)
		{
		case EGrpcResultCode::Unavailable:
		case EGrpcResultCode::ConnectionFailed:
		case EGrpcResultCode::Internal:
		case EGrpcResultCode::Unknown:
		default:
			SetConnectionStatus(EGrpcConnectionStatus::TransientError);
			break;
		}

		// Best-effort: cancel the errored context
		if (UChatServiceClient* ChatClient = GetClient())
		{
			ChatClient->TryCancel(Handle);
		}
	}
}

void UChatSubsystem::HandleGrpcStateChange(EGrpcServiceState ServiceState)
{
	// Call parent implementation for standard state handling
	Super::HandleGrpcStateChange(ServiceState);
	
	// Add any chat-specific state handling here if needed
}

void UChatSubsystem::NewChatMessage(FString Message)
{
	SendChatMessage(Message, TEXT(""));
}

void UChatSubsystem::SendChatMessage(const FString& Message, const FString& SenderOverride)
{
	UE_LOG(LogTemp, Log, TEXT("[ChatSubsystem] Sending chat message"));

	if (!IsConnected())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[ChatSubsystem] Cannot send chat message: not connected. Status=%d"),
			static_cast<int32>(ConnectionStatus)
		);
		return;
	}

	UChatServiceClient* ChatClient = GetClient();
	if (!ChatClient)
	{
		UE_LOG(LogTemp, Error, TEXT("[ChatSubsystem] Client is not available"));
		return;
	}

	FString SenderName = TEXT("Unknown");
	FGrpcMetaData MetaData = FGrpcMetaData();
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USelectedCharacterSubsystem* SelectedCharacterSubsystem = GameInstance->GetSubsystem<USelectedCharacterSubsystem>())
		{
			if (SelectedCharacterSubsystem->HasSelectedCharacter())
			{
				FCharacterData SelectedCharacter = SelectedCharacterSubsystem->GetSelectedCharacter();
				SenderName = SelectedCharacter.Name;
				if (!SelectedCharacter.CharacterId.IsEmpty())
				{
					MetaData.MetaData.Add("x-character-id", SelectedCharacter.CharacterId);
				}
			}
		}
	}

	if (!SenderOverride.IsEmpty())
	{
		SenderName = SenderOverride;
	}

	FGrpcContextHandle Context = ChatClient->InitMessage();
	FGrpcChatChatMessage ChatMessage;
	ChatMessage.Timestamp = "NOW";
	ChatMessage.Message = Message;
	ChatMessage.Sender = SenderName;
	MetaData.MetaData.Add("authorization", GetValidAuthToken());
	ChatClient->Message(Context, ChatMessage, MetaData);
}

void UChatSubsystem::TriggerNewMessageReceived(const FString& Timestamp,const FString& Sender,const FString& Message)
{
	FChatMessageEnvelope Envelope;
	Envelope.MessageId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
	Envelope.Timestamp = Timestamp;
	Envelope.SpeakerDisplayName = Sender;
	Envelope.PayloadText = Message;
	Envelope.MessageKind = EChatMessageKind::CharacterSpeech;
	Envelope.DeliveryScope = EChatDeliveryScope::Global;
	Envelope.Channel.ChannelId = TEXT("global");
	Envelope.Channel.DisplayName = TEXT("Global");
	Envelope.Channel.ChannelType = EChatChannelType::Global;
	TriggerMessageEnvelopeReceived(Envelope);
}

void UChatSubsystem::TriggerMessageEnvelopeReceived(const FChatMessageEnvelope& Message)
{
	AppendMessageToHistory(Message);
	OnChatMessageAdded.Broadcast(Message);
	OnNewMessageReceived.Broadcast(Message.Timestamp, Message.SpeakerDisplayName, Message.PayloadText);
}

void UChatSubsystem::ResetMessageHistory()
{
	MessageHistory.Reset();
	OnChatHistoryReset.Broadcast();
}

void UChatSubsystem::GetMessageHistory(TArray<FChatMessageEnvelope>& OutMessages) const
{
	OutMessages = MessageHistory;
}

bool UChatSubsystem::IsChatWindowFocused() const
{
	return BoundPlayerController.IsValid() && BoundPlayerController->IsChatWindowFocused();
}

void UChatSubsystem::HandleNpcChatMessageFromPlayerController(FString Sender, FString Message)
{
	TriggerMessageEnvelopeReceived(BuildNpcEnvelope(Sender, Message));
}

void UChatSubsystem::EnsureLocalPlayerControllerBinding()
{
	UGameInstance* GameInstance = GetGameInstance();
	if (!IsValid(GameInstance))
	{
		return;
	}

	ATPSCorePlayerController* PlayerController = Cast<ATPSCorePlayerController>(GameInstance->GetFirstLocalPlayerController());
	if (PlayerController == BoundPlayerController.Get())
	{
		return;
	}

	UnbindLocalPlayerController();
	BoundPlayerController = PlayerController;

	if (BoundPlayerController.IsValid())
	{
		BoundPlayerController->OnNpcChatMessageReceived.AddUniqueDynamic(this, &UChatSubsystem::HandleNpcChatMessageFromPlayerController);
	}
}

void UChatSubsystem::UnbindLocalPlayerController()
{
	if (BoundPlayerController.IsValid())
	{
		BoundPlayerController->OnNpcChatMessageReceived.RemoveDynamic(this, &UChatSubsystem::HandleNpcChatMessageFromPlayerController);
		BoundPlayerController.Reset();
	}
}

void UChatSubsystem::AppendMessageToHistory(const FChatMessageEnvelope& Message)
{
	MessageHistory.Add(Message);
}

FChatMessageEnvelope UChatSubsystem::BuildEnvelopeFromGrpcMessage(const FGrpcChatChatMessage& Response) const
{
	FChatMessageEnvelope Envelope;
	Envelope.MessageId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
	Envelope.Timestamp = Response.Timestamp;
	Envelope.SpeakerDisplayName = Response.Sender;
	Envelope.PayloadText = Response.Message;
	Envelope.MessageKind = EChatMessageKind::CharacterSpeech;
	Envelope.DeliveryScope = EChatDeliveryScope::Global;
	Envelope.Channel.ChannelId = TEXT("global");
	Envelope.Channel.DisplayName = TEXT("Global");
	Envelope.Channel.ChannelType = EChatChannelType::Global;
	return Envelope;
}

FChatMessageEnvelope UChatSubsystem::BuildNpcEnvelope(const FString& Sender, const FString& Message) const
{
	FChatMessageEnvelope Envelope;
	Envelope.MessageId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
	Envelope.Timestamp = TEXT("NOW");
	Envelope.SpeakerDisplayName = Sender;
	Envelope.PayloadText = Message;
	Envelope.MessageKind = EChatMessageKind::NpcSpeech;
	Envelope.DeliveryScope = EChatDeliveryScope::Local;
	Envelope.Channel.ChannelId = TEXT("npc");
	Envelope.Channel.DisplayName = TEXT("NPC");
	Envelope.Channel.ChannelType = EChatChannelType::Direct;
	return Envelope;
}