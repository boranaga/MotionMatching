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

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}


	//test용으로 한번 내가 추가함
	//std::cout << "이게 되네?"; //일단 이건 안됨
	
	//testfunc();
	//UE_LOG(LogTemp, Log, TEXT("HIHIHiHIHIhIhIhI22222"));

	float test_a = 0.1f;
	float test_b = 0.5f;
	float x = 0.3f;

	float result = clampf(x, test_a, test_b);



	vec2 test_vec1 = vec2(test_a, test_b);
	vec2 test_vec2 = vec2(test_a, 0.0f);

	vec2 vec_result = test_vec1 - test_vec2;


	UE_LOG(LogTemp, Log, TEXT("MyFloat value: %f"), result);
	UE_LOG(LogTemp, Log, TEXT("MyFloat value: %f, %f"), vec_result.x, vec_result.y);


	//LoadBinaryFile("E:/Unreal_engine_projects/MotionMatching/database.bin");

	//---------------------------------------------------
	
	//Character skeleton의 기본 roator 값을 배열로 저장
	SaveBasicRotators();
	//Character skeleton의 기본 vector 값을 배열로 저장
	SaveBasicVectors();


	//Character.bin load test
	CharacterLoadTest();

	//MotionMatching BeginPlay
	MotionMatchingMainBeginPlay();

	//PoseTest
	//PoseTest();
	//PoseTestByPostion(0);
	//PoseTest2(0); //우선은 프레임 0의 포즈 출력해 봄

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
//<사용자 정의 함수 정의 부분>



//UCharacter 클래스에 존재하는 GetMesh() 함수 재정의 -> PosealbeMeshComponent를 가져옴.
UPoseableMeshComponent* AMotionMatchingCharacter::GetMesh() const
{
	return PoseableMesh;
}



