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
#include "MMPlayerController.h"

#include "Kismet/GameplayStatics.h" //GetPlyaerController 함수 불러오려고 추가함
#include "DrawDebugHelpers.h" //Debug 구체 visualizaiton을 위해 추가함


#include "Engine/World.h" //world의 정보를 불러오기 위해 추가
#include "GameFramework/Actor.h"
#include "EngineUtils.h"


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
	//PoseableMesh->SetupAttachment(RootComponent);



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
	if (TickTime >= 3) {
		//UE_LOG(LogTemp, Log, TEXT("%d Seconds has passed."), TimePassed);
		//TimePassed += 1;
		TickTime = 0.0f;

		InputLog();
	}
	//-------------------------------------------------------------
	//Input이 잘 작동하는지 테스트
	//InputLog();

	DeltaT = DeltaTime; //DeltaT를 DeltaTime으로 초기화

	SetInputZero(); //Input value 초기화

	MotionMatchingMainTick();

	//UI
	//if (LMM_enabled)
	//{
	//	UE_LOG(LogTemp, Log, TEXT("LMM ON"));
	//}
	//else
	//{
	//	UE_LOG(LogTemp, Log, TEXT("LMM OFF"));
	//}

	//if (Ik_enabled)
	//{
	//	UE_LOG(LogTemp, Log, TEXT("Ik_enabled ON"));
	//}
	//else
	//{
	//	UE_LOG(LogTemp, Log, TEXT("Ik_enabled OFF"));
	//}

	//if (Adjustment_enabled)
	//{
	//	UE_LOG(LogTemp, Log, TEXT("Adjustment_enabled ON"));
	//}
	//else
	//{
	//	UE_LOG(LogTemp, Log, TEXT("Adjustment_enabled OFF"));
	//}

	//if (Clamping_enabled)
	//{
	//	UE_LOG(LogTemp, Log, TEXT("Clamping_enabled ON"));
	//}
	//else
	//{
	//	UE_LOG(LogTemp, Log, TEXT("Clamping_enabled OFF"));
	//}

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

		// Zomm
		EnhancedInputComponent->BindAction(ZoomInAction, ETriggerEvent::Started, this, &AMotionMatchingCharacter::CamZoomInOn);
		EnhancedInputComponent->BindAction(ZoomInAction, ETriggerEvent::Completed, this, &AMotionMatchingCharacter::CamZoomInOff);

		EnhancedInputComponent->BindAction(ZoomOutAction, ETriggerEvent::Started, this, &AMotionMatchingCharacter::CamZoomOutOn);
		EnhancedInputComponent->BindAction(ZoomOutAction, ETriggerEvent::Completed, this, &AMotionMatchingCharacter::CamZoomOutOff);

		// Gait
		EnhancedInputComponent->BindAction(GaitAction, ETriggerEvent::Started, this, &AMotionMatchingCharacter::GaitButtonOn);
		EnhancedInputComponent->BindAction(GaitAction, ETriggerEvent::Completed, this, &AMotionMatchingCharacter::GaitButtonOff);

		// MenuUI
		EnhancedInputComponent->BindAction(MenuAction, ETriggerEvent::Started, this, &AMotionMatchingCharacter::TapKeyDown);

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


	//이것은 사용안함 //사용자 정의 함수 사용할 것임
	//if (Controller != nullptr)
	//{
	//	// find out which way is forward
	//	const FRotator Rotation = Controller->GetControlRotation();
	//	const FRotator YawRotation(0, Rotation.Yaw, 0);

	//	// get forward vector
	//	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	//
	//	// get right vector 
	//	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	//	// add movement 
	//	AddMovementInput(ForwardDirection, MovementVector.Y);
	//	AddMovementInput(RightDirection, MovementVector.X);
	//}


	// Gamepad left stick vector 2d value
	LeftStickValue2D = Value.Get<FVector2D>();
}

void AMotionMatchingCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();


	//이것은 사용안함 //사용자 정의 함수를 사용할 것임
	//if (Controller != nullptr)
	//{
	//	// add yaw and pitch input to controller
	//	AddControllerYawInput(LookAxisVector.X);
	//	AddControllerPitchInput(LookAxisVector.Y);
	//}

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

void AMotionMatchingCharacter::OnStrafe(const FInputActionValue& Value)
{
	Desired_strafe = true;
}

void AMotionMatchingCharacter::OffStrafe(const FInputActionValue& Value)
{
	Desired_strafe = false;
}

void AMotionMatchingCharacter::CamZoomInOn(const FInputActionValue& Value)
{
	CamZoomIn = true;
}

void AMotionMatchingCharacter::CamZoomInOff(const FInputActionValue& Value)
{
	CamZoomIn = false;
}

void AMotionMatchingCharacter::CamZoomOutOn(const FInputActionValue& Value)
{
	CamZoomOut = true;
}

void AMotionMatchingCharacter::CamZoomOutOff(const FInputActionValue& Value)
{
	CamZoomOut = false;
}

void AMotionMatchingCharacter::GaitButtonOn(const FInputActionValue& Value)
{
	GaitButton = true;
}

void AMotionMatchingCharacter::GaitButtonOff(const FInputActionValue& Value)
{
	GaitButton = false;
}



void AMotionMatchingCharacter::TapKeyDown()
{
	AMMPlayerController* PlayerController = Cast<AMMPlayerController>(Controller);
	IsTabButtonDown = !IsTabButtonDown;

	if (PlayerController)
	{
		PlayerController->ViewMenu(IsTabButtonDown);
	}
}


void AMotionMatchingCharacter::SetSimulationObj() {

	float scale = 100;
	float rotscale = 1;

	//Set Position
	FVector NewCharacterWorldPos(Simulation_position.z * scale, Simulation_position.x * (-1) * scale, Simulation_position.y * scale); //언리얼 좌표계로 변환
	SetActorLocation(NewCharacterWorldPos);

	//Set Rotation
	//FQuat RotationQuat(Simulation_rotation.z * (1), Simulation_rotation.x * (-1), Simulation_rotation.y * (1), Simulation_rotation.w * rotscale); //언리얼 좌표계로 변환
	FQuat RotationQuat(Simulation_rotation.z * (1), Simulation_rotation.x * (1), Simulation_rotation.y * (-1), Simulation_rotation.w * rotscale); //언리얼 좌표계로 변환

	FRotator NewCharacterRotation = RotationQuat.Rotator();
	SetActorRotation(NewCharacterRotation);

}



