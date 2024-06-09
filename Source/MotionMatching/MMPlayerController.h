// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MMPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class MOTIONMATCHING_API AMMPlayerController : public APlayerController
{
	GENERATED_BODY()
	
	virtual void BeginPlay() override;

	//Menu UI
	UPROPERTY(EditDefaultsOnly, Category = "MenuUI")
	TSubclassOf<class UUserWidget> MenuWidgetClass;

	UPROPERTY()
	class UUserWidget* MenuWidget;

	//Stamina UI
	//UPROPERTY(EditDefaultsOnly, Category = "StaminaUI")
	UPROPERTY(EditAnywhere, Category = "StaminaUI")
	TSubclassOf<class UUserWidget> StaminaWidgetClass;

	UPROPERTY()
	class UUserWidget* StaminaWidget;


public:
	UFUNCTION()
	void ViewMenu(bool isMenuVisible);
};
