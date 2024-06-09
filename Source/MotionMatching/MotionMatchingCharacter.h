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

	/** Zoom Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ZoomInAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ZoomOutAction;

	/** Gait Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* GaitAction;

	/** Menu Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MenuAction;


public:
	AMotionMatchingCharacter();
	

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	//------------------------------------------------
	/** Called for strafe input */
	void OnStrafe(const FInputActionValue& Value);

	void OffStrafe(const FInputActionValue& Value);

	/** Called for camera zoom input */
	void CamZoomInOn(const FInputActionValue& Value);

	void CamZoomInOff(const FInputActionValue& Value);

	void CamZoomOutOn(const FInputActionValue& Value);

	void CamZoomOutOff(const FInputActionValue& Value);

	/** Called for gait input */
	void GaitButtonOn(const FInputActionValue& Value);

	void GaitButtonOff(const FInputActionValue& Value);

	/** Called for MenuUI input */
	void TapKeyDown();

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay();

	// Tick �Լ�
	virtual void Tick(float DeltaTime) override;

	//���ʸ��� �̺�Ʈ�� �߻���Ű�� �ϱ� ���� Ÿ�̸�
	float TickTime;
	int TimePassed;


public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }


protected: //Motion Matching ���� variables

	//���̷����� �� Joint �̸��� ������ �迭
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


	//���̷����� �⺻ rotator �� ����
	TArray<FRotator> BasicCharatorRotator;
	//���̷����� �⺻ position(vector) �� ����
	TArray<FVector> BasicCharatorVector;

	float Camera_azimuth = 0.0f;
	float Camera_altitude = 0.4f * 100;
	float Camera_distance = 4.0f * 100;


	// Scene Obstacles
	//�ϴ� obstacle�� �������� �ʴ´ٰ� ������
	UPROPERTY()
	TArray<AActor*> OutActors;

	UPROPERTY(EditAnywhere, Category = "Obstacle")
	TSubclassOf<AActor> ActorClass;

	array1d<vec3> Obstacles_positions = array1d<vec3>(0);
	array1d<vec3> Obstacles_scales = array1d<vec3>(0);


	// Load Animation Data and build Matching Database
	database DB; //database.bin�� features.bin ��� DB�� �����

	//feature�� ������ �� ���Ǵ� weight�� ��
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

	//vec3 Simulation_position = vec3(10.0f, 0.32f, 0); //�ʱ� �� ���� //(x, y, z) -> (z, -x, y)�� ����
	vec3 Simulation_position = vec3(10.0f, 0.0f, 0); //�ʱ� �� ���� //(x, y, z) -> (z, -x, y)�� ����
	vec3 Simulation_velocity;
	vec3 Simulation_acceleration;
	quat Simulation_rotation;
	vec3 Simulation_angular_velocity;

	float Simulation_velocity_halflife = 0.27f;
	float Simulation_rotation_halflife = 0.27f;


	// All speeds in m/s
	//float Simulation_run_fwrd_speed = 4.0f;
	//float Simulation_run_side_speed = 3.0f;
	//float Simulation_run_back_speed = 2.5f;

	//float Simulation_walk_fwrd_speed = 1.75f;
	//float Simulation_walk_side_speed = 1.5f;
	//float Simulation_walk_back_speed = 1.25f;
	// 
	float Simulation_run_fwrd_speed = 4.0f * 2;
	float Simulation_run_side_speed = 3.0f * 2;
	float Simulation_run_back_speed = 2.5f * 2;

	float Simulation_walk_fwrd_speed = 1.75f * 2;
	float Simulation_walk_side_speed = 1.5f * 2;
	float Simulation_walk_back_speed = 1.25f * 2;

	//float Simulation_walk_fwrd_speed = 1.75f * 5;
	//float Simulation_walk_side_speed = 1.5f * 5;
	//float Simulation_walk_back_speed = 1.25f * 5;



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
	UPROPERTY(BlueprintReadWrite)
	bool Adjustment_enabled;
	bool Adjustment_by_velocity_enabled;
	float Adjustment_position_halflife = 0.1f;
	float Adjustment_rotation_halflife = 0.2f;
	float Adjustment_position_max_ratio = 0.5f;
	float Adjustment_rotation_max_ratio = 0.5f;

	// Clamping
	UPROPERTY(BlueprintReadWrite)
	bool Clamping_enabled;
	float Clamping_max_distance = 0.15f;
	float Clamping_max_angle = 0.5f * PIf;

	// IK
	UPROPERTY(BlueprintReadWrite)
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
	UPROPERTY(BlueprintReadWrite)
	bool LMM_enabled;

	nnet Decompressor, Stepper, Projector;
	nnet_evaluation Decompressor_evaluation, Stepper_evaluation, Projector_evaluation;

	array1d<float> Features_proj;
	array1d<float> Features_curr;
	array1d<float> Latent_proj = array1d<float>(32);
	array1d<float> Latent_curr = array1d<float>(32);

	//DeltTime(FrameRate)
	//float DeltaT = 1.0f / 60.0f; //dt
	float DeltaT = 1.0f / 60.0f; //dt


	//Input Data
	UPROPERTY()
	bool Desired_strafe = false;

	bool CamZoomIn = false;
	bool CamZoomOut = false;

	bool GaitButton = false;

	bool IsTabButtonDown = false; //Menu

	float CamRotationInputSpeed = 2;

	FVector2D RightStickValue2D;
	FVector2D LeftStickValue2D;