void AMotionMatchingCharacter::SetCharacterAnimation() {


	//관절의 개수
	int JointsNum = JointsNames.Num();


	//-------------------------------------------------------------------------------
	//<Set Rotation>


	array1d<quat> bone_rotations = Global_bone_rotations;
	TArray<FQuat> JointsQuat;


	//JointsQuat.Emplace(FQuat(bone_rotations.data[0].z * (1), bone_rotations.data[0].x * (-1), bone_rotations.data[0].y * (1), bone_rotations.data[0].w));
	JointsQuat.Emplace(FQuat(bone_rotations.data[0].y * (1), bone_rotations.data[0].x * (1), bone_rotations.data[0].z * (-1), bone_rotations.data[0].w * (1) ));

	for (int i = 1; i < JointsNum; i++) {
		//언리얼에 맞게 좌표계를 변환해줌
		JointsQuat.Emplace(FQuat(bone_rotations.data[i].y * (1), bone_rotations.data[i].x * (1), bone_rotations.data[i].z * (-1), bone_rotations.data[i].w));
		//JointsQuat.Emplace(FQuat(bone_rotations.data[i].z * (1), bone_rotations.data[i].x * (-1), bone_rotations.data[i].y * (1), bone_rotations.data[i].w));
	}

	//Set pose
	FQuat StandQuat(0, 1, 0, 90);
	//GetMesh()->SetBoneRotationByName(FName(JointsNames[0]), JointsQuat[0].Rotator(), EBoneSpaces::WorldSpace);
	GetMesh()->SetBoneRotationByName(FName(JointsNames[0]), JointsQuat[0].Rotator(), EBoneSpaces::ComponentSpace);

	for (int i = 1; i < JointsNum; i++) {
		FRotator Rotation = JointsQuat[i].Rotator();
		GetMesh()->SetBoneRotationByName(FName(JointsNames[i]), Rotation, EBoneSpaces::ComponentSpace);
		//GetMesh()->SetBoneRotationByName(FName(JointsNames[i]), Rotation, EBoneSpaces::WorldSpace);
	}

	//Mesh를 world y축을 기준으로 90도 회전시켜서 세움
	//GetMesh()->SetBoneRotationByName(FName(JointsNames[1]), GetMesh()->GetBoneRotationByName(FName(JointsNames[1]), EBoneSpaces::ComponentSpace) + StandQuat.Rotator(), EBoneSpaces::ComponentSpace);

	//GetMesh()->SetBoneRotationByName(FName(JointsNames[0]), GetMesh()->GetBoneRotationByName(FName(JointsNames[0]), EBoneSpaces::WorldSpace) + StandQuat.Rotator(), EBoneSpaces::WorldSpace);
	GetMesh()->SetBoneRotationByName(FName(JointsNames[0]), GetMesh()->GetBoneRotationByName(FName(JointsNames[0]), EBoneSpaces::ComponentSpace) + StandQuat.Rotator(), EBoneSpaces::ComponentSpace);


	//-------------------------------------------------------------------------------
	//<Set Position>
	array1d<vec3> bone_positions = Global_bone_positions;
	TArray<FVector> JointsVec;

	int scale = 100;

	for (int i = 0; i < JointsNum; i++) {
		JointsVec.Emplace(FVector(bone_positions.data[i].z * scale, bone_positions.data[i].x * (-1) * scale, bone_positions.data[i].y * scale));
		//JointsVec.Emplace(FVector(bone_positions.data[i].z * scale, bone_positions.data[i].x * (1) * scale, bone_positions.data[i].y * scale));
	}

	//Set Pose
	FVector offset(0, 0, 90);
	GetMesh()->SetBoneLocationByName(FName(JointsNames[0]), JointsVec[0] + offset, EBoneSpaces::WorldSpace);

	//for (int i = 1; i < JointsNum; i++) {
	//	//FVector Position = JointsVector[i] + BasicCharatorVector[i]; //basic에 position 더한 값
	//	FVector Position = JointsVector[i];
	//	GetMesh()->SetBoneLocationByName(FName(JointsNames[i]), Position, EBoneSpaces::ComponentSpace);
	//}


}


//-----------------------------------------------------
void AMotionMatchingCharacter::GetObstaclesinfo()
{
	float scale = 100;


	UWorld* World = GetWorld();

	if (ActorClass)
	{
		//UGameplayStatics::GetAllActorsOfClass(this, ActorClass, OutActors); //이것도 가능
		UGameplayStatics::GetAllActorsOfClass(World, ActorClass, OutActors);
		UE_LOG(LogTemp, Log, TEXT("Number of Actors: %d"), OutActors.Num());

		Obstacles_positions.resize(OutActors.Num());
		Obstacles_scales.resize(OutActors.Num());
	}


	Obstacles_positions.resize(OutActors.Num());
	Obstacles_scales.resize(OutActors.Num());

	for (int i = 0; i < OutActors.Num(); i++)
	{
		FVector ActorLocation = OutActors[i]->GetActorLocation() / scale;

		Obstacles_positions(i) = vec3(-ActorLocation.Y, ActorLocation.Z, ActorLocation.X);


		// 스태틱 메시 컴포넌트를 찾음
		UStaticMeshComponent* MeshComponent = OutActors[i]->FindComponentByClass<UStaticMeshComponent>();
		if (MeshComponent)
		{
			// 메시 컴포넌트의 바운드를 가져옴
			FVector CubeBounds = MeshComponent->Bounds.BoxExtent;

			// 실제 크기를 계산
			FVector ActualCubeSize = CubeBounds * 2.0f / scale; // BoxExtent가 반지름이기 때문에 2를 곱함

			Obstacles_scales(i) = vec3(ActualCubeSize.Y, ActualCubeSize.Z, ActualCubeSize.X); //좌표계 변환
		}
	}
}

//------------------------------------------------------
void AMotionMatchingCharacter::SetInputZero() {

	//좌측 조이스틱(이동) 키를 입력하지 않으면 입력값 초기화.
	APlayerController* PlayerController = Cast<APlayerController>(Controller);
	if (PlayerController)
	{
		if (!PlayerController->IsInputKeyDown(EKeys::Gamepad_LeftStick_Down) && !PlayerController->IsInputKeyDown(EKeys::Gamepad_LeftStick_Up)
			&& !PlayerController->IsInputKeyDown(EKeys::Gamepad_LeftStick_Left) && !PlayerController->IsInputKeyDown(EKeys::Gamepad_LeftStick_Right))
		{
			LeftStickValue2D = FVector2D(0, 0);
		}
		//else {
		//	UE_LOG(LogTemp, Log, TEXT("LeftStick2D"));
		//	UE_LOG(LogTemp, Log, TEXT("X: %f, Y: %f"), LeftStickValue2D.X, LeftStickValue2D.Y);
		//}
	}

	//우측 조이스틱(룩) 키를 입력하지 않으면 입력값 초기화.
	if (PlayerController)
	{
		if (!PlayerController->IsInputKeyDown(EKeys::Gamepad_RightStick_Down) && !PlayerController->IsInputKeyDown(EKeys::Gamepad_RightStick_Up)
			&& !PlayerController->IsInputKeyDown(EKeys::Gamepad_RightStick_Left) && !PlayerController->IsInputKeyDown(EKeys::Gamepad_RightStick_Right))
		{
			RightStickValue2D = FVector2D(0, 0);
		}
		//else {
		//	UE_LOG(LogTemp, Log, TEXT("RightStick2D"));
		//	UE_LOG(LogTemp, Log, TEXT("X: %f, Y: %f"), RightStickValue2D.X, RightStickValue2D.Y);
		//}
	}
}



//------------------------------------------------------

FVector AMotionMatchingCharacter::To_Vector3(vec3 v)
{
	float scale = 100;

	return FVector(v.z * scale, -v.x * scale, v.y * scale);
}																															
													

