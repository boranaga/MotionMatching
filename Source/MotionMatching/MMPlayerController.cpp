// Fill out your copyright notice in the Description page of Project Settings.


#include "MMPlayerController.h"
#include "Blueprint/UserWidget.h"

void AMMPlayerController::BeginPlay()
{
	Super::BeginPlay();

    if (MenuWidgetClass)
    {
        MenuWidget = CreateWidget<UUserWidget>(GetWorld(), MenuWidgetClass);
        if (MenuWidget)
        {
            MenuWidget->AddToViewport();
            MenuWidget->SetVisibility(ESlateVisibility::Hidden);
        }
    }

}

void AMMPlayerController::ViewMenu(bool isMenuVisible)
{
    if (MenuWidget)
    {
        MenuWidget->SetVisibility(isMenuVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }
}