public: //poseblemesh ����

	//SkeletalMesh��� ������ �����ϼ� �ִ� UPoseableMeshComponent ��� �� ����.
	UPROPERTY(Category = Character, EditAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UPoseableMeshComponent> PoseableMesh;

	//UCharacter Ŭ������ �����ϴ� GetMesh() �Լ� ������ -> PosealbeMeshComponent�� ������.
	UFUNCTION()
	virtual UPoseableMeshComponent* GetMesh() const;

	UFUNCTION()
	void PoseTest(int frameindex); //Pose test�ϱ� ���� �Լ� //���ϴ� frame �� ������ �׿� �ش��ϴ� pose�� �����

	UFUNCTION()
	void PoseTestByPostion(int frameindex); //Pose test�ϱ� ���� �Լ� //���ϴ� frame �� ������ �׿� �ش��ϴ� pose�� �����


public: //Motion Matching ����
	
	UFUNCTION()
	void MotionMatchingMainBeginPlay(); //MotionMatching�� ����� �ʱ� ������ load �Լ� ���� ���Ե�(�ʱ� ���� �ʱ�ȭ)

	UFUNCTION()
	void MotionMatchingMainTick(); 

	void SetInputZero();

	UFUNCTION()
	void SaveBasicRotators(); //ĳ���� ���̷����� �⺻ Rotator �� ����

	UFUNCTION()
	void SaveBasicVectors(); //ĳ���� ���̷����� �⺻ Vector �� ����

	UFUNCTION()
	void DataBaseLog();

	UFUNCTION()
	void InputLog();
	
	//-------------------------------------------------------------------
	//Get obstacles informations
	void GetObstaclesinfo();


	//Set Simulation object, Set Animation
	void SetSimulationObj();
	void SetCharacterAnimation();

	//Draw Debug spheres
	FVector To_Vector3(vec3 v);

	void Draw_features(const slice1d<float> features, const vec3 pos, const quat rot, FColor color);
	void Draw_trajectory(const slice1d<vec3> trajectory_positions, const slice1d<quat> trajectory_rotations, FColor color);
	void Draw_simulation_object(const vec3 simulation_position, quat simulation_rotation, FColor color);

	//-------------------------------------------------------------------
	float orbit_camera_update_azimuth(
		const float azimuth,
		const vec3 gamepadstick_right,
		const bool desired_strafe,
		const float dt);

	float orbit_camera_update_altitude(
		const float altitude,
		const vec3 gamepadstick_right,
		const bool desired_strafe,
		const float dt);

	float orbit_camera_update_distance(
		const float distance,
		const float dt);


	void orbit_camera_update(
		//Camera3D& cam,
		float& camera_azimuth,
		float& camera_altitude,
		float& camera_distance,
		//const vec3 target,
		const vec3 gamepadstick_right,
		const bool desired_strafe,
		const float dt
	); //�𸮾� ������ �°� ���Ӱ� ��������


	void desired_gait_update(
		float& desired_gait,
		float& desired_gait_velocity,
		const float dt,
		const float gait_change_halflife);

	vec3 desired_velocity_update(
		const vec3 gamepadstick_left,
		const float camera_azimuth,
		const quat simulation_rotation,
		const float fwrd_speed,
		const float side_speed,
		const float back_speed);

	quat desired_rotation_update(
		const quat desired_rotation,
		const vec3 gamepadstick_left,
		const vec3 gamepadstick_right,
		const float camera_azimuth,
		const bool desired_strafe,
		const vec3 desired_velocity);

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

	void inertialize_pose_transition(
		slice1d<vec3> bone_offset_positions,
		slice1d<vec3> bone_offset_velocities,
		slice1d<quat> bone_offset_rotations,
		slice1d<vec3> bone_offset_angular_velocities,
		vec3& transition_src_position,
		quat& transition_src_rotation,
		vec3& transition_dst_position,
		quat& transition_dst_rotation,
		const vec3 root_position,
		const vec3 root_velocity,
		const quat root_rotation,
		const vec3 root_angular_velocity,
		const slice1d<vec3> bone_src_positions,
		const slice1d<vec3> bone_src_velocities,
		const slice1d<quat> bone_src_rotations,
		const slice1d<vec3> bone_src_angular_velocities,
		const slice1d<vec3> bone_dst_positions,
		const slice1d<vec3> bone_dst_velocities,
		const slice1d<quat> bone_dst_rotations,
		const slice1d<vec3> bone_dst_angular_velocities);


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

	//--------------------------------------
	// Copy a part of a feature vector from the 
	// matching database into the query feature vector
	void query_copy_denormalized_feature(
		slice1d<float> query,
		int& offset,
		const int size,
		const slice1d<float> features,
		const slice1d<float> features_offset,
		const slice1d<float> features_scale);
	
	// Compute the query feature vector for the current 
	// trajectory controlled by the gamepad.
	void query_compute_trajectory_position_feature(
		slice1d<float> query,
		int& offset,
		const vec3 root_position,
		const quat root_rotation,
		const slice1d<vec3> trajectory_positions);

	// Same but for the trajectory direction
	void query_compute_trajectory_direction_feature(
		slice1d<float> query,
		int& offset,
		const quat root_rotation,
		const slice1d<quat> trajectory_rotations);
	

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

	void contact_update(
		bool& contact_state,
		bool& contact_lock,
		vec3& contact_position,
		vec3& contact_velocity,
		vec3& contact_point,
		vec3& contact_target,
		vec3& contact_offset_position,
		vec3& contact_offset_velocity,
		const vec3 input_contact_position,
		const bool input_contact_state,
		const float unlock_radius,
		const float foot_height,
		const float halflife,
		const float dt,
		const float eps);

	void ik_look_at(
		quat& bone_rotation,
		const quat global_parent_rotation,
		const quat global_rotation,
		const vec3 global_position,
		const vec3 child_position,
		const vec3 target_position,
		const float eps);

	void ik_two_bone(
		quat& bone_root_lr,
		quat& bone_mid_lr,
		const vec3 bone_root,
		const vec3 bone_mid,
		const vec3 bone_end,
		const vec3 target,
		const vec3 fwd,
		const quat bone_root_gr,
		const quat bone_mid_gr,
		const quat bone_par_gr,
		const float max_length_buffer);


	vec3 simulation_collide_obstacles(
		const vec3 prev_pos,
		const vec3 next_pos,
		const slice1d<vec3> obstacles_positions,
		const slice1d<vec3> obstacles_scales,
		const float radius);


	void simulation_positions_update(
		vec3& position,
		vec3& velocity,
		vec3& acceleration,
		const vec3 desired_velocity,
		const float halflife,
		const float dt,
		const slice1d<vec3> obstacles_positions,
		const slice1d<vec3> obstacles_scales);

	void simulation_rotations_update(
		quat& rotation,
		vec3& angular_velocity,
		const quat desired_rotation,
		const float halflife,
		const float dt);

	void trajectory_desired_rotations_predict(
		slice1d<quat> desired_rotations,
		const slice1d<vec3> desired_velocities,
		const quat desired_rotation,
		const float camera_azimuth,
		const vec3 gamepadstick_left,
		const vec3 gamepadstick_right,
		const bool desired_strafe,
		const float dt);

	void trajectory_rotations_predict(
		slice1d<quat> rotations,
		slice1d<vec3> angular_velocities,
		const quat rotation,
		const vec3 angular_velocity,
		const slice1d<quat> desired_rotations,
		const float halflife,
		const float dt);

	void trajectory_positions_predict(
		slice1d<vec3> positions,
		slice1d<vec3> velocities,
		slice1d<vec3> accelerations,
		const vec3 position,
		const vec3 velocity,
		const vec3 acceleration,
		const slice1d<vec3> desired_velocities,
		const float halflife,
		const float dt,
		const slice1d<vec3> obstacles_positions,
		const slice1d<vec3> obstacles_scales);

	void trajectory_desired_velocities_predict(
		slice1d<vec3> desired_velocities,
		const slice1d<quat> trajectory_rotations,
		const vec3 desired_velocity,
		const float camera_azimuth,
		const vec3 gamepadstick_left,
		const vec3 gamepadstick_right,
		const bool desired_strafe,
		const float fwrd_speed,
		const float side_speed,
		const float back_speed,
		const float dt);

	vec3 adjust_character_position(
		const vec3 character_position,
		const vec3 simulation_position,
		const float halflife,
		const float dt);

	quat adjust_character_rotation(
		const quat character_rotation,
		const quat simulation_rotation,
		const float halflife,
		const float dt);

	vec3 adjust_character_position_by_velocity(
		const vec3 character_position,
		const vec3 character_velocity,
		const vec3 simulation_position,
		const float max_adjustment_ratio,
		const float halflife,
		const float dt);

	quat adjust_character_rotation_by_velocity(
		const quat character_rotation,
		const vec3 character_angular_velocity,
		const quat simulation_rotation,
		const float max_adjustment_ratio,
		const float halflife,
		const float dt);

	vec3 clamp_character_position(
		const vec3 character_position,
		const vec3 simulation_position,
		const float max_distance);

	quat clamp_character_rotation(
		const quat character_rotation,
		const quat simulation_rotation,
		const float max_angle);


public:

	UFUNCTION()
	void CharacterLoadTest();

	UFUNCTION()
	void SetCharacterPositionRest(); //ĳ������ position�� bone_rest_positions���� ����

	UFUNCTION()
	void SetCharacterRotationRest(); //ĳ������ rotation�� bone_rest_rotations���� ����


};