void AMotionMatchingCharacter::Draw_features(const slice1d<float> features, const vec3 pos, const quat rot, FColor color)
{												
	vec3 lfoot_pos = quat_mul_vec3(rot, vec3(features(0), features(1), features(2))) + pos;
	vec3 rfoot_pos = quat_mul_vec3(rot, vec3(features(3), features(4), features(5))) + pos;
	vec3 lfoot_vel = quat_mul_vec3(rot, vec3(features(6), features(7), features(8)));
	vec3 rfoot_vel = quat_mul_vec3(rot, vec3(features(9), features(10), features(11)));
	//vec3 hip_vel   = quat_mul_vec3(rot, vec3(features(12), features(13), features(14)));
	vec3 traj0_pos = quat_mul_vec3(rot, vec3(features(15), 0.0f, features(16))) + pos;
	vec3 traj1_pos = quat_mul_vec3(rot, vec3(features(17), 0.0f, features(18))) + pos;
	vec3 traj2_pos = quat_mul_vec3(rot, vec3(features(19), 0.0f, features(20))) + pos;
	vec3 traj0_dir = quat_mul_vec3(rot, vec3(features(21), 0.0f, features(22)));
	vec3 traj1_dir = quat_mul_vec3(rot, vec3(features(23), 0.0f, features(24)));
	vec3 traj2_dir = quat_mul_vec3(rot, vec3(features(25), 0.0f, features(26)));

	DrawDebugSphere(GetWorld(), To_Vector3(lfoot_pos), 5.0f, 8, color);
	DrawDebugSphere(GetWorld(), To_Vector3(rfoot_pos), 5.0f, 8, color);
	DrawDebugSphere(GetWorld(), To_Vector3(traj0_pos), 5.0f, 8, color);
	DrawDebugSphere(GetWorld(), To_Vector3(traj1_pos), 5.0f, 8, color);
	DrawDebugSphere(GetWorld(), To_Vector3(traj2_pos), 5.0f, 8, color);

	DrawDebugLine(GetWorld(), To_Vector3(lfoot_pos), To_Vector3(lfoot_pos + 0.1f * lfoot_vel), color);
	DrawDebugLine(GetWorld(), To_Vector3(rfoot_pos), To_Vector3(rfoot_pos + 0.1f * rfoot_vel), color);
	DrawDebugLine(GetWorld(), To_Vector3(traj0_pos), To_Vector3(traj0_pos + 0.1f * traj0_dir), color);
	DrawDebugLine(GetWorld(), To_Vector3(traj1_pos), To_Vector3(traj1_pos + 0.1f * traj1_dir), color);
	DrawDebugLine(GetWorld(), To_Vector3(traj2_pos), To_Vector3(traj2_pos + 0.1f * traj2_dir), color);
}

void AMotionMatchingCharacter::Draw_trajectory(const slice1d<vec3> trajectory_positions, const slice1d<quat> trajectory_rotations, FColor color)
{
	for (int i = 1; i < trajectory_positions.size; i++)
	{
		DrawDebugSphere(GetWorld(), To_Vector3(trajectory_positions(i)), 5.0f, 8, color);
		DrawDebugLine(
			GetWorld(),
			To_Vector3(trajectory_positions(i)),
			To_Vector3(trajectory_positions(i) + 0.6f * quat_mul_vec3(trajectory_rotations(i), vec3(0, 0, 1.0f))),
			color);
		DrawDebugLine(GetWorld(), To_Vector3(trajectory_positions(i - 1)), To_Vector3(trajectory_positions(i)), color);
	}
}

void AMotionMatchingCharacter::Draw_simulation_object(const vec3 simulation_position, quat simulation_rotation, FColor color)
{
	DrawDebugCircle(GetWorld(), To_Vector3(simulation_position), 60.0f, 60.0f, color, false, -1, 2, 2, FVector(0, 1, 0), FVector(1, 0, 0), false);
	DrawDebugSphere(GetWorld(), To_Vector3(simulation_position), 5.0f, 16, color);
	DrawDebugLine(
		GetWorld(),
		To_Vector3(simulation_position),
		To_Vector3(simulation_position + 0.6f * quat_mul_vec3(simulation_rotation, vec3(0.0f, 0.0f, 1.0f))),
		color
	);
}


//------------------------------------------------------


float AMotionMatchingCharacter::orbit_camera_update_azimuth(
	const float azimuth,
	const vec3 gamepadstick_right,
	const bool desired_strafe,
	const float dt)
{
	//float speed = 5; //회전 속도

	vec3 gamepadaxis = desired_strafe ? vec3() : gamepadstick_right;
	return azimuth + 2.0f * dt * -gamepadaxis.x * CamRotationInputSpeed * (-1); //(-1) 곱해줘야함
}

float AMotionMatchingCharacter::orbit_camera_update_altitude(
	const float altitude,
	const vec3 gamepadstick_right,
	const bool desired_strafe,
	const float dt)
{
	//float speed = 5; //회전 속도

	vec3 gamepadaxis = desired_strafe ? vec3() : gamepadstick_right;
	return clampf(altitude + 2.0f * dt * gamepadaxis.z * CamRotationInputSpeed * (-1), 0.0, 0.4f * PIf); //(-1) 곱해줘야함
}


float AMotionMatchingCharacter::orbit_camera_update_distance(
	const float distance,
	const float dt)
{
	float scale = 100;

	float gamepadzoom =
		CamZoomOut ? +1.0f :
		CamZoomIn ? -1.0f : 0.0f;

	return clampf(distance + 10.0f * dt * gamepadzoom * scale, 100.0f, 10000.0f);
}

