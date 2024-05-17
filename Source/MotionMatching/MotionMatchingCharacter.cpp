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


	// Strafe 동작 테스트 확인하고 지워도 됨
	if (Desired_strafe == true)
	{
		UE_LOG(LogTemp, Log, TEXT("Strafe ON!!"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Strafe OFF!!"));
	}


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

void AMotionMatchingCharacter::OnStrafe(const FInputActionValue& Value)
{
	Desired_strafe = true;
}

void AMotionMatchingCharacter::OffStrafe(const FInputActionValue& Value)
{
	Desired_strafe = false;
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
	//vec3 gamepadstick_left = gamepad_get_stick(GAMEPAD_STICK_LEFT);
	//vec3 gamepadstick_right = gamepad_get_stick(GAMEPAD_STICK_RIGHT);

	// Get if strafe is desired
	//bool desired_strafe = desired_strafe_update();


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


	Cast<APlayerController>(Controller)->IsInputKeyDown(EKeys::A)




	//// Strafe 동작 테스트 확인하고 지워도 됨
	//if (Desired_strafe == true)
	//{
	//	UE_LOG(LogTemp, Log, TEXT("Strafe ON!!"));
	//}
	//else
	//{
	//	UE_LOG(LogTemp, Log, TEXT("Strafe OFF!!"));
	//}


}
