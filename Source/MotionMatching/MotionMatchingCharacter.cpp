// Copyright Epic Games, Inc. All Rights Reserved.

#include "MotionMatchingCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

#include "Kismet/GameplayStatics.h" //GetPlyaerController 함수 불러오려고 추가함

#include "Components/PoseableMeshComponent.h"



DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AMotionMatchingCharacter

AMotionMatchingCharacter::AMotionMatchingCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm


	//PoseableMesh를 생성하고 RootComponent에 종속 시킴.
	PoseableMesh = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("PoseableMesh"));
	PoseableMesh->SetupAttachment(RootComponent);



	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AMotionMatchingCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	// Tick 함수 등록
	PrimaryActorTick.bCanEverTick = true;

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}



	//---------------------------------------------------

	TickTime = 0.0f; //Tick timer를 0으로 초기화
	TimePassed = 0;
	
	//Character skeleton의 기본 roator 값을 배열로 저장
	SaveBasicRotators();
	//Character skeleton의 기본 vector 값을 배열로 저장
	SaveBasicVectors();

	//MotionMatching BeginPlay
	MotionMatchingMainBeginPlay();

	//SetCharacterPositionRest();
	SetCharacterRotationRest();

	//DataBase Log
	DataBaseLog();

}


void AMotionMatchingCharacter::Tick(float DeltaTime) {

	Super::Tick(DeltaTime);

	//-------------------------------------------------------------
	//Tick 함수가 잘 작동하는지 테스트
	//초 단위로 출력
	TickTime += DeltaTime;
	if (TickTime >= 1) {
		UE_LOG(LogTemp, Log, TEXT("%d Seconds has passed."), TimePassed);
		TimePassed += 1;
		TickTime = 0.0f;
	}
	//-------------------------------------------------------------
	//Input이 잘 작동하는지 테스트
	InputLog();

}



//////////////////////////////////////////////////////////////////////////
// Input

void AMotionMatchingCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMotionMatchingCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMotionMatchingCharacter::Look);

		// Strafe
		EnhancedInputComponent->BindAction(StrafeAction, ETriggerEvent::Started, this, &AMotionMatchingCharacter::OnStrafe);
		EnhancedInputComponent->BindAction(StrafeAction, ETriggerEvent::Completed, this, &AMotionMatchingCharacter::OffStrafe);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AMotionMatchingCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}


	// Gamepad left stick vector 2d value
	LeftStickValue2D = Value.Get<FVector2D>();
}

void AMotionMatchingCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}


	// Gamepad Right stick vector 2d value
	RightStickValue2D = Value.Get<FVector2D>();
}




//---------------------------------------------------------------------
//<사용자 정의 함수 정의 부분>



//UCharacter 클래스에 존재하는 GetMesh() 함수 재정의 -> PosealbeMeshComponent를 가져옴.
UPoseableMeshComponent* AMotionMatchingCharacter::GetMesh() const
{
	return PoseableMesh;
}




//---------------------------------------------------------------------
//<controller.cpp에 정의되어 있는 함수들>

float AMotionMatchingCharacter::orbit_camera_update_azimuth(
	const float azimuth,
	const vec3 gamepadstick_right,
	const bool desired_strafe,
	const float dt)
{
	vec3 gamepadaxis = desired_strafe ? vec3() : gamepadstick_right;
	return azimuth + 2.0f * dt * -gamepadaxis.x;
}


void AMotionMatchingCharacter::OnStrafe(const FInputActionValue& Value)
{
	Desired_strafe = true;
}

void AMotionMatchingCharacter::OffStrafe(const FInputActionValue& Value)
{
	Desired_strafe = false;
}

void AMotionMatchingCharacter::desired_gait_update(
	float& desired_gait,
	float& desired_gait_velocity,
	const float dt,
	const float gait_change_halflife = 0.1f)
{
	simple_spring_damper_exact( //MMspring.h에서 구현
		desired_gait,
		desired_gait_velocity,
		Cast<APlayerController>(Controller)->IsInputKeyDown(EKeys::A) ? 1.0f : 0.0f,
		gait_change_halflife,
		dt);
}

vec3 AMotionMatchingCharacter::desired_velocity_update(
	const vec3 gamepadstick_left,
	const float camera_azimuth,
	const quat simulation_rotation,
	const float fwrd_speed,
	const float side_speed,
	const float back_speed)
{
	// Find stick position in world space by rotating using camera azimuth
	vec3 global_stick_direction = quat_mul_vec3(
		quat_from_angle_axis(camera_azimuth, vec3(0, 1, 0)), gamepadstick_left);

	// Find stick position local to current facing direction
	vec3 local_stick_direction = quat_inv_mul_vec3(
		simulation_rotation, global_stick_direction);

	// Scale stick by forward, sideways and backwards speeds
	vec3 local_desired_velocity = local_stick_direction.z > 0.0 ?
		vec3(side_speed, 0.0f, fwrd_speed) * local_stick_direction :
		vec3(side_speed, 0.0f, back_speed) * local_stick_direction;

	// Re-orientate into the world space
	return quat_mul_vec3(simulation_rotation, local_desired_velocity);
}