void AMotionMatchingCharacter::PoseTest() {


	//수정한 코드 (x y z w)
	FQuat Root = FQuat(-0.00000000e+00, -7.20359683e-01, 0.00000000e+00, 6.93600714e-01);
	FQuat Hips = FQuat(5.10667503e-01, 4.95325178e-01, 5.04266381e-01, 4.89476293e-01);

	FQuat LeftUpLeg = FQuat(2.53748521e-02, 9.90944326e-01, 1.31835669e-01, 2.17995676e-03);
	FQuat LeftLeg = FQuat(-8.28921720e-02, 2.09991354e-02, -1.01421498e-01, 9.91161764e-01);
	FQuat LeftFoot = FQuat(-6.12904504e-02, 1.27482358e-02, 6.35808527e-01, 7.69303858e-01);
	FQuat LeftToe = FQuat(-5.01755130e-06, 2.64855244e-05, 1.86134368e-01, 9.82524276e-01);

	FQuat RightUpLeg = FQuat(6.63435310e-02, 9.96164322e-01, 5.69788963e-02, -2.91720871e-03);
	FQuat RightLeg = FQuat(2.46823877e-02, -4.81897965e-02, -1.52169123e-01, 9.86870348e-01);
	FQuat RightFoot = FQuat(5.02567403e-02, 3.30432840e-02, 6.42255843e-01, 7.64126837e-01);
	FQuat RightToe = FQuat(4.90265484e-06, -2.66760471e-05, 1.86134413e-01, 9.82524276e-01);

	FQuat Spine = FQuat(-6.09852746e-03, 5.04404353e-03, 5.49792647e-02, 9.98456120e-01);
	FQuat Spine1 = FQuat(-1.17475521e-02, 1.06054097e-02, 2.26353761e-02, 9.99618471e-01);
	FQuat Spine2 = FQuat(-1.17821014e-02, 1.05682043e-02, 1.98605321e-02, 9.99677479e-01);

	FQuat Neck = FQuat(-2.18032510e-03, -7.87054747e-02, -6.44085109e-02, 9.94812667e-01);
	FQuat Head = FQuat(-4.80575450e-02, 5.41895144e-02, 8.09299946e-03, 9.97340679e-01);

	FQuat LeftShoulder = FQuat(-6.07465804e-01, 4.18190621e-02, -7.93006480e-01, 1.94211192e-02);
	FQuat LeftArm = FQuat(2.14652315e-01, 4.35188383e-01, -2.56695211e-01, 8.35848689e-01);
	FQuat LeftForeArm = FQuat(-9.66933277e-03, -6.76918477e-02, -2.28246704e-01, 9.71199155e-01);
	FQuat LeftHand = FQuat(2.77632058e-01, -1.67130064e-02, 1.20311685e-01, 9.52977538e-01);

	FQuat RightShoulder = FQuat(6.05823994e-01, -4.13361564e-02, -7.94293225e-01, 1.91542506e-02);
	FQuat RightArm = FQuat(-2.86332458e-01, -4.24856454e-01, -1.92388535e-01, 8.36957216e-01);
	FQuat RightForeArm = FQuat(2.23726705e-02, 1.18756600e-01, -2.66182274e-01, 9.56317604e-01);
	FQuat RightHand = FQuat(-1.00682385e-01, 1.92709230e-02, 9.08031687e-02, 9.90578830e-01);


	FQuat Root2 = FQuat(1, 0, 0, 90);

	FQuat LeftUpLeg2 = FQuat(0, 0, 1, 90);
	FQuat LeftLeg2 = FQuat(0, 1, 0, 90);
	FQuat LeftFoot2 = FQuat(0, 1, 0, 90);
	FQuat LeftToe2 = FQuat(0, 0, 1, 90);



	////현재 로테이터에 새로운 값 더한 것
	GetMesh()->SetBoneRotationByName(FName(TEXT("Root")), Root.Rotator() + GetMesh()->GetBoneRotationByName(FName(TEXT("Root")), EBoneSpaces::WorldSpace), EBoneSpaces::WorldSpace);

	//GetMesh()->SetBoneRotationByName(FName(TEXT("Root")), Root.Rotator() + GetMesh()->GetBoneRotationByName(FName(TEXT("Root")), EBoneSpaces::ComponentSpace), EBoneSpaces::ComponentSpace);

	GetMesh()->SetBoneRotationByName(FName(TEXT("Hips")), Hips.Rotator() + GetMesh()->GetBoneRotationByName(FName(TEXT("Hips")), EBoneSpaces::ComponentSpace), EBoneSpaces::ComponentSpace);

	GetMesh()->SetBoneRotationByName(FName(TEXT("LeftUpLeg")), LeftUpLeg.Rotator() + GetMesh()->GetBoneRotationByName(FName(TEXT("LeftUpLeg")), EBoneSpaces::ComponentSpace), EBoneSpaces::ComponentSpace);
	GetMesh()->SetBoneRotationByName(FName(TEXT("LeftLeg")), LeftLeg.Rotator() + GetMesh()->GetBoneRotationByName(FName(TEXT("LeftLeg")), EBoneSpaces::ComponentSpace), EBoneSpaces::ComponentSpace);
	GetMesh()->SetBoneRotationByName(FName(TEXT("LeftFoot")), LeftFoot.Rotator() + GetMesh()->GetBoneRotationByName(FName(TEXT("LeftFoot")), EBoneSpaces::ComponentSpace), EBoneSpaces::ComponentSpace);
	GetMesh()->SetBoneRotationByName(FName(TEXT("LeftToe")), LeftToe.Rotator() + GetMesh()->GetBoneRotationByName(FName(TEXT("LeftToe")), EBoneSpaces::ComponentSpace), EBoneSpaces::ComponentSpace);

	GetMesh()->SetBoneRotationByName(FName(TEXT("RightUpLeg")), RightUpLeg.Rotator() + GetMesh()->GetBoneRotationByName(FName(TEXT("RightUpLeg")), EBoneSpaces::ComponentSpace), EBoneSpaces::ComponentSpace);
	GetMesh()->SetBoneRotationByName(FName(TEXT("RightLeg")), RightLeg.Rotator() + GetMesh()->GetBoneRotationByName(FName(TEXT("RightLeg")), EBoneSpaces::ComponentSpace), EBoneSpaces::ComponentSpace);
	GetMesh()->SetBoneRotationByName(FName(TEXT("RightFoot")), RightFoot.Rotator() + GetMesh()->GetBoneRotationByName(FName(TEXT("RightFoot")), EBoneSpaces::ComponentSpace), EBoneSpaces::ComponentSpace);
	GetMesh()->SetBoneRotationByName(FName(TEXT("RightToe")), RightToe.Rotator() + GetMesh()->GetBoneRotationByName(FName(TEXT("RightToe")), EBoneSpaces::ComponentSpace), EBoneSpaces::ComponentSpace);

	GetMesh()->SetBoneRotationByName(FName(TEXT("Spine")), Spine.Rotator() + GetMesh()->GetBoneRotationByName(FName(TEXT("Spine")), EBoneSpaces::ComponentSpace), EBoneSpaces::ComponentSpace);
	GetMesh()->SetBoneRotationByName(FName(TEXT("Spine1")), Spine1.Rotator() + GetMesh()->GetBoneRotationByName(FName(TEXT("Spine1")), EBoneSpaces::ComponentSpace), EBoneSpaces::ComponentSpace);
	GetMesh()->SetBoneRotationByName(FName(TEXT("Spine2")), Spine2.Rotator() + GetMesh()->GetBoneRotationByName(FName(TEXT("Spine2")), EBoneSpaces::ComponentSpace), EBoneSpaces::ComponentSpace);

	GetMesh()->SetBoneRotationByName(FName(TEXT("Neck")), Neck.Rotator() + GetMesh()->GetBoneRotationByName(FName(TEXT("Neck")), EBoneSpaces::ComponentSpace), EBoneSpaces::ComponentSpace);
	GetMesh()->SetBoneRotationByName(FName(TEXT("Head")), Head.Rotator() + GetMesh()->GetBoneRotationByName(FName(TEXT("Head")), EBoneSpaces::ComponentSpace), EBoneSpaces::ComponentSpace);

	GetMesh()->SetBoneRotationByName(FName(TEXT("LeftShoulder")), LeftShoulder.Rotator() + GetMesh()->GetBoneRotationByName(FName(TEXT("LeftShoulder")), EBoneSpaces::ComponentSpace), EBoneSpaces::ComponentSpace);
	GetMesh()->SetBoneRotationByName(FName(TEXT("LeftArm")), LeftArm.Rotator() + GetMesh()->GetBoneRotationByName(FName(TEXT("LeftArm")), EBoneSpaces::ComponentSpace), EBoneSpaces::ComponentSpace);
	GetMesh()->SetBoneRotationByName(FName(TEXT("LeftForeArm")), LeftForeArm.Rotator() + GetMesh()->GetBoneRotationByName(FName(TEXT("LeftForeArm")), EBoneSpaces::ComponentSpace), EBoneSpaces::ComponentSpace);
	GetMesh()->SetBoneRotationByName(FName(TEXT("LeftHand")), LeftHand.Rotator() + GetMesh()->GetBoneRotationByName(FName(TEXT("LeftHand")), EBoneSpaces::ComponentSpace), EBoneSpaces::ComponentSpace);

	GetMesh()->SetBoneRotationByName(FName(TEXT("RightShoulder")), RightShoulder.Rotator() + GetMesh()->GetBoneRotationByName(FName(TEXT("RightShoulder")), EBoneSpaces::ComponentSpace), EBoneSpaces::ComponentSpace);
	GetMesh()->SetBoneRotationByName(FName(TEXT("RightArm")), RightArm.Rotator() + GetMesh()->GetBoneRotationByName(FName(TEXT("RightArm")), EBoneSpaces::ComponentSpace), EBoneSpaces::ComponentSpace);
	GetMesh()->SetBoneRotationByName(FName(TEXT("RightForeArm")), RightForeArm.Rotator() + GetMesh()->GetBoneRotationByName(FName(TEXT("RightForeArm")), EBoneSpaces::ComponentSpace), EBoneSpaces::ComponentSpace);
	GetMesh()->SetBoneRotationByName(FName(TEXT("RightHand")), RightHand.Rotator() + GetMesh()->GetBoneRotationByName(FName(TEXT("RightHand")), EBoneSpaces::ComponentSpace), EBoneSpaces::ComponentSpace);



}


