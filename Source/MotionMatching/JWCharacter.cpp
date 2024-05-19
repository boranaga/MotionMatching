// Fill out your copyright notice in the Description page of Project Settings.


#include "JWCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "MMPlayerController.h"
#include "Math/Color.h"
#include "DrawDebugHelpers.h"



void AJWCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AJWCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
    draw_features(Features_curr, Bone_positions(0), Bone_rotations(0));
}

FVector AJWCharacter::to_Vector3(vec3 v)
{
	return FVector(v.z, -v.x, v.y);
}


void AJWCharacter::draw_features(const slice1d<float> features, const vec3 pos, const quat rot)
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

    //DrawSphereWires(to_Vector3(lfoot_pos), 0.05f, 4, 10, color);
    //DrawSphereWires(to_Vector3(rfoot_pos), 0.05f, 4, 10, color);
    //DrawSphereWires(to_Vector3(traj0_pos), 0.05f, 4, 10, color);
    //DrawSphereWires(to_Vector3(traj1_pos), 0.05f, 4, 10, color);
    //DrawSphereWires(to_Vector3(traj2_pos), 0.05f, 4, 10, color);

    DrawDebugSphere(GetWorld(), to_Vector3(lfoot_pos), 5.0f, 8, FColor::Red);
    DrawDebugSphere(GetWorld(), to_Vector3(rfoot_pos), 5.0f, 8, FColor::Red);
    DrawDebugSphere(GetWorld(), to_Vector3(traj0_pos), 5.0f, 8, FColor::Red);
    DrawDebugSphere(GetWorld(), to_Vector3(traj1_pos), 5.0f, 8, FColor::Red);
    DrawDebugSphere(GetWorld(), to_Vector3(traj2_pos), 5.0f, 8, FColor::Red);


    //DrawLine3D(to_Vector3(lfoot_pos), to_Vector3(lfoot_pos + 0.1f * lfoot_vel), color);
    //DrawLine3D(to_Vector3(rfoot_pos), to_Vector3(rfoot_pos + 0.1f * rfoot_vel), color);

    //DrawLine3D(to_Vector3(traj0_pos), to_Vector3(traj0_pos + 0.25f * traj0_dir), color);
    //DrawLine3D(to_Vector3(traj1_pos), to_Vector3(traj1_pos + 0.25f * traj1_dir), color);
    //DrawLine3D(to_Vector3(traj2_pos), to_Vector3(traj2_pos + 0.25f * traj2_dir), color);

    DrawDebugLine(GetWorld(), to_Vector3(lfoot_pos), to_Vector3(lfoot_pos + 0.1f * lfoot_vel), FColor::Red);
    DrawDebugLine(GetWorld(), to_Vector3(rfoot_pos), to_Vector3(rfoot_pos + 0.1f * rfoot_vel), FColor::Red);
    DrawDebugLine(GetWorld(), to_Vector3(traj0_pos), to_Vector3(traj0_pos + 0.1f * traj0_dir), FColor::Red);
    DrawDebugLine(GetWorld(), to_Vector3(traj1_pos), to_Vector3(traj1_pos + 0.1f * traj1_dir), FColor::Red);
    DrawDebugLine(GetWorld(), to_Vector3(traj2_pos), to_Vector3(traj2_pos + 0.1f * traj2_dir), FColor::Red);
}

//void AJWCharacter::draw_trajectory(const slice1d<vec3> trajectory_positions, const slice1d<quat> trajectory_rotations)
//{
//    for (int i = 1; i < trajectory_positions.size; i++)
//    {
//        DrawSphereWires(to_Vector3(trajectory_positions(i)), 0.05f, 4, 10, color);
//        DrawLine3D(to_Vector3(trajectory_positions(i)), to_Vector3(
//            trajectory_positions(i) + 0.6f * quat_mul_vec3(trajectory_rotations(i), vec3(0, 0, 1.0f))), color);
//        DrawLine3D(to_Vector3(trajectory_positions(i - 1)), to_Vector3(trajectory_positions(i)), color);
//    }
//}

void AJWCharacter::draw_axis(const vec3 pos, const quat rot, const float scale)
{

	vec3 axis0 = pos + quat_mul_vec3(rot, scale * vec3(1.0f, 0.0f, 0.0f));
	vec3 axis1 = pos + quat_mul_vec3(rot, scale * vec3(0.0f, 1.0f, 0.0f));
	vec3 axis2 = pos + quat_mul_vec3(rot, scale * vec3(0.0f, 0.0f, 1.0f));

	/*DrawLine3D(to_Vector3(pos), to_Vector3(axis0), RED);
	DrawLine3D(to_Vector3(pos), to_Vector3(axis1), GREEN);
	DrawLine3D(to_Vector3(pos), to_Vector3(axis2), BLUE);*/

	DrawDebugLine(GetWorld(), to_Vector3(pos), to_Vector3(axis0), FColor::Red, true, -1, 100.0f);
	DrawDebugLine(GetWorld(), to_Vector3(pos), to_Vector3(axis1), FColor::Green, true, -1, 100.0f);
	DrawDebugLine(GetWorld(), to_Vector3(pos), to_Vector3(axis2), FColor::Blue, true, -1, 100.0f);

}









//void AJWCharacter::TapKeyDown()
//{
//	AMMPlayerController* PlayerController = Cast<AMMPlayerController>(Controller);
//	IsTabButtonDown = !IsTabButtonDown;
//
//	if(PlayerController)
//	{
//		PlayerController->ViewMenu(IsTabButtonDown);
//	}
//}



//////////////////////////////// 작업 완료 ///////////////////////////////////////////////
//------------------------------------------------------------------------------------//



//void AJWCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
//{
//	Super::SetupPlayerInputComponent(PlayerInputComponent);
//
//	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
//
//		// Strafe
//		// 주의 : 메인 캐릭터로 복붙할때 &AJWCharacter:: 이거 수정!
//		EnhancedInputComponent->BindAction(StrafeAction, ETriggerEvent::Started, this, &AJWCharacter::OnStrafe);
//		EnhancedInputComponent->BindAction(StrafeAction, ETriggerEvent::Completed, this, &AJWCharacter::OffStrafe);
//
//	}
//	else
//	{
//		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
//	}
//}
//void AJWCharacter::OnStrafe(const FInputActionValue& Value)
//{
//	Desired_strafe = true;
//}
//
//void AJWCharacter::OffStrafe(const FInputActionValue& Value)
//{
//	Desired_strafe = false;
//}

