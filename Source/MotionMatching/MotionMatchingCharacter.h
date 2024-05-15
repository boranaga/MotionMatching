// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"

#include "MMcommon.h"
#include "MMvec.h"
#include "MMquat.h"
#include "MMarray.h"
#include "MMcharacter.h"
#include "MMdatabase.h"
#include "MMnnet.h"
#include "MMlmm.h"


#include "MotionMatchingCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class AMotionMatchingCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;


protected: //Motion Matching ���� variables

	//���̷����� �� Joint �̸��� ������ �迭
	const TArray<std::string> JointsNames = {
		"Root",
		"Hips",
		"LeftUpLeg",
		"LeftLeg",
		"LeftFoot",
		"LeftToe",
		"RightUpLeg",
		"RightLeg",
		"RightFoot",
		"RightToe",
		"Spine",
		"Spine1",
		"Spine2",
		"Neck",
		"Head",
		"LeftShoulder",
		"LeftArm",
		"LeftForeArm",
		"LeftHand",
		"RightShoulder",
		"RightArm",
		"RightForeArm",
		"RightHand"
	};

	const TArray<FString> JointsNames2 = {
		TEXT("Root"),
		TEXT("Hips"),
		TEXT("LeftUpLeg"),
		TEXT("LeftLeg"),
		TEXT("LeftFoot"),
		TEXT("LeftToe"),
		TEXT("RightUpLeg"),
		TEXT("RightLeg"),
		TEXT("RightFoot"),
		TEXT("RightToe"),
		TEXT("Spine"),
		TEXT("Spine1"),
		TEXT("Spine2"),
		TEXT("Neck"),
		TEXT("Head"),
		TEXT("LeftShoulder"),
		TEXT("LeftArm"),
		TEXT("LeftForeArm"),
		TEXT("LeftHand"),
		TEXT("RightShoulder"),
		TEXT("RightArm"),
		TEXT("RightForeArm"),
		TEXT("RightHand")
	};



	//���̷����� �⺻ rotator �� ����
	TArray<FRotator> BasicCharatorRotator;
	//���̷����� �⺻ position(vector) �� ����
	TArray<FVector> BasicCharatorVector;

	database db;


public:
	AMotionMatchingCharacter();
	

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);
			

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay();

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }


public: //poseblemesh ����

	//SkeletalMesh��� ������ �����ϼ� �ִ� UPoseableMeshComponent ��� �� ����.
	UPROPERTY(Category = Character, EditAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UPoseableMeshComponent> PoseableMesh;

	//UCharacter Ŭ������ �����ϴ� GetMesh() �Լ� ������ -> PosealbeMeshComponent�� ������.
	UFUNCTION()
	virtual UPoseableMeshComponent* GetMesh() const;

	UFUNCTION()
	void PoseTest(); //Pose test�ϱ� ���� �Լ�

	UFUNCTION()
	void PoseTest2(int frameindex); //Pose test�ϱ� ���� �Լ� //���ϴ� frame �� ������ �׿� �ش��ϴ� pose�� �����

	UFUNCTION()
	void PoseTestByPostion(int frameindex); //Pose test�ϱ� ���� �Լ� //���ϴ� frame �� ������ �׿� �ش��ϴ� pose�� �����


public: //Motion Matching ����
	
	UFUNCTION()
	void MotionMatchingMainBeginPlay();

	UFUNCTION()
	void MotionMatchingMainTick();

	UFUNCTION()
	void SaveBasicRotators(); //ĳ���� ���̷����� �⺻ Rotator �� ����

	UFUNCTION()
	void SaveBasicVectors(); //ĳ���� ���̷����� �⺻ Vector �� ����


	UFUNCTION()
	void DataBaseLog();

};




//------------------------------------------------------
//<����� ���� �κ�>

//static inline Vector3 to_Vector3(vec3 v)
//{
//	return (Vector3) { v.x, v.y, v.z };
//}





//--------------------------------------
// Basic functionality to get gamepad input including deadzone and 
// squaring of the stick location to increase sensitivity. To make 
// all the other code that uses this easier, we assume stick is 
// oriented on floor (i.e. y-axis is zero)

//��� �ϴµ� ������ �� ����ϴ��� �𸣰���
enum
{
	GAMEPAD_PLAYER = 0,
};

enum
{
	GAMEPAD_STICK_LEFT,
	GAMEPAD_STICK_RIGHT,
};



////------------------------------------------------------
//vec3 gamepad_get_stick(int stick, const float deadzone = 0.2f)
//{
//	//GetGamepadAxisMovement �� �Լ��� ��ü�� ���� �˾Ƴ��� ��
//	float gamepadx = GetGamepadAxisMovement(GAMEPAD_PLAYER, stick == GAMEPAD_STICK_LEFT ? GAMEPAD_AXIS_LEFT_X : GAMEPAD_AXIS_RIGHT_X);
//	float gamepady = GetGamepadAxisMovement(GAMEPAD_PLAYER, stick == GAMEPAD_STICK_LEFT ? GAMEPAD_AXIS_LEFT_Y : GAMEPAD_AXIS_RIGHT_Y);
//	float gamepadmag = sqrtf(gamepadx * gamepadx + gamepady * gamepady);
//
//	if (gamepadmag > deadzone)
//	{
//		float gamepaddirx = gamepadx / gamepadmag;
//		float gamepaddiry = gamepady / gamepadmag;
//		float gamepadclippedmag = gamepadmag > 1.0f ? 1.0f : gamepadmag * gamepadmag;
//		gamepadx = gamepaddirx * gamepadclippedmag;
//		gamepady = gamepaddiry * gamepadclippedmag;
//	}
//	else
//	{
//		gamepadx = 0.0f;
//		gamepady = 0.0f;
//	}
//
//	return vec3(gamepadx, 0.0f, gamepady);
//}


////------------------------------------------------------