void AMotionMatchingCharacter::PoseTest2(int frameindex) {

	float pi = 3.141592;

	//int scale = (180/pi);
	int scale = 1;

	//관절의 개수
	int JointsNum = JointsNames2.Num();

	//db로부터 원하는 frame의 rotation data 가져옴
	array1d<quat> curr_bone_rotations = db.bone_rotations(frameindex);


	//현재 프레임의 쿼터니언 데이터 저장 //curr_bone_rotations.size
	TArray<FQuat> JointsQuat; 

	for (int i = 0; i < JointsNum; i++) {

		//JointsQuat.Emplace(FQuat(curr_bone_rotations.data[i].x * scale, curr_bone_rotations.data[i].y * scale, curr_bone_rotations.data[i].z * scale, curr_bone_rotations.data[i].w * scale));

		//JointsQuat.Emplace(FQuat(curr_bone_rotations.data[i].y * (-1), curr_bone_rotations.data[i].z * (-1), curr_bone_rotations.data[i].x * (-1), curr_bone_rotations.data[i].w * scale) * (-1));

		//현재로서는 가장 근접
		//JointsQuat.Emplace(FQuat(curr_bone_rotations.data[i].x * (1), curr_bone_rotations.data[i].z * (1), curr_bone_rotations.data[i].y * (1), curr_bone_rotations.data[i].w * scale* (1)));

		
		//acos(curr_bone_rotations.data[i].w) * 2 * (180 / pi)

		//JointsQuat.Emplace(FQuat(curr_bone_rotations.data[i].x * (1), curr_bone_rotations.data[i].z * (-1), curr_bone_rotations.data[i].y * (1), curr_bone_rotations.data[i].w * scale * (1)) );

		//JointsQuat.Emplace(FQuat(curr_bone_rotations.data[i].x * (1), curr_bone_rotations.data[i].z * (1), curr_bone_rotations.data[i].y * (1), acos(curr_bone_rotations.data[i].w) * 2 * (180 / pi)));

		JointsQuat.Emplace(FQuat(curr_bone_rotations.data[i].y * (1), curr_bone_rotations.data[i].x * (1), curr_bone_rotations.data[i].z * (-1), curr_bone_rotations.data[i].w));
	}


	//Set pose
	//GetMesh()->SetBoneRotationByName(FName(JointsNames2[0]), JointsQuat[0].Rotator() + BasicCharatorRotator[0], EBoneSpaces::ComponentSpace);
	//GetMesh()->SetBoneRotationByName(FName(JointsNames2[0]), JointsQuat[0].Rotator() + BasicCharatorRotator[0], EBoneSpaces::WorldSpace); //basic에 rotation 더한 값

	GetMesh()->SetBoneRotationByName(FName(JointsNames2[0]), JointsQuat[0].Rotator(), EBoneSpaces::WorldSpace);

	//GetMesh()->SetBoneRotationByName(FName(JointsNames2[0]), JointsQuat[0].Rotator(), EBoneSpaces::WorldSpace); //그냥 rotation만 적용 시키는 것

	for (int i = 1; i < JointsNum; i++) {

		//FRotator Rotation = JointsQuat[i].Rotator() + BasicCharatorRotator[i];
		FRotator Rotation = JointsQuat[i].Rotator();

		GetMesh()->SetBoneRotationByName(FName(JointsNames2[i]), Rotation, EBoneSpaces::ComponentSpace);

	}
}


