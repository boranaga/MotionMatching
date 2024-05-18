// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MotionMatchingCharacter.h"
#include "JWCharacter.generated.h"

/**
 * 
 */
UCLASS()
class MOTIONMATCHING_API AJWCharacter : public AMotionMatchingCharacter
{
	GENERATED_BODY()

protected:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// To add mapping context
	virtual void BeginPlay();

	// Tick �Լ�
	virtual void Tick(float DeltaTime) override; //override�� ���ϰ� �׳� ����� �ȵǳ�?


protected:

	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	//UInputAction* MenuAction;

	//void TapKeyDown();
	//bool IsTabButtonDown = false;

	/** StrafeInput Action */
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	/*UInputAction* StrafeAction;*/


	///** Called for looking input */
	//void OnStrafe(const FInputActionValue& Value);

	//void OffStrafe(const FInputActionValue& Value);

	//bool Desired_strafe = false;

};
