// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/TPSCoreSystemsWidget.h"

void UTPSCoreSystemsWidget::SetWidgetController(UWidgetController* InWidgetController)
{
	WidgetController = InWidgetController;
	OnWidgetControllerSet();
}
