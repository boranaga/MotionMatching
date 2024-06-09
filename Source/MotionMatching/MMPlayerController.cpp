// Fill out your copyright notice in the Description page of Project Settings.


#include "MMPlayerController.h"
#include "Blueprint/UserWidget.h"

void AMMPlayerController::BeginPlay()
{
	Super::BeginPlay();

    //Menu UI
    if (MenuWidgetClass)
    {
        MenuWidget = CreateWidget<UUserWidget>(GetWorld(), MenuWidgetClass);
        if (MenuWidget)
        {
            MenuWidget->AddToViewport();
            MenuWidget->SetVisibility(ESlateVisibility::Hidden);
        }
    }


    //Stamina UI
    if (StaminaWidgetClass)
    {
        StaminaWidget = CreateWidget<UUserWidget>(GetWorld(), StaminaWidgetClass);
        if (StaminaWidget)
        {
            StaminaWidget->AddToViewport();
            StaminaWidget->SetVisibility(ESlateVisibility::Visible);
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
