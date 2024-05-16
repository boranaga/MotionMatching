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


	//PoseableMesh�� �����ϰ� RootComponent�� ���� ��Ŵ.
	PoseableMesh = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("PoseableMesh"));
	PoseableMesh->SetupAttachment(RootComponent);



	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AMotionMatchingCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}



	//---------------------------------------------------
	
	//Character skeleton�� �⺻ roator ���� �迭�� ����
	SaveBasicRotators();
	//Character skeleton�� �⺻ vector ���� �迭�� ����
	SaveBasicVectors();


	//Character.bin load test
	//CharacterLoadTest();

	//MotionMatching BeginPlay
	MotionMatchingMainBeginPlay();


	//PoseTestByPostion(0);
	//PoseTest(0); //�켱�� ������ 0�� ���� ����� ��

	//SetCharacterPositionRest();
	SetCharacterRotationRest();

	//DataBase Log Test
	DataBaseLog();



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
//<����� ���� �Լ� ���� �κ�>



//UCharacter Ŭ������ �����ϴ� GetMesh() �Լ� ������ -> PosealbeMeshComponent�� ������.
UPoseableMeshComponent* AMotionMatchingCharacter::GetMesh() const
{
	return PoseableMesh;
}




//---------------------------------------------------------------------
//<controller.cpp�� ���ǵǾ� �ִ� �Լ���>


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
	const slice1d<vec3> bone_input_positions,
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
	inertialize_update( //MMspring.h
		bone_positions(0),
		bone_velocities(0),
		bone_offset_positions(0),
		bone_offset_velocities(0),
		world_space_position,
		world_space_velocity,
		halflife,
		dt);

	inertialize_update(
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





//---------------------------------------------------------------------


void AMotionMatchingCharacter::PoseTest(int frameindex) {

	float pi = 3.141592;

	//int scale = (180/pi);
	int scale = 1;

	//������ ����
	int JointsNum = JointsNames2.Num();

	//db�κ��� ���ϴ� frame�� rotation data ������
	array1d<quat> bone_rotations = db.bone_rotations(frameindex);


	//���� �������� ���ʹϾ� ������ ���� //curr_bone_rotations.size
	TArray<FQuat> JointsQuat; 

	for (int i = 0; i < JointsNum; i++) {

		JointsQuat.Emplace(FQuat(bone_rotations.data[i].y * (1), bone_rotations.data[i].x * (1), bone_rotations.data[i].z * (-1), bone_rotations.data[i].w));
	}

	//Set pose

	GetMesh()->SetBoneRotationByName(FName(JointsNames2[0]), JointsQuat[0].Rotator(), EBoneSpaces::WorldSpace);
	//GetMesh()->SetBoneRotationByName(FName(JointsNames2[0]), JointsQuat[0].Rotator(), EBoneSpaces::WorldSpace); //�׳� rotation�� ���� ��Ű�� ��

	for (int i = 1; i < JointsNum; i++) {

		//FRotator Rotation = JointsQuat[i].Rotator() + BasicCharatorRotator[i];
		FRotator Rotation = JointsQuat[i].Rotator();

		GetMesh()->SetBoneRotationByName(FName(JointsNames2[i]), Rotation, EBoneSpaces::ComponentSpace);

	}
}


void AMotionMatchingCharacter::PoseTestByPostion(int frameindex) {

	int scale = 100;

	//������ ����
	int JointsNum = JointsNames2.Num();

	//db�κ��� ���ϴ� frame�� rotation data ������
	array1d<vec3> bone_positions = db.bone_positions(frameindex);

	//���� �������� position ������ ���� //curr_bone_rotations.size
	TArray<FVector> JointsVector;
	for (int i = 0; i < JointsNum; i++) {
		//JointsVector.Emplace(FVector(curr_bone_positions.data[i].x * scale, curr_bone_positions.data[i].y * scale, curr_bone_positions.data[i].z * scale));

		JointsVector.Emplace(FVector(bone_positions.data[i].x * scale, bone_positions.data[i].z * scale, bone_positions.data[i].x * scale));
	}

	//Set Pose
	GetMesh()->SetBoneLocationByName(FName(JointsNames2[0]), JointsVector[0] + BasicCharatorVector[0], EBoneSpaces::WorldSpace);
	for (int i = 1; i < JointsNum; i++) {

		FVector Position = JointsVector[i] + BasicCharatorVector[i]; //basic�� position ���� ��

		GetMesh()->SetBoneLocationByName(FName(JointsNames2[i]), Position, EBoneSpaces::ComponentSpace);
	}
}




void AMotionMatchingCharacter::MotionMatchingMainBeginPlay() {


	// Character
	
	FString CharacterFilePath = FPaths::ProjectContentDir() + TEXT("/character.bin");
	const char* CharacterFilePathChar = TCHAR_TO_ANSI(*CharacterFilePath); 	// TCHAR_TO_ANSI ��ũ�θ� ����Ͽ� ��ȯ
	character_load(character_data, CharacterFilePathChar);


	// Load Animation Data

	FString DatabaseFilePath = FPaths::ProjectContentDir() + TEXT("/database.bin");
	const char* DatabaseFilePathChar = TCHAR_TO_ANSI(*DatabaseFilePath); 	// TCHAR_TO_ANSI ��ũ�θ� ����Ͽ� ��ȯ
	database_load(db, DatabaseFilePathChar);

	// build Matching Database
	database_build_matching_features(
		db,
		feature_weight_foot_position,
		feature_weight_foot_velocity,
		feature_weight_hip_velocity,
		feature_weight_trajectory_positions,
		feature_weight_trajectory_directions);


	FString FeaturesFilePath = FPaths::ProjectContentDir() + TEXT("/features.bin");
	const char* FeaturesFilePathChar = TCHAR_TO_ANSI(*FeaturesFilePath); 	// TCHAR_TO_ANSI ��ũ�θ� ����Ͽ� ��ȯ
	database_save_matching_features(db, FeaturesFilePathChar);


	// Pose & Inertializer Data

	int frame_index = db.range_starts(0);
	float inertialize_blending_halflife = 0.1f;

	array1d<vec3> curr_bone_positions = db.bone_positions(frame_index);
	array1d<vec3> curr_bone_velocities = db.bone_velocities(frame_index);
	array1d<quat> curr_bone_rotations = db.bone_rotations(frame_index);
	array1d<vec3> curr_bone_angular_velocities = db.bone_angular_velocities(frame_index);
	array1d<bool> curr_bone_contacts = db.contact_states(frame_index);

	array1d<vec3> trns_bone_positions = db.bone_positions(frame_index);
	array1d<vec3> trns_bone_velocities = db.bone_velocities(frame_index);
	array1d<quat> trns_bone_rotations = db.bone_rotations(frame_index);
	array1d<vec3> trns_bone_angular_velocities = db.bone_angular_velocities(frame_index);
	array1d<bool> trns_bone_contacts = db.contact_states(frame_index);

	array1d<vec3> bone_positions = db.bone_positions(frame_index);
	array1d<vec3> bone_velocities = db.bone_velocities(frame_index);
	array1d<quat> bone_rotations = db.bone_rotations(frame_index);
	array1d<vec3> bone_angular_velocities = db.bone_angular_velocities(frame_index);

	array1d<vec3> bone_offset_positions(db.nbones());
	array1d<vec3> bone_offset_velocities(db.nbones());
	array1d<quat> bone_offset_rotations(db.nbones());
	array1d<vec3> bone_offset_angular_velocities(db.nbones());

	array1d<vec3> global_bone_positions(db.nbones());
	array1d<vec3> global_bone_velocities(db.nbones());
	array1d<quat> global_bone_rotations(db.nbones());
	array1d<vec3> global_bone_angular_velocities(db.nbones());
	array1d<bool> global_bone_computed(db.nbones());

	vec3 transition_src_position;
	quat transition_src_rotation;
	vec3 transition_dst_position;
	quat transition_dst_rotation;

	inertialize_pose_reset(
		bone_offset_positions,
		bone_offset_velocities,
		bone_offset_rotations,
		bone_offset_angular_velocities,
		transition_src_position,
		transition_src_rotation,
		transition_dst_position,
		transition_dst_rotation,
		bone_positions(0),
		bone_rotations(0));

	inertialize_pose_update(
		bone_positions,
		bone_velocities,
		bone_rotations,
		bone_angular_velocities,
		bone_offset_positions,
		bone_offset_velocities,
		bone_offset_rotations,
		bone_offset_angular_velocities,
		db.bone_positions(frame_index),
		db.bone_velocities(frame_index),
		db.bone_rotations(frame_index),
		db.bone_angular_velocities(frame_index),
		transition_src_position,
		transition_src_rotation,
		transition_dst_position,
		transition_dst_rotation,
		inertialize_blending_halflife,
		0.0f);

}







void AMotionMatchingCharacter::MotionMatchingMainTick() {

	//��� ��Ī�� �������� ������ �� ������

}



void AMotionMatchingCharacter::DataBaseLog() {

	// Pose & Inertializer Data

	int frame_index = db.range_starts(0);

	array1d<vec3> bone_positions = db.bone_positions(frame_index);
	array1d<vec3> bone_velocities = db.bone_velocities(frame_index);
	array1d<quat> bone_rotations = db.bone_rotations(frame_index);

	//db�� data�� �� ����Ǿ����� Log ������� Ȯ��
	UE_LOG(LogTemp, Log, TEXT("Joints Num: %d"), bone_rotations.size);

	for (int i = 0; i < bone_rotations.size; i++) {

		UE_LOG(LogTemp, Log, TEXT("x: %f, y: %f, z: %f, w: %f"), bone_rotations.data[i].x, bone_rotations.data[i].y, bone_rotations.data[i].z, bone_rotations.data[i].w);
	}

}


void AMotionMatchingCharacter::SaveBasicRotators() {

	//Root�� WorldSpace �������� ����
	BasicCharatorRotator.Emplace(GetMesh()->GetBoneRotationByName(FName(TEXT("Root")), EBoneSpaces::WorldSpace));
	//BasicCharatorRotator.Emplace(GetMesh()->GetBoneRotationByName(FName(TEXT("Root")), EBoneSpaces::ComponentSpace));

	UE_LOG(LogTemp, Log, TEXT("SaveBasicRotators()"));
	for (int i = 1; i < JointsNames2.Num(); i++) {
		BasicCharatorRotator.Emplace(GetMesh()->GetBoneRotationByName(FName(JointsNames2[i]), EBoneSpaces::ComponentSpace));
		//BasicCharatorRotator.Emplace(GetMesh()->GetBoneRotationByName(FName(JointsNames2[i]), EBoneSpaces::WorldSpace));


		UE_LOG(LogTemp, Log, TEXT("Roll: %f, Pitch: %f, Yaw: %f"), BasicCharatorRotator[i].Roll, BasicCharatorRotator[i].Pitch, BasicCharatorRotator[i].Yaw);
	}
}


void AMotionMatchingCharacter::SaveBasicVectors() {

	//Root�� WorldSpace �������� ����
	BasicCharatorVector.Emplace(GetMesh()->GetBoneLocationByName(FName(TEXT("Root")), EBoneSpaces::WorldSpace));

	UE_LOG(LogTemp, Log, TEXT("SaveBasicVectors()"));
	for (int i = 1; i < JointsNames2.Num(); i++) {
		BasicCharatorVector.Emplace(GetMesh()->GetBoneLocationByName(FName(JointsNames2[i]), EBoneSpaces::ComponentSpace));
		//BasicCharatorRotator.Emplace(GetMesh()->GetBoneRotationByName(FName(JointsNames2[i]), EBoneSpaces::WorldSpace));
	}
}

void AMotionMatchingCharacter::CharacterLoadTest() {

	FString CharacterFilePath = FPaths::ProjectContentDir() + TEXT("/character.bin");
	const char* CharacterFilePathChar = TCHAR_TO_ANSI(*CharacterFilePath); 	// TCHAR_TO_ANSI ��ũ�θ� ����Ͽ� ��ȯ
	character_load(character_data, CharacterFilePathChar);

	UE_LOG(LogTemp, Log, TEXT("positions length: %d"), character_data.positions.size);
	UE_LOG(LogTemp, Log, TEXT("bone_rest_positions length: %d"), character_data.bone_rest_positions.size);
	UE_LOG(LogTemp, Log, TEXT("bone_rest_rotations length: %d"), character_data.bone_rest_rotations.size);

	//character_bone_rest_rotations ���
	for (int i = 0; i < character_data.bone_rest_rotations.size; i++) {
		UE_LOG(LogTemp, Log, TEXT("character_bone_rest_rotations:"));
		UE_LOG(LogTemp, Log, TEXT("x: %f, y: %f, z: %f, w: %f"), character_data.bone_rest_rotations.data[i].x, character_data.bone_rest_rotations.data[i].y, character_data.bone_rest_rotations.data[i].z, character_data.bone_rest_rotations.data[i].w);
	}
	////character_bone_rest_positions ���
	for (int i = 0; i < character_data.bone_rest_positions.size; i++) {
		UE_LOG(LogTemp, Log, TEXT("character_bone_rest_positions:"));
		UE_LOG(LogTemp, Log, TEXT("x: %f, y: %f, z: %f"), character_data.bone_rest_positions.data[i].x, character_data.bone_rest_positions.data[i].y, character_data.bone_rest_positions.data[i].z);
	}

}

void AMotionMatchingCharacter::SetCharacterPositionRest() {

	int scale = 100;

	//������ ����
	int JointsNum = JointsNames2.Num();

	array1d<vec3> bone_positions = character_data.bone_rest_positions;

	TArray<FVector> JointsVector;
	for (int i = 0; i < JointsNum; i++) {
		JointsVector.Emplace(FVector(bone_positions.data[i].x * scale, bone_positions.data[i].z * scale, bone_positions.data[i].y * scale));
	}

	//Set Pose
	//GetMesh()->SetBoneLocationByName(FName(JointsNames2[0]), JointsVector[0] + BasicCharatorVector[0], EBoneSpaces::WorldSpace);
	GetMesh()->SetBoneLocationByName(FName(JointsNames2[0]), JointsVector[0], EBoneSpaces::WorldSpace);

	for (int i = 1; i < JointsNum; i++) {
		//FVector Position = JointsVector[i] + BasicCharatorVector[i]; //basic�� position ���� ��
		FVector Position = JointsVector[i];
		GetMesh()->SetBoneLocationByName(FName(JointsNames2[i]), Position, EBoneSpaces::ComponentSpace);
	}
}

void AMotionMatchingCharacter::SetCharacterRotationRest() {

	float pi = 3.141592;

	int scale = (180/pi);
	//int scale = 1;

	//������ ����
	int JointsNum = JointsNames2.Num();

	array1d<quat> bone_rotations = character_data.bone_rest_rotations;

	TArray<FQuat> JointsQuat;

	for (int i = 0; i < JointsNum; i++) {

		//�𸮾� �°� ��ǥ�踦 ��ȯ����
		JointsQuat.Emplace(FQuat(bone_rotations.data[i].y * (1), bone_rotations.data[i].x * (1), bone_rotations.data[i].z * (-1), bone_rotations.data[i].w));

	}

	//Set pose
	FQuat StandQuat(0, 1, 0, 90);
	GetMesh()->SetBoneRotationByName(FName(JointsNames2[0]), JointsQuat[0].Rotator(), EBoneSpaces::WorldSpace);

	for (int i = 1; i < JointsNum; i++) {
		//FRotator Rotation = JointsQuat[i].Rotator() + BasicCharatorRotator[i];
		FRotator Rotation = JointsQuat[i].Rotator();
		GetMesh()->SetBoneRotationByName(FName(JointsNames2[i]), Rotation, EBoneSpaces::ComponentSpace);
		//GetMesh()->SetBoneRotationByName(FName(JointsNames2[i]), Rotation, EBoneSpaces::WorldSpace);
	}

	//Mesh�� world y���� �������� 90�� ȸ�����Ѽ� ����
	GetMesh()->SetBoneRotationByName(FName(JointsNames2[0]), GetMesh()->GetBoneRotationByName(FName(JointsNames2[0]), EBoneSpaces::WorldSpace) + StandQuat.Rotator(), EBoneSpaces::WorldSpace);

}