quat AMotionMatchingCharacter::desired_rotation_update(
	const quat desired_rotation,
	const vec3 gamepadstick_left,
	const vec3 gamepadstick_right,
	const float camera_azimuth,
	const bool desired_strafe,
	const vec3 desired_velocity)
{
	quat desired_rotation_curr = desired_rotation;

	// If strafe is active then desired direction is coming from right
	// stick as long as that stick is being used, otherwise we assume
	// forward facing
	if (desired_strafe)
	{
		vec3 desired_direction = quat_mul_vec3(quat_from_angle_axis(camera_azimuth, vec3(0, 1, 0)), vec3(0, 0, -1));

		if (length(gamepadstick_right) > 0.01f)
		{
			desired_direction = quat_mul_vec3(quat_from_angle_axis(camera_azimuth, vec3(0, 1, 0)), normalize(gamepadstick_right));
		}

		return quat_from_angle_axis(atan2f(desired_direction.x, desired_direction.z), vec3(0, 1, 0));
	}

	// If strafe is not active the desired direction comes from the left 
	// stick as long as that stick is being used
	else if (length(gamepadstick_left) > 0.01f)
	{

		vec3 desired_direction = normalize(desired_velocity);
		return quat_from_angle_axis(atan2f(desired_direction.x, desired_direction.z), vec3(0, 1, 0));
	}

	// Otherwise desired direction remains the same
	else
	{
		return desired_rotation_curr;
	}
}


void AMotionMatchingCharacter::inertialize_pose_reset(
	slice1d<vec3> bone_offset_positions,
	slice1d<vec3> bone_offset_velocities,
	slice1d<quat> bone_offset_rotations,
	slice1d<vec3> bone_offset_angular_velocities,
	vec3& transition_src_position,
	quat& transition_src_rotation,
	vec3& transition_dst_position,
	quat& transition_dst_rotation,
	const vec3 root_position,
	const quat root_rotation)
{
	bone_offset_positions.zero();
	bone_offset_velocities.zero();
	bone_offset_rotations.set(quat());
	bone_offset_angular_velocities.zero();

	transition_src_position = root_position;
	transition_src_rotation = root_rotation;
	transition_dst_position = vec3();
	transition_dst_rotation = quat();
}



// This function updates the inertializer states. Here 
// it outputs the smoothed animation (input plus offset) 
// as well as updating the offsets themselves. It takes 
// as input the current playing animation as well as the 
// root transition locations, a halflife, and a dt
void AMotionMatchingCharacter::inertialize_pose_update(
	slice1d<vec3> bone_positions,
	slice1d<vec3> bone_velocities,
	slice1d<quat> bone_rotations,
	slice1d<vec3> bone_angular_velocities,
	slice1d<vec3> bone_offset_positions,
	slice1d<vec3> bone_offset_velocities,
	slice1d<quat> bone_offset_rotations,
	slice1d<vec3> bone_offset_angular_velocities,
	const slice1d<vec3> bone_input_positions, //input에 대한 정보가 필요함
	const slice1d<vec3> bone_input_velocities,
	const slice1d<quat> bone_input_rotations,
	const slice1d<vec3> bone_input_angular_velocities,
	const vec3 transition_src_position,
	const quat transition_src_rotation,
	const vec3 transition_dst_position,
	const quat transition_dst_rotation,
	const float halflife,
	const float dt)
{
	// First we find the next root position, velocity, rotation
	// and rotational velocity in the world space by transforming 
	// the input animation from it's animation space into the 
	// space of the currently playing animation.
	vec3 world_space_position = quat_mul_vec3(transition_dst_rotation,
		quat_inv_mul_vec3(transition_src_rotation,
			bone_input_positions(0) - transition_src_position)) + transition_dst_position;

	vec3 world_space_velocity = quat_mul_vec3(transition_dst_rotation,
		quat_inv_mul_vec3(transition_src_rotation, bone_input_velocities(0)));

	// Normalize here because quat inv mul can sometimes produce 
	// unstable returns when the two rotations are very close.
	quat world_space_rotation = quat_normalize(quat_mul(transition_dst_rotation,
		quat_inv_mul(transition_src_rotation, bone_input_rotations(0))));

	vec3 world_space_angular_velocity = quat_mul_vec3(transition_dst_rotation,
		quat_inv_mul_vec3(transition_src_rotation, bone_input_angular_velocities(0)));

	// Then we update these two inertializers with these new world space inputs
	inertialize_update( //MMspring.h에 정의되어 있음
		bone_positions(0),
		bone_velocities(0),
		bone_offset_positions(0),
		bone_offset_velocities(0),
		world_space_position,
		world_space_velocity,
		halflife,
		dt);

	inertialize_update( //MMspring.h에 정의되어 있음
		bone_rotations(0),
		bone_angular_velocities(0),
		bone_offset_rotations(0),
		bone_offset_angular_velocities(0),
		world_space_rotation,
		world_space_angular_velocity,
		halflife,
		dt);

	// Then we update the inertializers for the rest of the bones
	for (int i = 1; i < bone_positions.size; i++)
	{
		inertialize_update(
			bone_positions(i),
			bone_velocities(i),
			bone_offset_positions(i),
			bone_offset_velocities(i),
			bone_input_positions(i),
			bone_input_velocities(i),
			halflife,
			dt);

		inertialize_update(
			bone_rotations(i),
			bone_angular_velocities(i),
			bone_offset_rotations(i),
			bone_offset_angular_velocities(i),
			bone_input_rotations(i),
			bone_input_angular_velocities(i),
			halflife,
			dt);
	}
}

//--------------------------------------

// Copy a part of a feature vector from the 
// matching database into the query feature vector
void AMotionMatchingCharacter::query_copy_denormalized_feature(
	slice1d<float> query,
	int& offset,
	const int size,
	const slice1d<float> features,
	const slice1d<float> features_offset,
	const slice1d<float> features_scale)
{
	for (int i = 0; i < size; i++)
	{
		query(offset + i) = features(offset + i) * features_scale(offset + i) + features_offset(offset + i);
	}

	offset += size;
}