void AMotionMatchingCharacter::PoseTestByPostion(int frameindex) {

	int scale = 100;

	//관절의 개수
	int JointsNum = JointsNames2.Num();

	//db로부터 원하는 frame의 rotation data 가져옴
	array1d<vec3> curr_bone_positions = db.bone_positions(frameindex);

	//현재 프레임의 position 데이터 저장 //curr_bone_rotations.size
	TArray<FVector> JointsVector;
	for (int i = 0; i < JointsNum; i++) {
		//JointsVector.Emplace(FVector(curr_bone_positions.data[i].x * scale, curr_bone_positions.data[i].y * scale, curr_bone_positions.data[i].z * scale));

		JointsVector.Emplace(FVector(curr_bone_positions.data[i].x * scale, curr_bone_positions.data[i].z * scale, curr_bone_positions.data[i].x * scale));
	}

	//Set Pose
	GetMesh()->SetBoneLocationByName(FName(JointsNames2[0]), JointsVector[0] + BasicCharatorVector[0], EBoneSpaces::WorldSpace);
	for (int i = 1; i < JointsNum; i++) {

		FVector Position = JointsVector[i] + BasicCharatorVector[i]; //basic에 position 더한 값

		GetMesh()->SetBoneLocationByName(FName(JointsNames2[i]), Position, EBoneSpaces::ComponentSpace);
	}
}




