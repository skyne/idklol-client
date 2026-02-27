#include "Chat/ChatSubsystem.h"

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
		TriggerNewMessageReceived(Response.Timestamp, Response.Sender, Response.Message);
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
	UE_LOG(LogTemp, Log, TEXT("[ChatSubsystem] Sending new message"));
	
	// TurboLink / gRPC generally does not use C++ exceptions, so instead of try/catch
	// we defensively check our tracked connection status and client validity before sending.
	if (IsConnected())
	{
		UChatServiceClient* ChatClient = GetClient();
		if (!ChatClient)
		{
			UE_LOG(LogTemp, Error, TEXT("[ChatSubsystem] Client is not available"));
			return;
		}
		
		FGrpcContextHandle Context = ChatClient->InitMessage();
		FGrpcChatChatMessage ChatMessage;
		ChatMessage.Timestamp = "NOW";
		ChatMessage.Message = Message;
		ChatMessage.Sender = "Me";
		FGrpcMetaData MetaData = FGrpcMetaData();
		MetaData.MetaData.Add("authorization", GetAuthTokenValue());
		ChatClient->Message(Context, ChatMessage, MetaData);
	}
	else
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[ChatSubsystem] Cannot send chat message: not connected. Status=%d"),
			static_cast<int32>(ConnectionStatus)
		);
	}
}

void UChatSubsystem::TriggerNewMessageReceived(const FString& Timestamp,const FString& Sender,const FString& Message)
{
	OnNewMessageReceived.Broadcast(Timestamp, Sender, Message);
}