// Compute the query feature vector for the current 
// trajectory controlled by the gamepad.
void AMotionMatchingCharacter::query_compute_trajectory_position_feature(
	slice1d<float> query,
	int& offset,
	const vec3 root_position,
	const quat root_rotation,
	const slice1d<vec3> trajectory_positions)
{
	vec3 traj0 = quat_inv_mul_vec3(root_rotation, trajectory_positions(1) - root_position);
	vec3 traj1 = quat_inv_mul_vec3(root_rotation, trajectory_positions(2) - root_position);
	vec3 traj2 = quat_inv_mul_vec3(root_rotation, trajectory_positions(3) - root_position);

	query(offset + 0) = traj0.x;
	query(offset + 1) = traj0.z;
	query(offset + 2) = traj1.x;
	query(offset + 3) = traj1.z;
	query(offset + 4) = traj2.x;
	query(offset + 5) = traj2.z;

	offset += 6;
}

// Same but for the trajectory direction
void AMotionMatchingCharacter::query_compute_trajectory_direction_feature(
	slice1d<float> query,
	int& offset,
	const quat root_rotation,
	const slice1d<quat> trajectory_rotations)
{
	vec3 traj0 = quat_inv_mul_vec3(root_rotation, quat_mul_vec3(trajectory_rotations(1), vec3(0, 0, 1)));
	vec3 traj1 = quat_inv_mul_vec3(root_rotation, quat_mul_vec3(trajectory_rotations(2), vec3(0, 0, 1)));
	vec3 traj2 = quat_inv_mul_vec3(root_rotation, quat_mul_vec3(trajectory_rotations(3), vec3(0, 0, 1)));

	query(offset + 0) = traj0.x;
	query(offset + 1) = traj0.z;
	query(offset + 2) = traj1.x;
	query(offset + 3) = traj1.z;
	query(offset + 4) = traj2.x;
	query(offset + 5) = traj2.z;

	offset += 6;
}


void AMotionMatchingCharacter::contact_reset(
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
	const bool input_contact_state)
{
	contact_state = false;
	contact_lock = false;
	contact_position = input_contact_position;
	contact_velocity = input_contact_velocity;
	contact_point = input_contact_position;
	contact_target = input_contact_position;
	contact_offset_position = vec3();
	contact_offset_velocity = vec3();
}

//--------------------------------------

// Collide against the obscales which are
// essentially bounding boxes of a given size
vec3 AMotionMatchingCharacter::simulation_collide_obstacles(
	const vec3 prev_pos,
	const vec3 next_pos,
	const slice1d<vec3> obstacles_positions,
	const slice1d<vec3> obstacles_scales,
	const float radius = 0.6f)
{
	vec3 dx = next_pos - prev_pos;
	vec3 proj_pos = prev_pos;

	// Substep because I'm too lazy to implement CCD
	int substeps = 1 + (int)(length(dx) * 5.0f);

	for (int j = 0; j < substeps; j++)
	{
		proj_pos = proj_pos + dx / substeps;

		for (int i = 0; i < obstacles_positions.size; i++)
		{
			// Find nearest point inside obscale and push out
			vec3 nearest = clamp(proj_pos,
				obstacles_positions(i) - 0.5f * obstacles_scales(i),
				obstacles_positions(i) + 0.5f * obstacles_scales(i));

			if (length(nearest - proj_pos) < radius)
			{
				proj_pos = radius * normalize(proj_pos - nearest) + nearest;
			}
		}
	}

	return proj_pos;
}

void AMotionMatchingCharacter::simulation_positions_update(
	vec3& position,
	vec3& velocity,
	vec3& acceleration,
	const vec3 desired_velocity,
	const float halflife,
	const float dt,
	const slice1d<vec3> obstacles_positions,
	const slice1d<vec3> obstacles_scales)
{
	float y = halflife_to_damping(halflife) / 2.0f;
	vec3 j0 = velocity - desired_velocity;
	vec3 j1 = acceleration + j0 * y;
	float eydt = fast_negexpf(y * dt);

	vec3 position_prev = position;

	position = eydt * (((-j1) / (y * y)) + ((-j0 - j1 * dt) / y)) +
		(j1 / (y * y)) + j0 / y + desired_velocity * dt + position_prev;
	velocity = eydt * (j0 + j1 * dt) + desired_velocity;
	acceleration = eydt * (acceleration - j1 * y * dt);

	position = simulation_collide_obstacles(
		position_prev,
		position,
		obstacles_positions,
		obstacles_scales);
}



void AMotionMatchingCharacter::simulation_rotations_update(
	quat& rotation,
	vec3& angular_velocity,
	const quat desired_rotation,
	const float halflife,
	const float dt)
{
	simple_spring_damper_exact(
		rotation,
		angular_velocity,
		desired_rotation,
		halflife, dt);
}

// Predict what the desired velocity will be in the 
// future. Here we need to use the future trajectory 
// rotation as well as predicted future camera 
// position to find an accurate desired velocity in 
// the world space
void AMotionMatchingCharacter::trajectory_desired_velocities_predict(
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
	const float dt)
{
	desired_velocities(0) = desired_velocity;

	for (int i = 1; i < desired_velocities.size; i++)
	{
		desired_velocities(i) = desired_velocity_update(
			gamepadstick_left,
			orbit_camera_update_azimuth(
				camera_azimuth, gamepadstick_right, desired_strafe, i * dt),
			trajectory_rotations(i),
			fwrd_speed,
			side_speed,
			back_speed);
	}
}



// Predict desired rotations given the estimated future 
// camera rotation and other parameters
void AMotionMatchingCharacter::trajectory_desired_rotations_predict(
	slice1d<quat> desired_rotations,
	const slice1d<vec3> desired_velocities,
	const quat desired_rotation,
	const float camera_azimuth,
	const vec3 gamepadstick_left,
	const vec3 gamepadstick_right,
	const bool desired_strafe,
	const float dt)
{
	desired_rotations(0) = desired_rotation;

	for (int i = 1; i < desired_rotations.size; i++)
	{
		desired_rotations(i) = desired_rotation_update(
			desired_rotations(i - 1),
			gamepadstick_left,
			gamepadstick_right,
			orbit_camera_update_azimuth(
				camera_azimuth, gamepadstick_right, desired_strafe, i * dt),
			desired_strafe,
			desired_velocities(i));
	}
}