void AMotionMatchingCharacter::MotionMatchingMainBeginPlay() {

	//해당 경로의 database.bin 파일을 load 하여 db에 저장함
	database_load(db, "E:/Unreal_engine_projects/MotionMatching/database.bin");
	
}



void AMotionMatchingCharacter::MotionMatchingMainTick() {

	//모션 매칭의 본격적인 내용이 들어갈 예정임

}



void AMotionMatchingCharacter::DataBaseLog() {

	// Pose & Inertializer Data

	int frame_index = db.range_starts(0);

	array1d<vec3> curr_bone_positions = db.bone_positions(frame_index);
	array1d<vec3> curr_bone_velocities = db.bone_velocities(frame_index);
	array1d<quat> curr_bone_rotations = db.bone_rotations(frame_index);


	//UE_LOG(LogTemp, Log, TEXT("w: %f"), curr_bone_rotations(0));


	//curr_bone_rotations.data[0].w


	//db.bone_rotations.data[300];



	//db에 data가 잘 저장되었는지 Log 출력으로 확인
	UE_LOG(LogTemp, Log, TEXT("Joints Num: %d"), curr_bone_rotations.size);

	for (int i = 0; i < curr_bone_rotations.size; i++) {

		//db.bone_rotations.rows;

		UE_LOG(LogTemp, Log, TEXT("x: %f, y: %f, z: %f, w: %f"), curr_bone_rotations.data[i].x, curr_bone_rotations.data[i].y, curr_bone_rotations.data[i].z, curr_bone_rotations.data[i].w);
	}

}


void AMotionMatchingCharacter::SaveBasicRotators() {

	//Root는 WorldSpace 기준으로 저장
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

	//Root는 WorldSpace 기준으로 저장
	BasicCharatorVector.Emplace(GetMesh()->GetBoneLocationByName(FName(TEXT("Root")), EBoneSpaces::WorldSpace));

	UE_LOG(LogTemp, Log, TEXT("SaveBasicVectors()"));
	for (int i = 1; i < JointsNames2.Num(); i++) {
		BasicCharatorVector.Emplace(GetMesh()->GetBoneLocationByName(FName(JointsNames2[i]), EBoneSpaces::ComponentSpace));
		//BasicCharatorRotator.Emplace(GetMesh()->GetBoneRotationByName(FName(JointsNames2[i]), EBoneSpaces::WorldSpace));
	}
}

