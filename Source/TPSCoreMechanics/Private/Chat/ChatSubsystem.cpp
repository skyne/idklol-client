#include "Chat/ChatSubsystem.h"
#include "TurboLinkGrpcUtilities.h"
#include "TurboLinkGrpcManager.h"

void UChatSubsystem::SetChatConnectionStatus(EChatConnectionStatus NewStatus)
{
	if (ChatConnectionStatus != NewStatus)
	{
		ChatConnectionStatus = NewStatus;
		UE_LOG(LogTemp, Log, TEXT("Chat connection status changed to %d"), static_cast<int32>(ChatConnectionStatus));
	}
}

void UChatSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency(UTurboLinkGrpcManager::StaticClass());
	Super::Initialize(Collection);

	SetChatConnectionStatus(EChatConnectionStatus::Connecting);
	InitializeConnection();
}

void UChatSubsystem::InitializeConnection()
{
	UTurboLinkGrpcManager* TurboLinkManager = UTurboLinkGrpcUtilities::GetTurboLinkGrpcManager(this);
	if (!TurboLinkManager)
	{
		UE_LOG(LogTemp, Error, TEXT("TurboLinkGrpcManager not available"));
		SetChatConnectionStatus(EChatConnectionStatus::Disconnected);
		return;
	}
	
	ChatService = Cast<UChatService>(TurboLinkManager->MakeService("ChatService"));
	if (!IsValid(ChatService))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create ChatService"));
		SetChatConnectionStatus(EChatConnectionStatus::Disconnected);
		return;
	}
	
	ChatService->Connect();
	
	Client = ChatService->MakeClient();
	if (!IsValid(Client))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create Chat client"));
		SetChatConnectionStatus(EChatConnectionStatus::Disconnected);
		return;
	}
	
	FGrpcContextHandle Context = Client->InitStream();

	FGrpcGoogleProtobufEmpty Request = {};

	ChatService->OnServiceStateChanged.AddUniqueDynamic(this, &UChatSubsystem::HandleGrpcStateChange);
	Client->OnStreamResponse.AddUniqueDynamic(this, &UChatSubsystem::HandleStreamResponse);
	Client->Stream(Context, Request);
	
	SetChatConnectionStatus(EChatConnectionStatus::Connected);
}

void UChatSubsystem::Deinitialize()
{
	if (IsValid(ChatService))
	{
		ChatService->OnServiceStateChanged.RemoveDynamic(this, &UChatSubsystem::HandleGrpcStateChange);
	}
	if (IsValid(Client))
	{
		Client->OnStreamResponse.RemoveDynamic(this, &UChatSubsystem::HandleStreamResponse);
	}
	
	Client = nullptr;
	ChatService = nullptr;
	SetChatConnectionStatus(EChatConnectionStatus::Disconnected);
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
			TEXT("Chat stream error. Code=%d, Message=%s"),
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
			SetChatConnectionStatus(EChatConnectionStatus::TransientError);
			break;
		}

		// Best-effort: cancel the errored context and let HandleGrpcStateChange
		// (if/when fired) attempt reconnection.
		if (IsValid(Client))
		{
			Client->TryCancel(Handle);
		}
	}
}

void UChatSubsystem::HandleGrpcStateChange(EGrpcServiceState ServiceState)
{
	UE_LOG(LogTemp, Warning, TEXT("GrpcStateChange: %d"), ServiceState);
	switch (ServiceState)
	{
	case EGrpcServiceState::TransientFailure:
		SetChatConnectionStatus(EChatConnectionStatus::TransientError);
		if (IsValid(ChatService))
		{
			// Try to reconnect the service when a transient error happens.
			ChatService->Connect();
		}
		break;
	case EGrpcServiceState::Shutdown:
		SetChatConnectionStatus(EChatConnectionStatus::Shutdown);
		// Attempt a clean reinitialization of the connection if the server comes back.
		if (IsValid(this))
		{
			SetChatConnectionStatus(EChatConnectionStatus::Connecting);
			InitializeConnection();
		}
		break;
	case EGrpcServiceState::Ready:
		SetChatConnectionStatus(EChatConnectionStatus::Connected);
		break;
	default:
		SetChatConnectionStatus(EChatConnectionStatus::Unknown);
		break;
	}
}

void UChatSubsystem::NewChatMessage(FString Message)
{
	UE_LOG(LogTemp, Warning, TEXT("NewChatMessage"));
	
	// TurboLink / gRPC generally does not use C++ exceptions, so instead of try/catch
	// we defensively check our tracked connection status and client validity before sending.
	if (ChatConnectionStatus == EChatConnectionStatus::Connected
		&& IsValid(Client))
	{
		FGrpcContextHandle Context = Client->InitMessage();
		FGrpcChatChatMessage ChatMessage;
		ChatMessage.Timestamp = "NOW";
		ChatMessage.Message = Message;
		ChatMessage.Sender = "Me";
		Client->Message(Context, ChatMessage);
	}
	else
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Cannot send chat message: client invalid or chat not connected. ChatStatus=%d"),
			static_cast<int32>(ChatConnectionStatus)
		);
	}
}

void UChatSubsystem::TriggerNewMessageReceived(const FString& Timestamp,const FString& Sender,const FString& Message)
{
	OnNewMessageReceived.Broadcast(Timestamp, Sender, Message);
}