void AMotionMatchingCharacter::trajectory_rotations_predict(
	slice1d<quat> rotations,
	slice1d<vec3> angular_velocities,
	const quat rotation,
	const vec3 angular_velocity,
	const slice1d<quat> desired_rotations,
	const float halflife,
	const float dt)
{
	rotations.set(rotation);
	angular_velocities.set(angular_velocity);

	for (int i = 1; i < rotations.size; i++)
	{
		simulation_rotations_update(
			rotations(i),
			angular_velocities(i),
			desired_rotations(i),
			halflife,
			i * dt);
	}
}

void AMotionMatchingCharacter::trajectory_positions_predict( //obstacle position이 문제가 됨
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
	const slice1d<vec3> obstacles_scales)
{
	positions(0) = position;
	velocities(0) = velocity;
	accelerations(0) = acceleration;

	for (int i = 1; i < positions.size; i++)
	{
		positions(i) = positions(i - 1);
		velocities(i) = velocities(i - 1);
		accelerations(i) = accelerations(i - 1);

		simulation_positions_update(
			positions(i),
			velocities(i),
			accelerations(i),
			desired_velocities(i),
			halflife,
			dt,
			obstacles_positions,
			obstacles_scales);
	}
}




//---------------------------------------------------------------------

void AMotionMatchingCharacter::MotionMatchingMainBeginPlay() {


	// Character
	
	FString CharacterFilePath = FPaths::ProjectContentDir() + TEXT("/character.bin");
	const char* CharacterFilePathChar = TCHAR_TO_ANSI(*CharacterFilePath); 	// TCHAR_TO_ANSI 매크로를 사용하여 변환
	character_load(Character_data, CharacterFilePathChar);


	// Load Animation Data

	FString DatabaseFilePath = FPaths::ProjectContentDir() + TEXT("/database.bin");
	const char* DatabaseFilePathChar = TCHAR_TO_ANSI(*DatabaseFilePath); 	// TCHAR_TO_ANSI 매크로를 사용하여 변환
	database_load(DB, DatabaseFilePathChar);

	// build Matching Database
	database_build_matching_features(
		DB,
		Feature_weight_foot_position,
		Feature_weight_foot_velocity,
		Feature_weight_hip_velocity,
		Feature_weight_trajectory_positions,
		Feature_weight_trajectory_directions);


	FString FeaturesFilePath = FPaths::ProjectContentDir() + TEXT("/features.bin");
	const char* FeaturesFilePathChar = TCHAR_TO_ANSI(*FeaturesFilePath); 	// TCHAR_TO_ANSI 매크로를 사용하여 변환
	database_save_matching_features(DB, FeaturesFilePathChar);


	// Pose & Inertializer Data

	Frame_index = DB.range_starts(0);

	Curr_bone_positions = DB.bone_positions(Frame_index);
	Curr_bone_velocities = DB.bone_velocities(Frame_index);
	Curr_bone_rotations = DB.bone_rotations(Frame_index);
	Curr_bone_angular_velocities = DB.bone_angular_velocities(Frame_index);
	Curr_bone_contacts = DB.contact_states(Frame_index);

	Trns_bone_positions = DB.bone_positions(Frame_index);
	Trns_bone_velocities = DB.bone_velocities(Frame_index);
	Trns_bone_rotations = DB.bone_rotations(Frame_index);
	Trns_bone_angular_velocities = DB.bone_angular_velocities(Frame_index);
	Trns_bone_contacts = DB.contact_states(Frame_index);

	Bone_positions = DB.bone_positions(Frame_index);
	Bone_velocities = DB.bone_velocities(Frame_index);
	Bone_rotations = DB.bone_rotations(Frame_index);
	Bone_angular_velocities = DB.bone_angular_velocities(Frame_index); 

	Bone_offset_positions = array1d<vec3>(DB.nbones());
	Bone_offset_velocities = array1d<vec3>(DB.nbones());
	Bone_offset_rotations = array1d<quat>(DB.nbones());
	Bone_offset_angular_velocities = array1d<vec3>(DB.nbones());

	Global_bone_positions = array1d<vec3>(DB.nbones());
	Global_bone_velocities = array1d<vec3>(DB.nbones());
	Global_bone_rotations = array1d<quat>(DB.nbones());
	Global_bone_angular_velocities = array1d<vec3>(DB.nbones());
	Global_bone_computed = array1d<bool>(DB.nbones());

	inertialize_pose_reset(
		Bone_offset_positions,
		Bone_offset_velocities,
		Bone_offset_rotations,
		Bone_offset_angular_velocities,
		Transition_src_position,
		Transition_src_rotation,
		Transition_dst_position,
		Transition_dst_rotation,
		Bone_positions(0),
		Bone_rotations(0));

	inertialize_pose_update(
		Bone_positions,
		Bone_velocities,
		Bone_rotations,
		Bone_angular_velocities,
		Bone_offset_positions,
		Bone_offset_velocities,
		Bone_offset_rotations,
		Bone_offset_angular_velocities,
		DB.bone_positions(Frame_index),
		DB.bone_velocities(Frame_index),
		DB.bone_rotations(Frame_index),
		DB.bone_angular_velocities(Frame_index),
		Transition_src_position,
		Transition_src_rotation,
		Transition_dst_position,
		Transition_dst_rotation,
		Inertialize_blending_halflife,
		0.0f);


	// Trajectory & Gameplay Data
	Search_timer = Search_time;
	Force_search_timer = Search_time;

	// Synchronization
	Synchronization_enabled = false;

	// Adjustment
	Adjustment_enabled = true;
	Adjustment_by_velocity_enabled = true;

	// Clamping
	Clamping_enabled = true;

	// IK
	Ik_enabled = true;

	// Contact and Foot Locking data
	Contact_bones(0) = Bone_LeftToe; //Bone_LegtToe 는 관절 index 값임(enum)
	Contact_bones(1) = Bone_RightToe;

	for (int i = 0; i < Contact_bones.size; i++)
	{
		vec3 bone_position;
		vec3 bone_velocity;
		quat bone_rotation;
		vec3 bone_angular_velocity;

		forward_kinematics_velocity( //MMdatabase.cpp에 정의되어 있음
			bone_position,
			bone_velocity,
			bone_rotation,
			bone_angular_velocity,
			Bone_positions,
			Bone_velocities,
			Bone_rotations,
			Bone_angular_velocities,
			DB.bone_parents,
			Contact_bones(i));

		contact_reset( //멤버 함수임
			Contact_states(i),
			Contact_locks(i),
			Contact_positions(i),
			Contact_velocities(i),
			Contact_points(i),
			Contact_targets(i),
			Contact_offset_positions(i),
			Contact_offset_velocities(i),
			bone_position,
			bone_velocity,
			false);
	}

	Adjusted_bone_positions = Bone_positions;
	Adjusted_bone_rotations = Bone_rotations;


	// Learned Motion Matching
	LMM_enabled = false;


	FString DecompressorPath = FPaths::ProjectContentDir() + TEXT("/decompressor.bin");
	FString StepperPath = FPaths::ProjectContentDir() + TEXT("/stepper.bin");
	FString ProjectorPath = FPaths::ProjectContentDir() + TEXT("/projector.bin");

	nnet_load(Decompressor, TCHAR_TO_ANSI(*DecompressorPath));
	nnet_load(Stepper, TCHAR_TO_ANSI(*StepperPath));
	nnet_load(Projector, TCHAR_TO_ANSI(*ProjectorPath));

	Decompressor_evaluation.resize(Decompressor);
	Stepper_evaluation.resize(Stepper);
	Projector_evaluation.resize(Projector);

	Features_proj = DB.features(Frame_index);
	Features_curr = DB.features(Frame_index);
	Latent_proj.zero();
	Latent_curr.zero();




}