// Updates the camera using the orbit cam controls
void AMotionMatchingCharacter::orbit_camera_update(
	//Camera3D& cam,
	float& camera_azimuth,
	float& camera_altitude,
	float& camera_distance,
	//const vec3 target,
	const vec3 gamepadstick_right,
	const bool desired_strafe,
	const float dt
)
{
	camera_azimuth = orbit_camera_update_azimuth(camera_azimuth, gamepadstick_right, desired_strafe, dt);
	camera_altitude = orbit_camera_update_altitude(camera_altitude, gamepadstick_right, desired_strafe, dt);
	camera_distance = orbit_camera_update_distance(camera_distance, dt);

	//quat rotation_azimuth = quat_from_angle_axis(camera_azimuth, vec3(0, 1, 0));
	quat rotation_azimuth = quat_from_angle_axis(camera_azimuth, vec3(0, 0, 1)); //언리얼에서는 (0, 0, 1)을 기준으로 함

	//vec3 position = quat_mul_vec3(rotation_azimuth, vec3(0, 0, camera_distance));
	vec3 position = quat_mul_vec3(rotation_azimuth, vec3(0, camera_distance, 0)); //언리얼에 맞게 수정해줌
	//vec3 axis = normalize(cross(position, vec3(0, 1, 0)));
	vec3 axis = normalize(cross(position, vec3(0, 0, 1)));

	quat rotation_altitude = quat_from_angle_axis(camera_altitude, axis);

	//vec3 eye = target + quat_mul_vec3(rotation_altitude, position);

	//cam.target = (Vector3){ target.x, target.y, target.z };
	//cam.position = (Vector3){ eye.x, eye.y, eye.z };

	//---------------------------------------------------------------------------
	FVector CamWorldPos = FollowCamera->GetComponentLocation();
	FVector CharacterWorldPos = GetActorLocation();
	FVector offset(0, 0, 160); //target offset (눈 높이)

	//vec3 target = vec3(CharacterWorldPos.X, CharacterWorldPos.Y, CharacterWorldPos.Z);
	vec3 target = vec3(CharacterWorldPos.Y, CharacterWorldPos.X, CharacterWorldPos.Z * (1) ); //마찬가지로 좌표계를 언리얼에 맞게 변환시켜야함
	vec3 eye = target + quat_mul_vec3(rotation_altitude, position);
	//FVector NewCamPos(eye.x, eye.y, eye.z);
	FVector NewCamPos = FVector(eye.y, eye.x, eye.z * (1)) +offset; //언리얼에 맞게 좌표계 변환
	FVector CamLookAtVec = (CharacterWorldPos + offset)- NewCamPos;
	
	FollowCamera->SetWorldRotation(CamLookAtVec.Rotation());

	//FollowCamera->SetRelativeRotation(CamLookAtVec.Rotation());


	FollowCamera->SetWorldLocation(NewCamPos); //카메라를 새로운 위치로 업데이트
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
		GaitButton ? 1.0f : 0.0f,
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


// Moving the root is a little bit difficult when we have the
// inertializer set up in the way we do. Essentially we need
// to also make sure to adjust all of the locations where 
// we are transforming the data to and from as well as the 
// offsets being blended out
void inertialize_root_adjust(
	vec3& offset_position,
	vec3& transition_src_position,
	quat& transition_src_rotation,
	vec3& transition_dst_position,
	quat& transition_dst_rotation,
	vec3& position,
	quat& rotation,
	const vec3 input_position,
	const quat input_rotation)
{
	// Find the position difference and add it to the state and transition location
	vec3 position_difference = input_position - position;
	position = position_difference + position;
	transition_dst_position = position_difference + transition_dst_position;

	// Find the point at which we want to now transition from in the src data
	transition_src_position = transition_src_position + quat_mul_vec3(transition_src_rotation,
		quat_inv_mul_vec3(transition_dst_rotation, position - offset_position - transition_dst_position));
	transition_dst_position = position;
	offset_position = vec3();

	// Find the rotation difference. We need to normalize here or some error can accumulate 
	// over time during adjustment.
	quat rotation_difference = quat_normalize(quat_mul_inv(input_rotation, rotation));

	// Apply the rotation difference to the current rotation and transition location
	rotation = quat_mul(rotation_difference, rotation);
	transition_dst_rotation = quat_mul(rotation_difference, transition_dst_rotation);
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


// This function transitions the inertializer for 
// the full character. It takes as input the current 
// offsets, as well as the root transition locations,
// current root state, and the full pose information 
// for the pose being transitioned from (src) as well 
// as the pose being transitioned to (dst) in their
// own animation spaces.
void AMotionMatchingCharacter::inertialize_pose_transition(
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
	const slice1d<vec3> bone_dst_angular_velocities)
{
	// First we record the root position and rotation
	// in the animation data for the source and destination
	// animation
	transition_dst_position = root_position;
	transition_dst_rotation = root_rotation;
	transition_src_position = bone_dst_positions(0);
	transition_src_rotation = bone_dst_rotations(0);

	// We then find the velocities so we can transition the 
	// root inertiaizers
	vec3 world_space_dst_velocity = quat_mul_vec3(transition_dst_rotation,
		quat_inv_mul_vec3(transition_src_rotation, bone_dst_velocities(0)));

	vec3 world_space_dst_angular_velocity = quat_mul_vec3(transition_dst_rotation,
		quat_inv_mul_vec3(transition_src_rotation, bone_dst_angular_velocities(0)));

	// Transition inertializers recording the offsets for 
	// the root joint
	inertialize_transition(
		bone_offset_positions(0),
		bone_offset_velocities(0),
		root_position,
		root_velocity,
		root_position,
		world_space_dst_velocity);

	inertialize_transition(
		bone_offset_rotations(0),
		bone_offset_angular_velocities(0),
		root_rotation,
		root_angular_velocity,
		root_rotation,
		world_space_dst_angular_velocity);

	// Transition all the inertializers for each other bone
	for (int i = 1; i < bone_offset_positions.size; i++)
	{
		inertialize_transition(
			bone_offset_positions(i),
			bone_offset_velocities(i),
			bone_src_positions(i),
			bone_src_velocities(i),
			bone_dst_positions(i),
			bone_dst_velocities(i));

		inertialize_transition(
			bone_offset_rotations(i),
			bone_offset_angular_velocities(i),
			bone_src_rotations(i),
			bone_src_angular_velocities(i),
			bone_dst_rotations(i),
			bone_dst_angular_velocities(i));
	}
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


void AMotionMatchingCharacter::contact_update(
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
	const float eps = 1e-8)
{
	// First compute the input contact position velocity via finite difference
	vec3 input_contact_velocity =
		(input_contact_position - contact_target) / (dt + eps);
	contact_target = input_contact_position;

	// Update the inertializer to tick forward in time
	inertialize_update(
		contact_position,
		contact_velocity,
		contact_offset_position,
		contact_offset_velocity,
		// If locked we feed the contact point and zero velocity, 
		// otherwise we feed the input from the animation
		contact_lock ? contact_point : input_contact_position,
		contact_lock ? vec3() : input_contact_velocity,
		halflife,
		dt);

	// If the contact point is too far from the current input position 
	// then we need to unlock the contact
	bool unlock_contact = contact_lock && (
		length(contact_point - input_contact_position) > unlock_radius);

	// If the contact was previously inactive but is now active we 
	// need to transition to the locked contact state
	if (!contact_state && input_contact_state)
	{
		// Contact point is given by the current position of 
		// the foot projected onto the ground plus foot height
		contact_lock = true;
		contact_point = contact_position;
		contact_point.y = foot_height;

		inertialize_transition(
			contact_offset_position,
			contact_offset_velocity,
			input_contact_position,
			input_contact_velocity,
			contact_point,
			vec3());
	}

	// Otherwise if we need to unlock or we were previously in 
	// contact but are no longer we transition to just taking 
	// the input position as-is
	else if ((contact_lock && contact_state && !input_contact_state)
		|| unlock_contact)
	{
		contact_lock = false;

		inertialize_transition(
			contact_offset_position,
			contact_offset_velocity,
			contact_point,
			vec3(),
			input_contact_position,
			input_contact_velocity);
	}

	// Update contact state
	contact_state = input_contact_state;
}

//--------------------------------------
// Rotate a joint to look toward some 
// given target position
void AMotionMatchingCharacter::ik_look_at(
	quat& bone_rotation,
	const quat global_parent_rotation,
	const quat global_rotation,
	const vec3 global_position,
	const vec3 child_position,
	const vec3 target_position,
	const float eps = 1e-5f)
{
	vec3 curr_dir = normalize(child_position - global_position);
	vec3 targ_dir = normalize(target_position - global_position);

	if (fabs(1.0f - dot(curr_dir, targ_dir) > eps))
	{
		bone_rotation = quat_inv_mul(global_parent_rotation,
			quat_mul(quat_between(curr_dir, targ_dir), global_rotation));
	}
}

// Basic two-joint IK in the style of https://theorangeduck.com/page/simple-two-joint
// Here I add a basic "forward vector" which acts like a kind of pole-vetor
// to control the bending direction
void AMotionMatchingCharacter::ik_two_bone(
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
	const float max_length_buffer) {

	float max_extension =
		length(bone_root - bone_mid) +
		length(bone_mid - bone_end) -
		max_length_buffer;

	vec3 target_clamp = target;
	if (length(target - bone_root) > max_extension)
	{
		target_clamp = bone_root + max_extension * normalize(target - bone_root);
	}

	vec3 axis_dwn = normalize(bone_end - bone_root);
	vec3 axis_rot = normalize(cross(axis_dwn, fwd));

	vec3 a = bone_root;
	vec3 b = bone_mid;
	vec3 c = bone_end;
	vec3 t = target_clamp;

	float lab = length(b - a);
	float lcb = length(b - c);
	float lat = length(t - a);

	float ac_ab_0 = acosf(clampf(dot(normalize(c - a), normalize(b - a)), -1.0f, 1.0f));
	float ba_bc_0 = acosf(clampf(dot(normalize(a - b), normalize(c - b)), -1.0f, 1.0f));

	float ac_ab_1 = acosf(clampf((lab * lab + lat * lat - lcb * lcb) / (2.0f * lab * lat), -1.0f, 1.0f));
	float ba_bc_1 = acosf(clampf((lab * lab + lcb * lcb - lat * lat) / (2.0f * lab * lcb), -1.0f, 1.0f));

	quat r0 = quat_from_angle_axis(ac_ab_1 - ac_ab_0, axis_rot);
	quat r1 = quat_from_angle_axis(ba_bc_1 - ba_bc_0, axis_rot);

	vec3 c_a = normalize(bone_end - bone_root);
	vec3 t_a = normalize(target_clamp - bone_root);

	quat r2 = quat_from_angle_axis(
		acosf(clampf(dot(c_a, t_a), -1.0f, 1.0f)),
		normalize(cross(c_a, t_a)));

	bone_root_lr = quat_inv_mul(bone_par_gr, quat_mul(r2, quat_mul(r0, bone_root_gr)));
	bone_mid_lr = quat_inv_mul(bone_root_gr, quat_mul(r1, bone_mid_gr));
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


//--------------------------------------

vec3 AMotionMatchingCharacter::adjust_character_position(
	const vec3 character_position,
	const vec3 simulation_position,
	const float halflife,
	const float dt)
{
	// Find the difference in positioning
	vec3 difference_position = simulation_position - character_position;

	// Damp that difference using the given halflife and dt
	vec3 adjustment_position = damp_adjustment_exact(
		difference_position,
		halflife,
		dt);

	// Add the damped difference to move the character toward the sim
	return adjustment_position + character_position;
}

quat AMotionMatchingCharacter::adjust_character_rotation(
	const quat character_rotation,
	const quat simulation_rotation,
	const float halflife,
	const float dt)
{
	// Find the difference in rotation (from character to simulation).
	// Here `quat_abs` forces the quaternion to take the shortest 
	// path and normalization is required as sometimes taking 
	// the difference between two very similar rotations can 
	// introduce numerical instability
	quat difference_rotation = quat_abs(quat_normalize(
		quat_mul_inv(simulation_rotation, character_rotation)));

	// Damp that difference using the given halflife and dt
	quat adjustment_rotation = damp_adjustment_exact(
		difference_rotation,
		halflife,
		dt);

	// Apply the damped adjustment to the character
	return quat_mul(adjustment_rotation, character_rotation);
}

vec3 AMotionMatchingCharacter::adjust_character_position_by_velocity(
	const vec3 character_position,
	const vec3 character_velocity,
	const vec3 simulation_position,
	const float max_adjustment_ratio,
	const float halflife,
	const float dt)
{
	// Find and damp the desired adjustment
	vec3 adjustment_position = damp_adjustment_exact(
		simulation_position - character_position,
		halflife,
		dt);

	// If the length of the adjustment is greater than the character velocity 
	// multiplied by the ratio then we need to clamp it to that length
	float max_length = max_adjustment_ratio * length(character_velocity) * dt;

	if (length(adjustment_position) > max_length)
	{
		adjustment_position = max_length * normalize(adjustment_position);
	}

	// Apply the adjustment
	return adjustment_position + character_position;
}

quat AMotionMatchingCharacter::adjust_character_rotation_by_velocity(
	const quat character_rotation,
	const vec3 character_angular_velocity,
	const quat simulation_rotation,
	const float max_adjustment_ratio,
	const float halflife,
	const float dt)
{
	// Find and damp the desired rotational adjustment
	quat adjustment_rotation = damp_adjustment_exact(
		quat_abs(quat_normalize(quat_mul_inv(
			simulation_rotation, character_rotation))),
		halflife,
		dt);

	// If the length of the adjustment is greater than the angular velocity 
	// multiplied by the ratio then we need to clamp this adjustment
	float max_length = max_adjustment_ratio *
		length(character_angular_velocity) * dt;

	if (length(quat_to_scaled_angle_axis(adjustment_rotation)) > max_length)
	{
		// To clamp can convert to scaled angle axis, rescale, and convert back
		adjustment_rotation = quat_from_scaled_angle_axis(max_length *
			normalize(quat_to_scaled_angle_axis(adjustment_rotation)));
	}

	// Apply the adjustment
	return quat_mul(adjustment_rotation, character_rotation);
}

//--------------------------------------
vec3 AMotionMatchingCharacter::clamp_character_position(
	const vec3 character_position,
	const vec3 simulation_position,
	const float max_distance)
{
	// If the character deviates too far from the simulation 
	// position we need to clamp it to within the max distance
	if (length(character_position - simulation_position) > max_distance)
	{
		return max_distance *
			normalize(character_position - simulation_position) +
			simulation_position;
	}
	else
	{
		return character_position;
	}
}

quat AMotionMatchingCharacter::clamp_character_rotation(
	const quat character_rotation,
	const quat simulation_rotation,
	const float max_angle)
{
	// If the angle between the character rotation and simulation 
	// rotation exceeds the threshold we need to clamp it back
	if (quat_angle_between(character_rotation, simulation_rotation) > max_angle)
	{
		// First, find the rotational difference between the two
		quat diff = quat_abs(quat_mul_inv(
			character_rotation, simulation_rotation));

		// We can then decompose it into angle and axis
		float diff_angle; vec3 diff_axis;
		quat_to_angle_axis(diff, diff_angle, diff_axis);

		// We then clamp the angle to within our bounds
		diff_angle = clampf(diff_angle, -max_angle, max_angle);

		// And apply back the clamped rotation
		return quat_mul(
			quat_from_angle_axis(diff_angle, diff_axis), simulation_rotation);
	}
	else
	{
		return character_rotation;
	}
}




//---------------------------------------------------------------------

void AMotionMatchingCharacter::MotionMatchingMainBeginPlay() {

	// Set obstacles
	//GetObstaclesinfo();


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
	//LMM_enabled = false;
	LMM_enabled = true;


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
	//vec3 gamepadstick_left = vec3(LeftStickValue2D.X, 0.0f, LeftStickValue2D.Y);

	vec3 gamepadstick_left = vec3(LeftStickValue2D.X, 0.0f, LeftStickValue2D.Y *(-1)); //Y에 (-1)을 곱해주었음

	vec3 gamepadstick_right = vec3(RightStickValue2D.X * (-1), 0.0f, RightStickValue2D.Y * (-1));


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

	// Do we need to search?
	if (force_search || Search_timer <= 0.0f || end_of_anim)
	{
		if (LMM_enabled)
		{
			// Project query onto nearest feature vector

			float best_cost = FLT_MAX;
			bool transition = false;

			projector_evaluate( //MMlmm에 정의되어 있음
				transition,
				best_cost,
				Features_proj,
				Latent_proj,
				Projector_evaluation,
				query,
				DB.features_offset,
				DB.features_scale,
				Features_curr,
				Projector,
				0.0f);

			// If projection is sufficiently different from current
			if (transition)
			{
				// Evaluate pose for projected features
				decompressor_evaluate(
					Trns_bone_positions,
					Trns_bone_velocities,
					Trns_bone_rotations,
					Trns_bone_angular_velocities,
					Trns_bone_contacts,
					Decompressor_evaluation,
					Features_proj,
					Latent_proj,
					Curr_bone_positions(0),
					Curr_bone_rotations(0),
					Decompressor,
					DeltaT);

				// Transition inertializer to this pose
				inertialize_pose_transition(
					Bone_offset_positions,
					Bone_offset_velocities,
					Bone_offset_rotations,
					Bone_offset_angular_velocities,
					Transition_src_position,
					Transition_src_rotation,
					Transition_dst_position,
					Transition_dst_rotation,
					Bone_positions(0),
					Bone_velocities(0),
					Bone_rotations(0),
					Bone_angular_velocities(0),
					Curr_bone_positions,
					Curr_bone_velocities,
					Curr_bone_rotations,
					Curr_bone_angular_velocities,
					Trns_bone_positions,
					Trns_bone_velocities,
					Trns_bone_rotations,
					Trns_bone_angular_velocities);

				// Update current features and latents
				Features_curr = Features_proj;
				Latent_curr = Latent_proj;
			}
		}
		else
		{
			// Search

			int best_index = end_of_anim ? -1 : Frame_index;
			float best_cost = FLT_MAX;

			database_search(
				best_index,
				best_cost,
				DB,
				query,
				0.0f,
				20,
				20);

			// Transition if better frame found

			if (best_index != Frame_index)
			{
				Trns_bone_positions = DB.bone_positions(best_index);
				Trns_bone_velocities = DB.bone_velocities(best_index);
				Trns_bone_rotations = DB.bone_rotations(best_index);
				Trns_bone_angular_velocities = DB.bone_angular_velocities(best_index);

				inertialize_pose_transition(
					Bone_offset_positions,
					Bone_offset_velocities,
					Bone_offset_rotations,
					Bone_offset_angular_velocities,
					Transition_src_position,
					Transition_src_rotation,
					Transition_dst_position,
					Transition_dst_rotation,
					Bone_positions(0),
					Bone_velocities(0),
					Bone_rotations(0),
					Bone_angular_velocities(0),
					Curr_bone_positions,
					Curr_bone_velocities,
					Curr_bone_rotations,
					Curr_bone_angular_velocities,
					Trns_bone_positions,
					Trns_bone_velocities,
					Trns_bone_rotations,
					Trns_bone_angular_velocities);

				Frame_index = best_index;
			}
		}

		// Reset search timer
		Search_timer = Search_time;
	}

	// Tick down search timer
	Search_timer -= DeltaT;

	if (LMM_enabled)
	{
		// Update features and latents
		stepper_evaluate(
			Features_curr,
			Latent_curr,
			Stepper_evaluation,
			Stepper,
			DeltaT);

		// Decompress next pose
		decompressor_evaluate(
			Curr_bone_positions,
			Curr_bone_velocities,
			Curr_bone_rotations,
			Curr_bone_angular_velocities,
			Curr_bone_contacts,
			Decompressor_evaluation,
			Features_curr,
			Latent_curr,
			Curr_bone_positions(0),
			Curr_bone_rotations(0),
			Decompressor,
			DeltaT);
	}
	else
	{
		// Tick frame
		Frame_index++; // Assumes dt is fixed to 60fps

		// Look-up Next Pose
		Curr_bone_positions = DB.bone_positions(Frame_index);
		Curr_bone_velocities = DB.bone_velocities(Frame_index);
		Curr_bone_rotations = DB.bone_rotations(Frame_index);
		Curr_bone_angular_velocities = DB.bone_angular_velocities(Frame_index);
		Curr_bone_contacts = DB.contact_states(Frame_index);
	}

	// Update inertializer
	inertialize_pose_update(
		Bone_positions,
		Bone_velocities,
		Bone_rotations,
		Bone_angular_velocities,
		Bone_offset_positions,
		Bone_offset_velocities,
		Bone_offset_rotations,
		Bone_offset_angular_velocities,
		Curr_bone_positions,
		Curr_bone_velocities,
		Curr_bone_rotations,
		Curr_bone_angular_velocities,
		Transition_src_position,
		Transition_src_rotation,
		Transition_dst_position,
		Transition_dst_rotation,
		Inertialize_blending_halflife,
		DeltaT);
	
	// Update Simulation
	vec3 simulation_position_prev = Simulation_position;

	simulation_positions_update(
		Simulation_position,
		Simulation_velocity,
		Simulation_acceleration,
		Desired_velocity,
		Simulation_velocity_halflife,
		DeltaT,
		Obstacles_positions,
		Obstacles_scales);

	simulation_rotations_update(
		Simulation_rotation,
		Simulation_angular_velocity,
		Desired_rotation,
		Simulation_rotation_halflife,
		DeltaT);

	// Synchronization 
	if (Synchronization_enabled)
	{
		vec3 synchronized_position = lerp(
			Simulation_position,
			Bone_positions(0),
			Synchronization_data_factor);

		quat synchronized_rotation = quat_nlerp_shortest(
			Simulation_rotation,
			Bone_rotations(0),
			Synchronization_data_factor);

		synchronized_position = simulation_collide_obstacles(
			simulation_position_prev,
			synchronized_position,
			Obstacles_positions,
			Obstacles_scales);

		Simulation_position = synchronized_position;
		Simulation_rotation = synchronized_rotation;

		inertialize_root_adjust(
			Bone_offset_positions(0),
			Transition_src_position,
			Transition_src_rotation,
			Transition_dst_position,
			Transition_dst_rotation,
			Bone_positions(0),
			Bone_rotations(0),
			synchronized_position,
			synchronized_rotation);
	}


	// Adjustment 
	if (!Synchronization_enabled && Adjustment_enabled)
	{
		vec3 adjusted_position = Bone_positions(0);
		quat adjusted_rotation = Bone_rotations(0);

		if (Adjustment_by_velocity_enabled)
		{
			adjusted_position = adjust_character_position_by_velocity(
				Bone_positions(0),
				Bone_velocities(0),
				Simulation_position,
				Adjustment_position_max_ratio,
				Adjustment_position_halflife,
				DELTA);

			adjusted_rotation = adjust_character_rotation_by_velocity(
				Bone_rotations(0),
				Bone_angular_velocities(0),
				Simulation_rotation,
				Adjustment_rotation_max_ratio,
				Adjustment_rotation_halflife,
				DeltaT);
		}
		else
		{
			adjusted_position = adjust_character_position(
				Bone_positions(0),
				Simulation_position,
				Adjustment_position_halflife,
				DeltaT);

			adjusted_rotation = adjust_character_rotation(
				Bone_rotations(0),
				Simulation_rotation,
				Adjustment_rotation_halflife,
				DELTA);
		}

		inertialize_root_adjust(
			Bone_offset_positions(0),
			Transition_src_position,
			Transition_src_rotation,
			Transition_dst_position,
			Transition_dst_rotation,
			Bone_positions(0),
			Bone_rotations(0),
			adjusted_position,
			adjusted_rotation);
	}

	// Clamping
	if (!Synchronization_enabled && Clamping_enabled)
	{
		vec3 adjusted_position = Bone_positions(0);
		quat adjusted_rotation = Bone_rotations(0);

		adjusted_position = clamp_character_position(
			adjusted_position,
			Simulation_position,
			Clamping_max_distance);

		adjusted_rotation = clamp_character_rotation(
			adjusted_rotation,
			Simulation_rotation,
			Clamping_max_angle);

		inertialize_root_adjust(
			Bone_offset_positions(0),
			Transition_src_position,
			Transition_src_rotation,
			Transition_dst_position,
			Transition_dst_rotation,
			Bone_positions(0),
			Bone_rotations(0),
			adjusted_position,
			adjusted_rotation);
	}


	// Contact fixup with foot locking and IK
	Adjusted_bone_positions = Bone_positions;
	Adjusted_bone_rotations = Bone_rotations;

	if (Ik_enabled)
	{
		for (int i = 0; i < Contact_bones.size; i++)
		{
			// Find all the relevant bone indices
			int toe_bone = Contact_bones(i);
			int heel_bone = DB.bone_parents(toe_bone);
			int knee_bone = DB.bone_parents(heel_bone);
			int hip_bone = DB.bone_parents(knee_bone);
			int root_bone = DB.bone_parents(hip_bone);

			// Compute the world space position for the toe
			Global_bone_computed.zero();

			forward_kinematics_partial(
				Global_bone_positions,
				Global_bone_rotations,
				Global_bone_computed,
				Bone_positions,
				Bone_rotations,
				DB.bone_parents,
				toe_bone);

			// Update the contact state
			contact_update(
				Contact_states(i),
				Contact_locks(i),
				Contact_positions(i),
				Contact_velocities(i),
				Contact_points(i),
				Contact_targets(i),
				Contact_offset_positions(i),
				Contact_offset_velocities(i),
				Global_bone_positions(toe_bone),
				Curr_bone_contacts(i),
				Ik_unlock_radius,
				Ik_foot_height,
				Ik_blending_halflife,
				DeltaT);

			// Ensure contact position never goes through floor
			vec3 contact_position_clamp = Contact_positions(i);
			contact_position_clamp.y = maxf(contact_position_clamp.y, Ik_foot_height);

			// Re-compute toe, heel, knee, hip, and root bone positions
			for (int bone : {heel_bone, knee_bone, hip_bone, root_bone})
			{
				forward_kinematics_partial(
					Global_bone_positions,
					Global_bone_rotations,
					Global_bone_computed,
					Bone_positions,
					Bone_rotations,
					DB.bone_parents,
					bone);
			}

			// Perform simple two-joint IK to place heel
			ik_two_bone(
				Adjusted_bone_rotations(hip_bone),
				Adjusted_bone_rotations(knee_bone),
				Global_bone_positions(hip_bone),
				Global_bone_positions(knee_bone),
				Global_bone_positions(heel_bone),
				contact_position_clamp + (Global_bone_positions(heel_bone) - Global_bone_positions(toe_bone)),
				quat_mul_vec3(Global_bone_rotations(knee_bone), vec3(0.0f, 1.0f, 0.0f)),
				Global_bone_rotations(hip_bone),
				Global_bone_rotations(knee_bone),
				Global_bone_rotations(root_bone),
				Ik_max_length_buffer);

			// Re-compute toe, heel, and knee positions 
			Global_bone_computed.zero();

			for (int bone : {toe_bone, heel_bone, knee_bone})
			{
				forward_kinematics_partial(
					Global_bone_positions,
					Global_bone_rotations,
					Global_bone_computed,
					Adjusted_bone_positions,
					Adjusted_bone_rotations,
					DB.bone_parents,
					bone);
			}

			// Rotate heel so toe is facing toward contact point
			ik_look_at(
				Adjusted_bone_rotations(heel_bone),
				Global_bone_rotations(knee_bone),
				Global_bone_rotations(heel_bone),
				Global_bone_positions(heel_bone),
				Global_bone_positions(toe_bone),
				contact_position_clamp);

			// Re-compute toe and heel positions
			Global_bone_computed.zero();

			for (int bone : {toe_bone, heel_bone})
			{
				forward_kinematics_partial(
					Global_bone_positions,
					Global_bone_rotations,
					Global_bone_computed,
					Adjusted_bone_positions,
					Adjusted_bone_rotations,
					DB.bone_parents,
					bone);
			}

			// Rotate toe bone so that the end of the toe 
			// does not intersect with the ground
			vec3 toe_end_curr = quat_mul_vec3(
				Global_bone_rotations(toe_bone), vec3(Ik_toe_length, 0.0f, 0.0f)) +
				Global_bone_positions(toe_bone);

			vec3 toe_end_targ = toe_end_curr;
			toe_end_targ.y = maxf(toe_end_targ.y, Ik_foot_height);

			ik_look_at(
				Adjusted_bone_rotations(toe_bone),
				Global_bone_rotations(heel_bone),
				Global_bone_rotations(toe_bone),
				Global_bone_positions(toe_bone),
				toe_end_curr,
				toe_end_targ);
		}
	}

	// Full pass of forward kinematics to compute 
	// all bone positions and rotations in the world
	// space ready for rendering
	forward_kinematics_full(
		Global_bone_positions,
		Global_bone_rotations,
		Adjusted_bone_positions,
		Adjusted_bone_rotations,
		DB.bone_parents);

	//----------------------------------------------------------------------------------
	//여기서부터는 unreal engine 형식에 맞게 변환하는 작업이 필요함

	// Update camera
	orbit_camera_update(
		Camera_azimuth,
		Camera_altitude,
		Camera_distance,
		//Bone_positions(0) + vec3(0, 1, 0), //이것이 카메라의 target의 위치가 될 예정임
		// simulation_position + vec3(0, 1, 0),
		gamepadstick_right,
		Desired_strafe,
		DeltaT);
	
	// Render
	SetSimulationObj();
	SetCharacterAnimation();

	//Draw matched features
	array1d<float> current_features = LMM_enabled ? slice1d<float>(Features_curr) : DB.features(Frame_index);
	denormalize_features(current_features, DB.features_offset, DB.features_scale);
	Draw_features(current_features, Bone_positions(0), Bone_rotations(0), FColor::Blue);



	//Draw traectory
	Draw_trajectory(Trajectory_positions, Trajectory_rotations, FColor::Orange);

	//Draw simulation object
	Draw_simulation_object(Simulation_position, Simulation_rotation, FColor::Orange);

	//// Draw Clamping Radius/Angles
	//if (clamping_enabled)
	//{
	//	DrawCylinderWires(
	//		to_Vector3(simulation_position),
	//		clamping_max_distance,
	//		clamping_max_distance,
	//		0.001f, 17, SKYBLUE);

	//	quat rotation_clamp_0 = quat_mul(quat_from_angle_axis(+clamping_max_angle, vec3(0.0f, 1.0f, 0.0f)), simulation_rotation);
	//	quat rotation_clamp_1 = quat_mul(quat_from_angle_axis(-clamping_max_angle, vec3(0.0f, 1.0f, 0.0f)), simulation_rotation);

	//	vec3 rotation_clamp_0_dir = simulation_position + 0.6f * quat_mul_vec3(rotation_clamp_0, vec3(0.0f, 0.0f, 1.0f));
	//	vec3 rotation_clamp_1_dir = simulation_position + 0.6f * quat_mul_vec3(rotation_clamp_1, vec3(0.0f, 0.0f, 1.0f));

	//	DrawLine3D(to_Vector3(simulation_position), to_Vector3(rotation_clamp_0_dir), SKYBLUE);
	//	DrawLine3D(to_Vector3(simulation_position), to_Vector3(rotation_clamp_1_dir), SKYBLUE);
	//}

	//// Draw IK foot lock positions

	//if (ik_enabled)
	//{
	//	for (int i = 0; i < contact_positions.size; i++)
	//	{
	//		if (contact_locks(i))
	//		{
	//			DrawSphereWires(to_Vector3(contact_positions(i)), 0.05f, 4, 10, PINK);
	//		}
	//	}
	//}

	//draw_trajectory(
	//	trajectory_positions,
	//	trajectory_rotations,
	//	ORANGE);

	//draw_obstacles(
	//	obstacles_positions,
	//	obstacles_scales);

	//deform_character_mesh(
	//	character_mesh,
	//	character_data,
	//	global_bone_positions,
	//	global_bone_rotations,
	//	db.bone_parents);

	//DrawModel(character_model, (Vector3) { 0.0f, 0.0f, 0.0f }, 1.0f, RAYWHITE);

	//// Draw matched features

	//array1d<float> current_features = lmm_enabled ? slice1d<float>(features_curr) : db.features(frame_index);
	//denormalize_features(current_features, db.features_offset, db.features_scale);
	//draw_features(current_features, bone_positions(0), bone_rotations(0), MAROON);

	//// Draw Simuation Bone

	//DrawSphereWires(to_Vector3(bone_positions(0)), 0.05f, 4, 10, MAROON);
	//DrawLine3D(to_Vector3(bone_positions(0)), to_Vector3(
	//	bone_positions(0) + 0.6f * quat_mul_vec3(bone_rotations(0), vec3(0.0f, 0.0f, 1.0f))), MAROON);

	//// Draw Ground Plane

	//DrawModel(ground_plane_model, (Vector3) { 0.0f, -0.01f, 0.0f }, 1.0f, WHITE);
	//DrawGrid(20, 1.0f);
	//draw_axis(vec3(), quat());

	//EndMode3D();

	//--------------------------------------------------------------------------------


	bool ik_enabled_prev = Ik_enabled;

	// Foot locking needs resetting when IK is toggled
	if (Ik_enabled && !ik_enabled_prev)
	{
		for (int i = 0; i < Contact_bones.size; i++)
		{
			vec3 bone_position;
			vec3 bone_velocity;
			quat bone_rotation;
			vec3 bone_angular_velocity;

			forward_kinematics_velocity(
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

			contact_reset(
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
	}

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
	//if (Controller != nullptr)
	//{
	//	// find out which way is forward
	//	const FRotator Rotation = Controller->GetControlRotation();
	//	const FRotator YawRotation(0, Rotation.Yaw, 0);

	//	// get forward vector
	//	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	//	// get right vector 
	//	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);


	//	UE_LOG(LogTemp, Log, TEXT("ForwardDirection: "));
	//	UE_LOG(LogTemp, Log, TEXT("x: %f, y: %f, z: %f"), ForwardDirection.X, ForwardDirection.Y, ForwardDirection.Z);

	//	UE_LOG(LogTemp, Log, TEXT("RightDirection: "));
	//	UE_LOG(LogTemp, Log, TEXT("x: %f, y: %f, z: %f"), RightDirection.X, RightDirection.Y, RightDirection.Z);
	//}

	////-------------------------------------------
	////키보드 입력 부분

	////Add Input Mapping Context
	//if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	//{
	//	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
	//	{
	//		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	//	}

	//	if (PlayerController->IsInputKeyDown(EKeys::A)) {
	//		UE_LOG(LogTemp, Log, TEXT("A is pressed"));
	//	}
	//	if (PlayerController->IsInputKeyDown(EKeys::S)) {
	//		UE_LOG(LogTemp, Log, TEXT("S is pressed"));
	//	}
	//	if (PlayerController->IsInputKeyDown(EKeys::D)) {
	//		UE_LOG(LogTemp, Log, TEXT("D is pressed"));
	//	}
	//	if (PlayerController->IsInputKeyDown(EKeys::W)) {
	//		UE_LOG(LogTemp, Log, TEXT("W is pressed"));
	//	}
	//}

	////-------------------------------------------
	//// Strafe 동작 테스트
	//if (Desired_strafe == true)
	//{
	//	UE_LOG(LogTemp, Log, TEXT("Strafe ON!!"));
	//}
	//else
	//{
	//	UE_LOG(LogTemp, Log, TEXT("Strafe OFF!!"));
	//}


	////-------------------------------------------

	////좌측 조이스틱(이동) 키를 입력하지 않으면 입력값 초기화.
	//APlayerController* PlayerController = Cast<APlayerController>(Controller);
	//if (PlayerController)
	//{
	//	if (!PlayerController->IsInputKeyDown(EKeys::Gamepad_LeftStick_Down) && !PlayerController->IsInputKeyDown(EKeys::Gamepad_LeftStick_Up)
	//		&& !PlayerController->IsInputKeyDown(EKeys::Gamepad_LeftStick_Left) && !PlayerController->IsInputKeyDown(EKeys::Gamepad_LeftStick_Right))
	//	{
	//		LeftStickValue2D = FVector2D(0, 0);
	//	}
	//	else {
	//		UE_LOG(LogTemp, Log, TEXT("LeftStick2D"));
	//		UE_LOG(LogTemp, Log, TEXT("X: %f, Y: %f"), LeftStickValue2D.X, LeftStickValue2D.Y);
	//	}
	//}

	////우측 조이스틱(룩) 키를 입력하지 않으면 입력값 초기화.
	//if (PlayerController)
	//{
	//	if (!PlayerController->IsInputKeyDown(EKeys::Gamepad_RightStick_Down) && !PlayerController->IsInputKeyDown(EKeys::Gamepad_RightStick_Up)
	//		&& !PlayerController->IsInputKeyDown(EKeys::Gamepad_RightStick_Left) && !PlayerController->IsInputKeyDown(EKeys::Gamepad_RightStick_Right))
	//	{
	//		RightStickValue2D = FVector2D(0, 0);
	//	}
	//	else {
	//		UE_LOG(LogTemp, Log, TEXT("RightStick2D"));
	//		UE_LOG(LogTemp, Log, TEXT("X: %f, Y: %f"), RightStickValue2D.X, RightStickValue2D.Y);
	//	}
	//}



	////캐릭터의 월드 위치
	//FVector CharacterWorldPos = GetActorLocation();
	//UE_LOG(LogTemp, Log, TEXT("Character World Location: "));
	//UE_LOG(LogTemp, Log, TEXT("x: %f, y: %f, z: %f"), CharacterWorldPos.X, CharacterWorldPos.Y, CharacterWorldPos.Z);

	////카메라의 월드 위치
	//FVector CamWorldPos = FollowCamera->GetComponentLocation();
	//UE_LOG(LogTemp, Log, TEXT("Cam World Location: "));
	//UE_LOG(LogTemp, Log, TEXT("x: %f, y: %f, z: %f"), CamWorldPos.X, CamWorldPos.Y, CamWorldPos.Z);


	////-------------------------------------------
	////Zoom input 테스트
	//if (CamZoomIn == true)
	//{
	//	UE_LOG(LogTemp, Log, TEXT("Cam Zoom In"));
	//}
	//else
	//{
	//	;
	//}

	//if (CamZoomOut == true)
	//{
	//	UE_LOG(LogTemp, Log, TEXT("Cam Zoom Out"));
	//}
	//else
	//{
	//	;
	//}

	////-------------------------------------------
	////Gait input 테스트
	//if (GaitButton == true)
	//{
	//	UE_LOG(LogTemp, Log, TEXT("Gait button on"));
	//}
	//else
	//{
	//	UE_LOG(LogTemp, Log, TEXT("Gait button off"));
	//}


	////-----------------------------------
	////Obstacle이 저장되었는지 확인
	////array1d<vec3> Obstacles_positions = array1d<vec3>(0);
	////array1d<vec3> Obstacles_scales = array1d<vec3>(0);

	//UE_LOG(LogTemp, Log, TEXT("Num of Obstacles : %d"), Obstacles_positions.size);

	//for (int i = 0; i < Obstacles_positions.size; i++) {

	//	UE_LOG(LogTemp, Log, TEXT("Obstacle %i position: x: %f, y: %f, z: %f"), i, Obstacles_positions.data[i].x, Obstacles_positions.data[i].y, Obstacles_positions.data[i].z);

	//}

	//for (int i = 0; i < Obstacles_scales.size; i++) {

	//	UE_LOG(LogTemp, Log, TEXT("Obstacle %i scale: x: %f, y: %f, z: %f"), i, Obstacles_scales.data[i].x, Obstacles_scales.data[i].y, Obstacles_scales.data[i].z);

	//}


	//--------------------------------------
	//Log Delta time
	UE_LOG(LogTemp, Log, TEXT("Delta Time: %f"), DeltaT);


}
