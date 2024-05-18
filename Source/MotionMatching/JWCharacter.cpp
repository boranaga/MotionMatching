// Fill out your copyright notice in the Description page of Project Settings.


#include "JWCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "MMPlayerController.h"


void AJWCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		EnhancedInputComponent->BindAction(MenuAction, ETriggerEvent::Triggered, this, &AJWCharacter::DisPlayMenu);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}
void AJWCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AJWCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (LMM_enabled)
	{
		UE_LOG(LogTemp, Log, TEXT("LMM ON"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("LMM OFF"));
	}

	if (Ik_enabled)
	{
		UE_LOG(LogTemp, Log, TEXT("Ik_enabled ON"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Ik_enabled OFF"));
	}

	if (Adjustment_enabled)
	{
		UE_LOG(LogTemp, Log, TEXT("Adjustment_enabled ON"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Adjustment_enabled OFF"));
	}

	if (Clamping_enabled)
	{
		UE_LOG(LogTemp, Log, TEXT("Clamping_enabled ON"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Clamping_enabled OFF"));
	}
}

void AJWCharacter::DisPlayMenu()
{
	AMMPlayerController* PlayerController = Cast<AMMPlayerController>(Controller);
	IsTabButtonDown = !IsTabButtonDown;

	if(PlayerController)
	{
		PlayerController->ViewMenu(IsTabButtonDown);

	}
}







//void AJWCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
//{
//	Super::SetupPlayerInputComponent(PlayerInputComponent);
//
//	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
//
//		// Strafe
//		// 주의 : 메인 캐릭터로 복붙할때 &AJWCharacter:: 이거 수정!
//		EnhancedInputComponent->BindAction(StrafeAction, ETriggerEvent::Started, this, &AJWCharacter::OnStrafe);
//		EnhancedInputComponent->BindAction(StrafeAction, ETriggerEvent::Completed, this, &AJWCharacter::OffStrafe);
//
//	}
//	else
//	{
//		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
//	}
//}
//void AJWCharacter::OnStrafe(const FInputActionValue& Value)
//{
//	Desired_strafe = true;
//}
//
//void AJWCharacter::OffStrafe(const FInputActionValue& Value)
//{
//	Desired_strafe = false;
//}