void AMotionMatchingCharacter::MotionMatchingMainTick() {

	// Get gamepad stick states
	vec3 gamepadstick_left = vec3(LeftStickValue2D.X, 0.0f, LeftStickValue2D.Y);
	vec3 gamepadstick_right = vec3(RightStickValue2D.X, 0.0f, RightStickValue2D.Y);

	// Get the desired gait (walk / run)
	desired_gait_update(
		Desired_gait,
		Desired_gait_velocity,
		DeltaT);

	// Get the desired simulation speeds based on the gait
	float simulation_fwrd_speed = lerpf(Simulation_run_fwrd_speed, Simulation_walk_fwrd_speed, Desired_gait);
	float simulation_side_speed = lerpf(Simulation_run_side_speed, Simulation_walk_side_speed, Desired_gait);
	float simulation_back_speed = lerpf(Simulation_run_back_speed, Simulation_walk_back_speed, Desired_gait);

	// Get the desired velocity
	vec3 desired_velocity_curr = desired_velocity_update(
		gamepadstick_left,
		Camera_azimuth,
		Simulation_rotation,
		simulation_fwrd_speed,
		simulation_side_speed,
		simulation_back_speed);

	// Get the desired rotation/direction
	quat desired_rotation_curr = desired_rotation_update(
		Desired_rotation,
		gamepadstick_left,
		gamepadstick_right,
		Camera_azimuth,
		Desired_strafe,
		desired_velocity_curr);

	// Check if we should force a search because input changed quickly
	Desired_velocity_change_prev = Desired_velocity_change_curr;
	Desired_velocity_change_curr = (desired_velocity_curr - Desired_velocity) / DeltaT;
	Desired_velocity = desired_velocity_curr;

	Desired_rotation_change_prev = Desired_rotation_change_curr;
	Desired_rotation_change_curr = quat_to_scaled_angle_axis(quat_abs(quat_mul_inv(desired_rotation_curr, Desired_rotation))) / DeltaT;
	Desired_rotation = desired_rotation_curr;

	bool force_search = false;

	if (Force_search_timer <= 0.0f && (
		(length(Desired_velocity_change_prev) >= Desired_velocity_change_threshold &&
			length(Desired_velocity_change_curr) < Desired_velocity_change_threshold)
		|| (length(Desired_rotation_change_prev) >= Desired_rotation_change_threshold &&
			length(Desired_rotation_change_curr) < Desired_rotation_change_threshold)))
	{
		force_search = true;
		Force_search_timer = Search_time;
	}
	else if (Force_search_timer > 0)
	{
		Force_search_timer -= DeltaT;
	}

	// Predict Future Trajectory
	trajectory_desired_rotations_predict(
		Trajectory_desired_rotations,
		Trajectory_desired_velocities,
		Desired_rotation,
		Camera_azimuth,
		gamepadstick_left,
		gamepadstick_right,
		Desired_strafe,
		20.0f * DeltaT);

	trajectory_rotations_predict(
		Trajectory_rotations,
		Trajectory_angular_velocities,
		Simulation_rotation,
		Simulation_angular_velocity,
		Trajectory_desired_rotations,
		Simulation_rotation_halflife,
		20.0f * DeltaT);

	trajectory_desired_velocities_predict(
		Trajectory_desired_velocities,
		Trajectory_rotations,
		Desired_velocity,
		Camera_azimuth,
		gamepadstick_left,
		gamepadstick_right,
		Desired_strafe,
		simulation_fwrd_speed,
		simulation_side_speed,
		simulation_back_speed,
		20.0f * DeltaT);

	trajectory_positions_predict(
		Trajectory_positions,
		Trajectory_velocities,
		Trajectory_accelerations,
		Simulation_position,
		Simulation_velocity,
		Simulation_acceleration,
		Trajectory_desired_velocities,
		Simulation_velocity_halflife,
		20.0f * DeltaT,
		Obstacles_positions,
		Obstacles_scales);


	// Make query vector for search.
	// In theory this only needs to be done when a search is 
	// actually required however for visualization purposes it
	// can be nice to do it every frame
	array1d<float> query(DB.nfeatures());

	// Compute the features of the query vector
	slice1d<float> query_features = LMM_enabled ? slice1d<float>(Features_curr) : DB.features(Frame_index);

	int offset = 0;
	query_copy_denormalized_feature(query, offset, 3, query_features, DB.features_offset, DB.features_scale); // Left Foot Position
	query_copy_denormalized_feature(query, offset, 3, query_features, DB.features_offset, DB.features_scale); // Right Foot Position
	query_copy_denormalized_feature(query, offset, 3, query_features, DB.features_offset, DB.features_scale); // Left Foot Velocity
	query_copy_denormalized_feature(query, offset, 3, query_features, DB.features_offset, DB.features_scale); // Right Foot Velocity
	query_copy_denormalized_feature(query, offset, 3, query_features, DB.features_offset, DB.features_scale); // Hip Velocity
	query_compute_trajectory_position_feature(query, offset, Bone_positions(0), Bone_rotations(0), Trajectory_positions);
	query_compute_trajectory_direction_feature(query, offset, Bone_rotations(0), Trajectory_rotations);

	assert(offset == DB.nfeatures());

	// Check if we reached the end of the current anim
	bool end_of_anim = database_trajectory_index_clamp(DB, Frame_index, 1) == Frame_index; //MMdatabase에 정의되어 있음

	


}



