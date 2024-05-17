// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"

#include "MMcommon.h"
#include "MMvec.h"
#include "MMquat.h"
#include "MMspring.h"
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

	/** Strafe Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* StrafeAction;

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

	// Tick 함수
	virtual void Tick(float DeltaTime) override; //override를 안하고 그냥 사용은 안되나?

	//매초마다 이벤트를 발생시키게 하기 위한 타이머
	float TickTime;
	int TimePassed;


public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }


protected: //Motion Matching 관련 variables

	//스켈레톤의 각 Joint 이름을 저장한 배열
	const TArray<FString> JointsNames = {
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


	//스켈레톤의 기본 rotator 값 저장
	TArray<FRotator> BasicCharatorRotator;
	//스켈레톤의 기본 position(vector) 값 저장
	TArray<FVector> BasicCharatorVector;


	// Load Animation Data and build Matching Database
	database DB; //database.bin과 features.bin 모두 DB에 저장됨

	//feature를 저장할 때 사용되는 weight의 값
	float Feature_weight_foot_position = 0.75f;
	float Feature_weight_foot_velocity = 1.0f;
	float Feature_weight_hip_velocity = 1.0f;
	float Feature_weight_trajectory_positions = 1.0f;
	float Feature_weight_trajectory_directions = 1.5f;

	// Character
	character Character_data;

	// Pose & Inertializer Data
	int Frame_index;
	float Inertialize_blending_halflife = 0.1f;

	array1d<vec3> Curr_bone_positions;
	array1d<vec3> Curr_bone_velocities;
	array1d<quat> Curr_bone_rotations;
	array1d<vec3> Curr_bone_angular_velocities;
	array1d<bool> Curr_bone_contacts;

	array1d<vec3> Trns_bone_positions;
	array1d<vec3> Trns_bone_velocities;
	array1d<quat> Trns_bone_rotations;
	array1d<vec3> Trns_bone_angular_velocities;
	array1d<bool> Trns_bone_contacts;

	array1d<vec3> Bone_positions;
	array1d<vec3> Bone_velocities;
	array1d<quat> Bone_rotations;
	array1d<vec3> Bone_angular_velocities;

	array1d<vec3> Bone_offset_positions;
	array1d<vec3> Bone_offset_velocities;
	array1d<quat> Bone_offset_rotations;
	array1d<vec3> Bone_offset_angular_velocities;

	array1d<vec3> Global_bone_positions;
	array1d<vec3> Global_bone_velocities;
	array1d<quat> Global_bone_rotations;
	array1d<vec3> Global_bone_angular_velocities;
	array1d<bool> Global_bone_computed;

	vec3 Transition_src_position;
	quat Transition_src_rotation;
	vec3 Transition_dst_position;
	quat Transition_dst_rotation;

	// Trajectory & Gameplay Data

	float Search_time = 0.1f;
	float Search_timer;
	float Force_search_timer;

	vec3 Desired_velocity;
	vec3 Desired_velocity_change_curr;
	vec3 Desired_velocity_change_prev;
	float Desired_velocity_change_threshold = 50.0;

	quat Desired_rotation;
	vec3 Desired_rotation_change_curr;
	vec3 Desired_rotation_change_prev;
	float Desired_rotation_change_threshold = 50.0;

	float Desired_gait = 0.0f;
	float Desired_gait_velocity = 0.0f;

	vec3 Simulation_position;
	vec3 Simulation_velocity;
	vec3 Simulation_acceleration;
	quat Simulation_rotation;
	vec3 Simulation_angular_velocity;

	float Simulation_velocity_halflife = 0.27f; //이 값들이 이 후 게임 로직 중에 수정되는지 아닌지 확인해 봐야함(수정된다면 여기서 값을 정의 하는것이 좋지 않을 듯함)
	float Simulation_rotation_halflife = 0.27f;


	// All speeds in m/s
	float Simulation_run_fwrd_speed = 4.0f;
	float Simulation_run_side_speed = 3.0f;
	float Simulation_run_back_speed = 2.5f;

	float Simulation_walk_fwrd_speed = 1.75f;
	float Simulation_walk_side_speed = 1.5f;
	float Simulation_walk_back_speed = 1.25f;

	array1d<vec3> Trajectory_desired_velocities = array1d<vec3>(4);
	array1d<quat> Trajectory_desired_rotations = array1d<quat>(4);
	array1d<vec3> Trajectory_positions = array1d<vec3>(4);
	array1d<vec3> Trajectory_velocities = array1d<vec3>(4);
	array1d<vec3> Trajectory_accelerations = array1d<vec3>(4);
	array1d<quat> Trajectory_rotations = array1d<quat>(4);
	array1d<vec3> Trajectory_angular_velocities = array1d<vec3>(4);

	// Synchronization
	bool Synchronization_enabled;
	float Synchronization_data_factor = 1.0f;

	// Adjustment
	bool Adjustment_enabled;
	bool Adjustment_by_velocity_enabled;
	float Adjustment_position_halflife = 0.1f;
	float Adjustment_rotation_halflife = 0.2f;
	float Adjustment_position_max_ratio = 0.5f;
	float Adjustment_rotation_max_ratio = 0.5f;

	// Clamping
	bool Clamping_enabled;
	float Clamping_max_distance = 0.15f;
	float Clamping_max_angle = 0.5f * PIf;

	// IK
	bool Ik_enabled;
	float Ik_max_length_buffer = 0.015f;
	float Ik_foot_height = 0.02f;
	float Ik_toe_length = 0.15f;
	float Ik_unlock_radius = 0.2f;
	float Ik_blending_halflife = 0.1f;

	// Contact and Foot Locking data
	array1d<int> Contact_bones = array1d<int>(2);

	array1d<bool> Contact_states = array1d<bool>(Contact_bones.size);
	array1d<bool> Contact_locks = array1d<bool>(Contact_bones.size);
	array1d<vec3> Contact_positions = array1d<vec3>(Contact_bones.size);
	array1d<vec3> Contact_velocities = array1d<vec3>(Contact_bones.size);
	array1d<vec3> Contact_points = array1d<vec3>(Contact_bones.size);
	array1d<vec3> Contact_targets = array1d<vec3>(Contact_bones.size);
	array1d<vec3> Contact_offset_positions = array1d<vec3>(Contact_bones.size);
	array1d<vec3> Contact_offset_velocities = array1d<vec3>(Contact_bones.size);

	array1d<vec3> Adjusted_bone_positions;
	array1d<quat> Adjusted_bone_rotations;


	// Learned Motion Matching
	bool LMM_enabled;

	nnet Decompressor, Stepper, Projector;
	nnet_evaluation Decompressor_evaluation, Stepper_evaluation, Projector_evaluation;

	array1d<float> Features_proj;
	array1d<float> Features_curr;
	array1d<float> Latent_proj = array1d<float>(32);
	array1d<float> Latent_curr = array1d<float>(32);

	//DeltTime(FrameRate)
	float DeltaT = 1.0f / 60.0f; //dt

	//Input Data
	UPROPERTY()
	bool Desired_strafe = false;



public: //poseblemesh 관련

	//SkeletalMesh대신 관절을 움직일수 있는 UPoseableMeshComponent 사용 할 예정.
	UPROPERTY(Category = Character, EditAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UPoseableMeshComponent> PoseableMesh;

	//UCharacter 클래스에 존재하는 GetMesh() 함수 재정의 -> PosealbeMeshComponent를 가져옴.
	UFUNCTION()
	virtual UPoseableMeshComponent* GetMesh() const;

	UFUNCTION()
	void PoseTest(int frameindex); //Pose test하기 위한 함수 //원하는 frame 값 넣으면 그에 해당하는 pose를 출력함

	UFUNCTION()
	void PoseTestByPostion(int frameindex); //Pose test하기 위한 함수 //원하는 frame 값 넣으면 그에 해당하는 pose를 출력함


public: //Motion Matching 관련
	

	//기본 베이스 및 내가 추가한 것
	UFUNCTION()
	void MotionMatchingMainBeginPlay(); //MotionMatching에 사용할 초기 데이터 load 함수 등이 포함됨(초기 변수 초기화)

	UFUNCTION()
	void MotionMatchingMainTick(); 

	UFUNCTION()
	void SaveBasicRotators(); //캐릭터 스켈레톤의 기본 Rotator 값 저장

	UFUNCTION()
	void SaveBasicVectors(); //캐릭터 스켈레톤의 기본 Vector 값 저장

	UFUNCTION()
	void DataBaseLog();

	UFUNCTION()
	void InputLog();


	FVector2D RightStickValue;
	FVector2D LeftStickValue;

	//-------------------------------------------------------------------
	//오렌지 덕의 controller.cpp에 정의되어 있는 함수들

	/** Called for looking input */
	void OnStrafe(const FInputActionValue& Value);

	void OffStrafe(const FInputActionValue& Value);

	void inertialize_pose_reset(
		slice1d<vec3> bone_offset_positions,
		slice1d<vec3> bone_offset_velocities,
		slice1d<quat> bone_offset_rotations,
		slice1d<vec3> bone_offset_angular_velocities,
		vec3& transition_src_position,
		quat& transition_src_rotation,
		vec3& transition_dst_position,
		quat& transition_dst_rotation,
		const vec3 root_position,
		const quat root_rotation);


	void inertialize_pose_update(
		slice1d<vec3> bone_positions,
		slice1d<vec3> bone_velocities,
		slice1d<quat> bone_rotations,
		slice1d<vec3> bone_angular_velocities,
		slice1d<vec3> bone_offset_positions,
		slice1d<vec3> bone_offset_velocities,
		slice1d<quat> bone_offset_rotations,
		slice1d<vec3> bone_offset_angular_velocities,
		const slice1d<vec3> bone_input_positions,
		const slice1d<vec3> bone_input_velocities,
		const slice1d<quat> bone_input_rotations,
		const slice1d<vec3> bone_input_angular_velocities,
		const vec3 transition_src_position,
		const quat transition_src_rotation,
		const vec3 transition_dst_position,
		const quat transition_dst_rotation,
		const float halflife,
		const float dt);


	void contact_reset(
		bool& contact_state,
		bool& contact_lock,
		vec3& contact_position,
		vec3& contact_velocity,
		vec3& contact_point,
		vec3& contact_target,
		vec3& contact_offset_position,
		vec3& contact_offset_velocity,
		const vec3 input_contact_position,
		const vec3 input_contact_velocity,
		const bool input_contact_state);


public:

	UFUNCTION()
	void CharacterLoadTest();

	UFUNCTION()
	void SetCharacterPositionRest(); //캐릭터의 position을 bone_rest_positions으로 설정

	UFUNCTION()
	void SetCharacterRotationRest(); //캐릭터의 rotation을 bone_rest_rotations으로 설정

};


//------------------------------------------------------------
enum
{
	GAMEPAD_PLAYER = 0,
};

enum
{
	GAMEPAD_STICK_LEFT, //0
	GAMEPAD_STICK_RIGHT, //1
};