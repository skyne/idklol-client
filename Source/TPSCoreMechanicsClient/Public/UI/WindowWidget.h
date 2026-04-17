#pragma once

#include "CoreMinimal.h"
#include "UI/TPSCoreSystemsWidget.h"
#include "WindowWidget.generated.h"

UENUM(BlueprintType)
enum class EWindowResizeHandle : uint8
{
	None UMETA(DisplayName = "None"),
	Left UMETA(DisplayName = "Left"),
	TopLeft UMETA(DisplayName = "Top Left"),
	Top UMETA(DisplayName = "Top"),
	TopRight UMETA(DisplayName = "Top Right"),
	Right UMETA(DisplayName = "Right"),
	BottomRight UMETA(DisplayName = "Bottom Right"),
	Bottom UMETA(DisplayName = "Bottom"),
	BottomLeft UMETA(DisplayName = "Bottom Left")
};

UCLASS(Abstract, Blueprintable)
class TPSCOREMECHANICSCLIENT_API UWindowWidget : public UTPSCoreSystemsWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Window")
	void SetWindowPosition(const FVector2D& InPosition);

	UFUNCTION(BlueprintCallable, Category = "Window")
	void SetWindowSize(const FVector2D& InSize);

	UFUNCTION(BlueprintCallable, Category = "Window")
	void SetWindowPositionAndSize(const FVector2D& InPosition, const FVector2D& InSize);

	UFUNCTION(BlueprintCallable, Category = "Window")
	void SetWindowActive(bool bInIsActive);

	UFUNCTION(BlueprintCallable, Category = "Window")
	void BeginWindowDrag(const FVector2D& PointerScreenPosition);

	UFUNCTION(BlueprintCallable, Category = "Window")
	void BeginWindowResize(EWindowResizeHandle ResizeHandle, const FVector2D& PointerScreenPosition);

	UFUNCTION(BlueprintCallable, Category = "Window")
	void UpdateWindowInteraction(const FVector2D& PointerScreenPosition);

	UFUNCTION(BlueprintCallable, Category = "Window")
	void EndWindowInteraction();

	UFUNCTION(BlueprintPure, Category = "Window")
	FVector2D GetWindowPosition() const { return WindowPosition; }

	UFUNCTION(BlueprintPure, Category = "Window")
	FVector2D GetWindowSize() const { return WindowSize; }

	UFUNCTION(BlueprintPure, Category = "Window")
	bool IsWindowActive() const { return bIsWindowActive; }

	UFUNCTION(BlueprintPure, Category = "Window")
	bool IsWindowInteracting() const;

protected:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Window")
	void OnWindowGeometryChanged(const FVector2D& NewPosition, const FVector2D& NewSize);

	UFUNCTION(BlueprintImplementableEvent, Category = "Window")
	void OnWindowActivationChanged(bool bNewIsActive);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Window")
	FVector2D WindowPosition = FVector2D(64.f, 64.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Window")
	FVector2D WindowSize = FVector2D(720.f, 360.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Window")
	FVector2D MinimumWindowSize = FVector2D(360.f, 220.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Window")
	bool bClampToViewportBounds = true;

	UPROPERTY(BlueprintReadOnly, Category = "Window")
	bool bIsWindowActive = false;

private:
	enum class EWindowInteractionMode : uint8
	{
		None,
		Dragging,
		Resizing
	};

	EWindowInteractionMode InteractionMode = EWindowInteractionMode::None;
	EWindowResizeHandle ActiveResizeHandle = EWindowResizeHandle::None;
	FVector2D InteractionStartScreenPosition = FVector2D::ZeroVector;
	FVector2D InteractionStartWindowPosition = FVector2D::ZeroVector;
	FVector2D InteractionStartWindowSize = FVector2D::ZeroVector;

	void ApplyWindowGeometry();
	FVector2D GetViewportSizeSafe() const;
	FVector2D ClampWindowSizeToViewport(const FVector2D& InSize) const;
	FVector2D ClampWindowPositionToViewport(const FVector2D& InPosition, const FVector2D& InSize) const;
};