void AMotionMatchingCharacter::DataBaseLog() {

	// Pose & Inertializer Data

	int frame_index = DB.range_starts(0);

	array1d<vec3> bone_positions = DB.bone_positions(frame_index);
	array1d<vec3> bone_velocities = DB.bone_velocities(frame_index);
	array1d<quat> bone_rotations = DB.bone_rotations(frame_index);

	//db에 data가 잘 저장되었는지 Log 출력으로 확인
	UE_LOG(LogTemp, Log, TEXT("Joints Num: %d"), bone_rotations.size);

	for (int i = 0; i < bone_rotations.size; i++) {

		UE_LOG(LogTemp, Log, TEXT("x: %f, y: %f, z: %f, w: %f"), bone_rotations.data[i].x, bone_rotations.data[i].y, bone_rotations.data[i].z, bone_rotations.data[i].w);
	}

}


void AMotionMatchingCharacter::SaveBasicRotators() {

	//Root는 WorldSpace 기준으로 저장
	BasicCharatorRotator.Emplace(GetMesh()->GetBoneRotationByName(FName(TEXT("Root")), EBoneSpaces::WorldSpace));
	//BasicCharatorRotator.Emplace(GetMesh()->GetBoneRotationByName(FName(TEXT("Root")), EBoneSpaces::ComponentSpace));

	UE_LOG(LogTemp, Log, TEXT("SaveBasicRotators()"));
	for (int i = 1; i < JointsNames.Num(); i++) {
		BasicCharatorRotator.Emplace(GetMesh()->GetBoneRotationByName(FName(JointsNames[i]), EBoneSpaces::ComponentSpace));
		//BasicCharatorRotator.Emplace(GetMesh()->GetBoneRotationByName(FName(JointsNames2[i]), EBoneSpaces::WorldSpace));


		UE_LOG(LogTemp, Log, TEXT("Roll: %f, Pitch: %f, Yaw: %f"), BasicCharatorRotator[i].Roll, BasicCharatorRotator[i].Pitch, BasicCharatorRotator[i].Yaw);
	}
}


void AMotionMatchingCharacter::SaveBasicVectors() {

	//Root는 WorldSpace 기준으로 저장
	BasicCharatorVector.Emplace(GetMesh()->GetBoneLocationByName(FName(TEXT("Root")), EBoneSpaces::WorldSpace));

	UE_LOG(LogTemp, Log, TEXT("SaveBasicVectors()"));
	for (int i = 1; i < JointsNames.Num(); i++) {
		BasicCharatorVector.Emplace(GetMesh()->GetBoneLocationByName(FName(JointsNames[i]), EBoneSpaces::ComponentSpace));
		//BasicCharatorRotator.Emplace(GetMesh()->GetBoneRotationByName(FName(JointsNames2[i]), EBoneSpaces::WorldSpace));
	}
}

void AMotionMatchingCharacter::CharacterLoadTest() {

	FString CharacterFilePath = FPaths::ProjectContentDir() + TEXT("/character.bin");
	const char* CharacterFilePathChar = TCHAR_TO_ANSI(*CharacterFilePath); 	// TCHAR_TO_ANSI 매크로를 사용하여 변환
	character_load(Character_data, CharacterFilePathChar);

	UE_LOG(LogTemp, Log, TEXT("positions length: %d"), Character_data.positions.size);
	UE_LOG(LogTemp, Log, TEXT("bone_rest_positions length: %d"), Character_data.bone_rest_positions.size);
	UE_LOG(LogTemp, Log, TEXT("bone_rest_rotations length: %d"), Character_data.bone_rest_rotations.size);

	//character_bone_rest_rotations 출력
	for (int i = 0; i < Character_data.bone_rest_rotations.size; i++) {
		UE_LOG(LogTemp, Log, TEXT("character_bone_rest_rotations:"));
		UE_LOG(LogTemp, Log, TEXT("x: %f, y: %f, z: %f, w: %f"), Character_data.bone_rest_rotations.data[i].x, Character_data.bone_rest_rotations.data[i].y, Character_data.bone_rest_rotations.data[i].z, Character_data.bone_rest_rotations.data[i].w);
	}
	////character_bone_rest_positions 출력
	for (int i = 0; i < Character_data.bone_rest_positions.size; i++) {
		UE_LOG(LogTemp, Log, TEXT("character_bone_rest_positions:"));
		UE_LOG(LogTemp, Log, TEXT("x: %f, y: %f, z: %f"), Character_data.bone_rest_positions.data[i].x, Character_data.bone_rest_positions.data[i].y, Character_data.bone_rest_positions.data[i].z);
	}

}