void AMotionMatchingCharacter::CharacterLoadTest() {

	character_load(character_data, "E:/Unreal_engine_projects/MotionMatching/character.bin");

	UE_LOG(LogTemp, Log, TEXT("positions length: %d"), character_data.positions.size);
	UE_LOG(LogTemp, Log, TEXT("bone_rest_positions length: %d"), character_data.bone_rest_positions.size);
	UE_LOG(LogTemp, Log, TEXT("bone_rest_rotations length: %d"), character_data.bone_rest_rotations.size);

	//character_bone_rest_rotations 출력
	for (int i = 0; i < character_data.bone_rest_rotations.size; i++) {
		UE_LOG(LogTemp, Log, TEXT("character_bone_rest_rotations:"));
		UE_LOG(LogTemp, Log, TEXT("x: %f, y: %f, z: %f, w: %f"), character_data.bone_rest_rotations.data[i].x, character_data.bone_rest_rotations.data[i].y, character_data.bone_rest_rotations.data[i].z, character_data.bone_rest_rotations.data[i].w);
	}
	////character_bone_rest_positions 출력
	for (int i = 0; i < character_data.bone_rest_positions.size; i++) {
		UE_LOG(LogTemp, Log, TEXT("character_bone_rest_positions:"));
		UE_LOG(LogTemp, Log, TEXT("x: %f, y: %f, z: %f"), character_data.bone_rest_positions.data[i].x, character_data.bone_rest_positions.data[i].y, character_data.bone_rest_positions.data[i].z);
	}

}

void AMotionMatchingCharacter::SetCharacterPositionRest() {

	int scale = 100;

	//관절의 개수
	int JointsNum = JointsNames2.Num();

	array1d<vec3> curr_bone_positions = character_data.bone_rest_positions;

	TArray<FVector> JointsVector;
	for (int i = 0; i < JointsNum; i++) {
		JointsVector.Emplace(FVector(curr_bone_positions.data[i].x * scale, curr_bone_positions.data[i].z * scale, curr_bone_positions.data[i].y * scale));
	}

	//Set Pose
	//GetMesh()->SetBoneLocationByName(FName(JointsNames2[0]), JointsVector[0] + BasicCharatorVector[0], EBoneSpaces::WorldSpace);
	GetMesh()->SetBoneLocationByName(FName(JointsNames2[0]), JointsVector[0], EBoneSpaces::WorldSpace);

	for (int i = 1; i < JointsNum; i++) {
		//FVector Position = JointsVector[i] + BasicCharatorVector[i]; //basic에 position 더한 값
		FVector Position = JointsVector[i];
		GetMesh()->SetBoneLocationByName(FName(JointsNames2[i]), Position, EBoneSpaces::ComponentSpace);
	}
}

void AMotionMatchingCharacter::SetCharacterRotationRest() {

	float pi = 3.141592;

	int scale = (180/pi);
	//int scale = 1;

	//관절의 개수
	int JointsNum = JointsNames2.Num();

	array1d<quat> curr_bone_rotations = character_data.bone_rest_rotations;

	TArray<FQuat> JointsQuat;

	for (int i = 0; i < JointsNum; i++) {

		//언리얼에 맞게 좌표계를 변환해줌
		JointsQuat.Emplace(FQuat(curr_bone_rotations.data[i].y * (1), curr_bone_rotations.data[i].x * (1), curr_bone_rotations.data[i].z * (-1), curr_bone_rotations.data[i].w));

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

	//Mesh를 world y축을 기준으로 90도 회전시켜서 세움
	GetMesh()->SetBoneRotationByName(FName(JointsNames2[0]), GetMesh()->GetBoneRotationByName(FName(JointsNames2[0]), EBoneSpaces::WorldSpace) + StandQuat.Rotator(), EBoneSpaces::WorldSpace);

}