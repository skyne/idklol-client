#include "UI/WindowWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"

void UWindowWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyWindowGeometry();
	OnWindowActivationChanged(bIsWindowActive);
}

void UWindowWidget::SetWindowPosition(const FVector2D& InPosition)
{
	SetWindowPositionAndSize(InPosition, WindowSize);
}

void UWindowWidget::SetWindowSize(const FVector2D& InSize)
{
	SetWindowPositionAndSize(WindowPosition, InSize);
}

void UWindowWidget::SetWindowPositionAndSize(const FVector2D& InPosition, const FVector2D& InSize)
{
	WindowSize = ClampWindowSizeToViewport(InSize);
	WindowPosition = bClampToViewportBounds ? ClampWindowPositionToViewport(InPosition, WindowSize) : InPosition;
	ApplyWindowGeometry();
}

void UWindowWidget::SetWindowActive(bool bInIsActive)
{
	if (bIsWindowActive == bInIsActive)
	{
		return;
	}

	bIsWindowActive = bInIsActive;
	OnWindowActivationChanged(bIsWindowActive);
}

void UWindowWidget::BeginWindowDrag(const FVector2D& PointerScreenPosition)
{
	InteractionMode = EWindowInteractionMode::Dragging;
	ActiveResizeHandle = EWindowResizeHandle::None;
	InteractionStartScreenPosition = PointerScreenPosition;
	InteractionStartWindowPosition = WindowPosition;
	InteractionStartWindowSize = WindowSize;
	SetWindowActive(true);
}

void UWindowWidget::BeginWindowResize(EWindowResizeHandle ResizeHandle, const FVector2D& PointerScreenPosition)
{
	if (ResizeHandle == EWindowResizeHandle::None)
	{
		return;
	}

	InteractionMode = EWindowInteractionMode::Resizing;
	ActiveResizeHandle = ResizeHandle;
	InteractionStartScreenPosition = PointerScreenPosition;
	InteractionStartWindowPosition = WindowPosition;
	InteractionStartWindowSize = WindowSize;
	SetWindowActive(true);
}

void UWindowWidget::UpdateWindowInteraction(const FVector2D& PointerScreenPosition)
{
	if (InteractionMode == EWindowInteractionMode::None)
	{
		return;
	}

	const FVector2D Delta = PointerScreenPosition - InteractionStartScreenPosition;

	if (InteractionMode == EWindowInteractionMode::Dragging)
	{
		SetWindowPositionAndSize(InteractionStartWindowPosition + Delta, InteractionStartWindowSize);
		return;
	}

	const float RightEdge = InteractionStartWindowPosition.X + InteractionStartWindowSize.X;
	const float BottomEdge = InteractionStartWindowPosition.Y + InteractionStartWindowSize.Y;

	FVector2D NewPosition = InteractionStartWindowPosition;
	FVector2D NewSize = InteractionStartWindowSize;

	switch (ActiveResizeHandle)
	{
	case EWindowResizeHandle::Left:
	case EWindowResizeHandle::TopLeft:
	case EWindowResizeHandle::BottomLeft:
		NewPosition.X = FMath::Min(InteractionStartWindowPosition.X + Delta.X, RightEdge - MinimumWindowSize.X);
		NewSize.X = RightEdge - NewPosition.X;
		break;

	case EWindowResizeHandle::Right:
	case EWindowResizeHandle::TopRight:
	case EWindowResizeHandle::BottomRight:
		NewSize.X = FMath::Max(MinimumWindowSize.X, InteractionStartWindowSize.X + Delta.X);
		break;

	default:
		break;
	}

	switch (ActiveResizeHandle)
	{
	case EWindowResizeHandle::Top:
	case EWindowResizeHandle::TopLeft:
	case EWindowResizeHandle::TopRight:
		NewPosition.Y = FMath::Min(InteractionStartWindowPosition.Y + Delta.Y, BottomEdge - MinimumWindowSize.Y);
		NewSize.Y = BottomEdge - NewPosition.Y;
		break;

	case EWindowResizeHandle::Bottom:
	case EWindowResizeHandle::BottomLeft:
	case EWindowResizeHandle::BottomRight:
		NewSize.Y = FMath::Max(MinimumWindowSize.Y, InteractionStartWindowSize.Y + Delta.Y);
		break;

	default:
		break;
	}

	SetWindowPositionAndSize(NewPosition, NewSize);
}

void UWindowWidget::EndWindowInteraction()
{
	InteractionMode = EWindowInteractionMode::None;
	ActiveResizeHandle = EWindowResizeHandle::None;
}

bool UWindowWidget::IsWindowInteracting() const
{
	return InteractionMode != EWindowInteractionMode::None;
}

void UWindowWidget::ApplyWindowGeometry()
{
	SetPositionInViewport(WindowPosition, false);
	SetDesiredSizeInViewport(WindowSize);
	OnWindowGeometryChanged(WindowPosition, WindowSize);
}

FVector2D UWindowWidget::GetViewportSizeSafe() const
{
	return UWidgetLayoutLibrary::GetViewportSize(this);
}

FVector2D UWindowWidget::ClampWindowSizeToViewport(const FVector2D& InSize) const
{
	const FVector2D ViewportSize = GetViewportSizeSafe();
	FVector2D ClampedSize(FMath::Max(MinimumWindowSize.X, InSize.X), FMath::Max(MinimumWindowSize.Y, InSize.Y));

	if (bClampToViewportBounds && ViewportSize.X > 0.f && ViewportSize.Y > 0.f)
	{
		ClampedSize.X = FMath::Min(ClampedSize.X, ViewportSize.X);
		ClampedSize.Y = FMath::Min(ClampedSize.Y, ViewportSize.Y);
	}

	return ClampedSize;
}

FVector2D UWindowWidget::ClampWindowPositionToViewport(const FVector2D& InPosition, const FVector2D& InSize) const
{
	if (!bClampToViewportBounds)
	{
		return InPosition;
	}

	const FVector2D ViewportSize = GetViewportSizeSafe();
	if (ViewportSize.X <= 0.f || ViewportSize.Y <= 0.f)
	{
		return InPosition;
	}

	return FVector2D(
		FMath::Clamp(InPosition.X, 0.f, FMath::Max(0.f, ViewportSize.X - InSize.X)),
		FMath::Clamp(InPosition.Y, 0.f, FMath::Max(0.f, ViewportSize.Y - InSize.Y)));
}