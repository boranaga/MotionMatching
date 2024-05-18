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

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UUserWidget> MenuWidgetClass;

	UPROPERTY()
	class UUserWidget* MenuWidget;


public:
	UFUNCTION()
	void ViewMenu(bool isMenuVisible);
};
