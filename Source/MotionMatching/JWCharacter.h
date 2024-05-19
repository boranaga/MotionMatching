// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MotionMatchingCharacter.h"

#include "MMcommon.h"
#include "MMvec.h"
#include "MMquat.h"
#include "MMspring.h"
#include "MMarray.h"
#include "MMcharacter.h"
#include "MMdatabase.h"
#include "MMnnet.h"
#include "MMlmm.h"

#include "JWCharacter.generated.h"

/**
 * 
 */
UCLASS()
class MOTIONMATCHING_API AJWCharacter : public AMotionMatchingCharacter
{
	GENERATED_BODY()

protected:
	//virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// To add mapping context
	virtual void BeginPlay();

	// Tick �Լ�
	virtual void Tick(float DeltaTime) override; //override�� ���ϰ� �׳� ����� �ȵǳ�?


protected:

	void draw_axis(const vec3 pos, const quat rot, const float scale = 1.0f);

	FVector to_Vector3(vec3 v);

	void draw_features(const slice1d<float> features, const vec3 pos, const quat rot);
	//void draw_trajectory(const slice1d<vec3> trajectory_positions, const slice1d<quat> trajectory_rotations);
};








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