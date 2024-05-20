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

	// Tick 함수
	virtual void Tick(float DeltaTime) override; //override를 안하고 그냥 사용은 안되나?


protected:
	FVector To_Vector3(vec3 v);

	void Draw_features(const slice1d<float> features, const vec3 pos, const quat rot, FColor color);
	void Draw_trajectory(const slice1d<vec3> trajectory_positions, const slice1d<quat> trajectory_rotations, FColor color);
	void Draw_simulation_object(const vec3 simulation_position, quat simulation_rotation, FColor color);
};







//void Draw_axis(const vec3 pos, const quat rot, const float scale = 1.0f);
// 
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