void AMotionMatchingCharacter::SetCharacterPositionRest() {

	int scale = 100;

	//관절의 개수
	int JointsNum = JointsNames.Num();

	array1d<vec3> bone_positions = Character_data.bone_rest_positions;

	TArray<FVector> JointsVector;
	for (int i = 0; i < JointsNum; i++) {
		JointsVector.Emplace(FVector(bone_positions.data[i].x * scale, bone_positions.data[i].z * scale, bone_positions.data[i].y * scale));
	}

	//Set Pose
	//GetMesh()->SetBoneLocationByName(FName(JointsNames2[0]), JointsVector[0] + BasicCharatorVector[0], EBoneSpaces::WorldSpace);
	GetMesh()->SetBoneLocationByName(FName(JointsNames[0]), JointsVector[0], EBoneSpaces::WorldSpace);

	for (int i = 1; i < JointsNum; i++) {
		//FVector Position = JointsVector[i] + BasicCharatorVector[i]; //basic에 position 더한 값
		FVector Position = JointsVector[i];
		GetMesh()->SetBoneLocationByName(FName(JointsNames[i]), Position, EBoneSpaces::ComponentSpace);
	}
}

void AMotionMatchingCharacter::SetCharacterRotationRest() {

	float pi = 3.141592;

	int scale = (180/pi);
	//int scale = 1;

	//관절의 개수
	int JointsNum = JointsNames.Num();

	array1d<quat> bone_rotations = Character_data.bone_rest_rotations;

	TArray<FQuat> JointsQuat;

	for (int i = 0; i < JointsNum; i++) {

		//언리얼에 맞게 좌표계를 변환해줌
		JointsQuat.Emplace(FQuat(bone_rotations.data[i].y * (1), bone_rotations.data[i].x * (1), bone_rotations.data[i].z * (-1), bone_rotations.data[i].w));

	}

	//Set pose
	FQuat StandQuat(0, 1, 0, 90);
	GetMesh()->SetBoneRotationByName(FName(JointsNames[0]), JointsQuat[0].Rotator(), EBoneSpaces::WorldSpace);

	for (int i = 1; i < JointsNum; i++) {
		//FRotator Rotation = JointsQuat[i].Rotator() + BasicCharatorRotator[i];
		FRotator Rotation = JointsQuat[i].Rotator();
		GetMesh()->SetBoneRotationByName(FName(JointsNames[i]), Rotation, EBoneSpaces::ComponentSpace);
		//GetMesh()->SetBoneRotationByName(FName(JointsNames2[i]), Rotation, EBoneSpaces::WorldSpace);
	}

	//Mesh를 world y축을 기준으로 90도 회전시켜서 세움
	GetMesh()->SetBoneRotationByName(FName(JointsNames[0]), GetMesh()->GetBoneRotationByName(FName(JointsNames[0]), EBoneSpaces::WorldSpace) + StandQuat.Rotator(), EBoneSpaces::WorldSpace);

}


void AMotionMatchingCharacter::PoseTest(int frameindex) {

	float pi = 3.141592;

	//int scale = (180/pi);
	int scale = 1;

	//관절의 개수
	int JointsNum = JointsNames.Num();

	//db로부터 원하는 frame의 rotation data 가져옴
	array1d<quat> bone_rotations = DB.bone_rotations(frameindex);


	//현재 프레임의 쿼터니언 데이터 저장 //curr_bone_rotations.size
	TArray<FQuat> JointsQuat;

	for (int i = 0; i < JointsNum; i++) {

		JointsQuat.Emplace(FQuat(bone_rotations.data[i].y * (1), bone_rotations.data[i].x * (1), bone_rotations.data[i].z * (-1), bone_rotations.data[i].w));
	}

	//Set pose

	GetMesh()->SetBoneRotationByName(FName(JointsNames[0]), JointsQuat[0].Rotator(), EBoneSpaces::WorldSpace);
	//GetMesh()->SetBoneRotationByName(FName(JointsNames2[0]), JointsQuat[0].Rotator(), EBoneSpaces::WorldSpace); //그냥 rotation만 적용 시키는 것

	for (int i = 1; i < JointsNum; i++) {

		//FRotator Rotation = JointsQuat[i].Rotator() + BasicCharatorRotator[i];
		FRotator Rotation = JointsQuat[i].Rotator();

		GetMesh()->SetBoneRotationByName(FName(JointsNames[i]), Rotation, EBoneSpaces::ComponentSpace);

	}
}


void AMotionMatchingCharacter::PoseTestByPostion(int frameindex) {

	int scale = 100;

	//관절의 개수
	int JointsNum = JointsNames.Num();

	//db로부터 원하는 frame의 rotation data 가져옴
	array1d<vec3> bone_positions = DB.bone_positions(frameindex);

	//현재 프레임의 position 데이터 저장 //curr_bone_rotations.size
	TArray<FVector> JointsVector;
	for (int i = 0; i < JointsNum; i++) {
		//JointsVector.Emplace(FVector(curr_bone_positions.data[i].x * scale, curr_bone_positions.data[i].y * scale, curr_bone_positions.data[i].z * scale));

		JointsVector.Emplace(FVector(bone_positions.data[i].x * scale, bone_positions.data[i].z * scale, bone_positions.data[i].x * scale));
	}

	//Set Pose
	GetMesh()->SetBoneLocationByName(FName(JointsNames[0]), JointsVector[0] + BasicCharatorVector[0], EBoneSpaces::WorldSpace);
	for (int i = 1; i < JointsNum; i++) {

		FVector Position = JointsVector[i] + BasicCharatorVector[i]; //basic에 position 더한 값

		GetMesh()->SetBoneLocationByName(FName(JointsNames[i]), Position, EBoneSpaces::ComponentSpace);
	}
}



