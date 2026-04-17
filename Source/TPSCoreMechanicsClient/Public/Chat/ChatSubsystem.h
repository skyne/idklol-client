// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GrpcHandlerSubsystem.h"
#include "SChat/ChatService.h"
#include "Game/PlayerController/TPSCorePlayerController.h"
#include "ChatSubsystem.generated.h"

/**
 * Chat subsystem - handles chat service gRPC connection and messaging
 */

UENUM(BlueprintType)
enum class EChatMessageKind : uint8
{
	CharacterSpeech UMETA(DisplayName = "Character Speech"),
	NpcSpeech UMETA(DisplayName = "NPC Speech"),
	Party UMETA(DisplayName = "Party"),
	Channel UMETA(DisplayName = "Channel"),
	Yell UMETA(DisplayName = "Yell"),
	Announcement UMETA(DisplayName = "Announcement"),
	Service UMETA(DisplayName = "Service")
};

UENUM(BlueprintType)
enum class EChatDeliveryScope : uint8
{
	Local UMETA(DisplayName = "Local"),
	Global UMETA(DisplayName = "Global"),
	Party UMETA(DisplayName = "Party"),
	CustomChannel UMETA(DisplayName = "Custom Channel"),
	System UMETA(DisplayName = "System")
};

UENUM(BlueprintType)
enum class EChatChannelType : uint8
{
	Direct UMETA(DisplayName = "Direct"),
	Party UMETA(DisplayName = "Party"),
	Global UMETA(DisplayName = "Global"),
	Custom UMETA(DisplayName = "Custom"),
	System UMETA(DisplayName = "System")
};

USTRUCT(BlueprintType)
struct FChatChannelDescriptor
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Chat")
	FString ChannelId = TEXT("global");

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Chat")
	FString DisplayName = TEXT("Global");

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Chat")
	EChatChannelType ChannelType = EChatChannelType::Global;
};

USTRUCT(BlueprintType)
struct FChatMessageEnvelope
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Chat")
	FString MessageId;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Chat")
	FString Timestamp;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Chat")
	FString SpeakerId;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Chat")
	FString SpeakerDisplayName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Chat")
	FString PayloadText;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Chat")
	EChatMessageKind MessageKind = EChatMessageKind::CharacterSpeech;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Chat")
	EChatDeliveryScope DeliveryScope = EChatDeliveryScope::Global;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Chat")
	FChatChannelDescriptor Channel;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Chat")
	bool bIsSelfAuthored = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Chat")
	bool bIsServiceMessage = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnNewMessageReceivedSignature, FString, Timestamp, FString, Sender, FString, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChatMessageAddedSignature, FChatMessageEnvelope, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChatHistoryResetSignature);

UCLASS(config=Game)
class TPSCOREMECHANICSCLIENT_API UChatSubsystem : public UGrpcHandlerSubsystem
{
	GENERATED_BODY()
	
protected:
	DECLARE_GRPC_SUBSYSTEM_TYPES(ChatService)

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	// Override base class methods
	virtual void OnServiceConnected(UObject* InService, UObject* InClient) override;
	virtual void OnServiceDisconnected() override;
	virtual void HandleGrpcStateChange(EGrpcServiceState ServiceState) override;
	
	// Chat-specific stream response handler
	UFUNCTION()
	void HandleStreamResponse(FGrpcContextHandle Handle, const FGrpcResult& GrpcResult, const FGrpcChatChatMessage& Response);

	UFUNCTION()
	void HandleNpcChatMessageFromPlayerController(FString Sender, FString Message);

	void EnsureLocalPlayerControllerBinding();
	void UnbindLocalPlayerController();
	void AppendMessageToHistory(const FChatMessageEnvelope& Message);
	FChatMessageEnvelope BuildEnvelopeFromGrpcMessage(const FGrpcChatChatMessage& Response) const;
	FChatMessageEnvelope BuildNpcEnvelope(const FString& Sender, const FString& Message) const;
	
public:
	// Chat-specific functionality
	UFUNCTION(BlueprintCallable, Category = "Chat Subsystem")
	void NewChatMessage(FString Message);

	UFUNCTION(BlueprintCallable, Category = "Chat Subsystem")
	void SendChatMessage(const FString& Message, const FString& SenderOverride = TEXT(""));

	/** Event broadcast when a new chat message is received */
	UPROPERTY(BlueprintAssignable, Category = "Chat Subsystem")
	FOnNewMessageReceivedSignature OnNewMessageReceived;

	UPROPERTY(BlueprintAssignable, Category = "Chat Subsystem")
	FOnChatMessageAddedSignature OnChatMessageAdded;

	UPROPERTY(BlueprintAssignable, Category = "Chat Subsystem")
	FOnChatHistoryResetSignature OnChatHistoryReset;
	
	UFUNCTION(BlueprintCallable, Category = "Chat Subsystem")
	void TriggerNewMessageReceived(const FString& Timestamp, const FString& Sender, const FString& Message);

	UFUNCTION(BlueprintCallable, Category = "Chat Subsystem")
	void TriggerMessageEnvelopeReceived(const FChatMessageEnvelope& Message);

	UFUNCTION(BlueprintCallable, Category = "Chat Subsystem")
	void ResetMessageHistory();

	UFUNCTION(BlueprintCallable, Category = "Chat Subsystem")
	void GetMessageHistory(TArray<FChatMessageEnvelope>& OutMessages) const;

	UFUNCTION(BlueprintPure, Category = "Chat Subsystem")
	bool IsChatWindowFocused() const;

private:
	UPROPERTY(Transient)
	TArray<FChatMessageEnvelope> MessageHistory;

	UPROPERTY(Transient)
	TWeakObjectPtr<ATPSCorePlayerController> BoundPlayerController;

	FTimerHandle PlayerControllerBindingTimerHandle;
};