//Input Log Test
void AMotionMatchingCharacter::InputLog() 
{
	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);


		UE_LOG(LogTemp, Log, TEXT("ForwardDirection: "));
		UE_LOG(LogTemp, Log, TEXT("x: %f, y: %f, z: %f"), ForwardDirection.X, ForwardDirection.Y, ForwardDirection.Z);

		UE_LOG(LogTemp, Log, TEXT("RightDirection: "));
		UE_LOG(LogTemp, Log, TEXT("x: %f, y: %f, z: %f"), RightDirection.X, RightDirection.Y, RightDirection.Z);
	}

	//-------------------------------------------
	//키보드 입력 부분

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}

		if (PlayerController->IsInputKeyDown(EKeys::A)) {
			UE_LOG(LogTemp, Log, TEXT("A is pressed"));
		}
		if (PlayerController->IsInputKeyDown(EKeys::S)) {
			UE_LOG(LogTemp, Log, TEXT("S is pressed"));
		}
		if (PlayerController->IsInputKeyDown(EKeys::D)) {
			UE_LOG(LogTemp, Log, TEXT("D is pressed"));
		}
		if (PlayerController->IsInputKeyDown(EKeys::W)) {
			UE_LOG(LogTemp, Log, TEXT("W is pressed"));
		}
	}

	//-------------------------------------------
	// Strafe 동작 테스트
	if (Desired_strafe == true)
	{
		UE_LOG(LogTemp, Log, TEXT("Strafe ON!!"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Strafe OFF!!"));
	}


	//-------------------------------------------

	//좌측 조이스틱(이동) 키를 입력하지 않으면 입력값 초기화.
	APlayerController* PlayerController = Cast<APlayerController>(Controller);
	if (PlayerController)
	{
		if (!PlayerController->IsInputKeyDown(EKeys::Gamepad_LeftStick_Down) && !PlayerController->IsInputKeyDown(EKeys::Gamepad_LeftStick_Up)
			&& !PlayerController->IsInputKeyDown(EKeys::Gamepad_LeftStick_Left) && !PlayerController->IsInputKeyDown(EKeys::Gamepad_LeftStick_Right))
		{
			LeftStickValue2D = FVector2D(0, 0);
		}
		else {
			UE_LOG(LogTemp, Log, TEXT("LeftStick2D"));
			UE_LOG(LogTemp, Log, TEXT("X: %f, Y: %f"), LeftStickValue2D.X, LeftStickValue2D.Y);
		}
	}

	//우측 조이스틱(룩) 키를 입력하지 않으면 입력값 초기화.
	if (PlayerController)
	{
		if (!PlayerController->IsInputKeyDown(EKeys::Gamepad_RightStick_Down) && !PlayerController->IsInputKeyDown(EKeys::Gamepad_RightStick_Up)
			&& !PlayerController->IsInputKeyDown(EKeys::Gamepad_RightStick_Left) && !PlayerController->IsInputKeyDown(EKeys::Gamepad_RightStick_Right))
		{
			RightStickValue2D = FVector2D(0, 0);
		}
		else {
			UE_LOG(LogTemp, Log, TEXT("RightStick2D"));
			UE_LOG(LogTemp, Log, TEXT("X: %f, Y: %f"), RightStickValue2D.X, RightStickValue2D.Y);
		}
	}


	//FRotator Rottest = PlayerController->GetControlRotation();
	//UE_LOG(LogTemp, Log, TEXT("GetControlRotation()"));
	//UE_LOG(LogTemp, Log, TEXT("Roll: %f, Pitch: %f, Yaw: %f"), Rottest.Roll, Rottest.Pitch, Rottest.Yaw);


	//FVector Vectest = PlayerController->GetInputVectorAxisValue(EKeys::Gamepad_LeftThumbstick);
	//UE_LOG(LogTemp, Log, TEXT("EKeys::Gamepad_LeftThumbstick"));
	//UE_LOG(LogTemp, Log, TEXT("X: %f, Y: %f, Z: %f"), Vectest.X, Vectest.Y, Vectest.Z);




	////우측 조이스틱(회전) 키를 입력하는지 체크
	//if (PlayerController)
	//{
	//	if (!PlayerController->IsInputKeyDown(EKeys::Gamepad_RightStick_Down) && !PlayerController->IsInputKeyDown(EKeys::Gamepad_RightStick_Up)
	//		&& !PlayerController->IsInputKeyDown(EKeys::Gamepad_RightStick_Left) && !PlayerController->IsInputKeyDown(EKeys::Gamepad_RightStick_Right))
	//	{
	//		IsHandlingRightStick = false;
	//	}
	//	else
	//	{
	//		IsHandlingRightStick = true;
	//	}
	//}

	//if (PlayerController->IsInputKeyDown(EKeys::Gamepad_RightStick_Down) || PlayerController->IsInputKeyDown(EKeys::Gamepad_RightStick_Up)
	//	|| PlayerController->IsInputKeyDown(EKeys::Gamepad_RightStick_Left) || PlayerController->IsInputKeyDown(EKeys::Gamepad_RightStick_Right))
	//{
	//	UE_LOG(LogTemp, Log, TEXT("Stick activated"));
	//}
	//else
	//{
	//	;
	//}





	//Cast<APlayerController>(Controller)->IsInputKeyDown(EKeys::A);

}
