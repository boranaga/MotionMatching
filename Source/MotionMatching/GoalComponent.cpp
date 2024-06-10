// Fill out your copyright notice in the Description page of Project Settings.


#include "GoalComponent.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Components/MeshComponent.h"

UGoalComponent::UGoalComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UGoalComponent::BeginPlay()
{	
	Super::BeginPlay();
}

void UGoalComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//TArray<AActor*> Actors;
	//GetOverlappingActors(Actors);

	//for (AActor* Actor : Actors)
	//{
	//	if (Actor->ActorHasTag(Tag))
	//	{
	//		UE_LOG(LogTemp, Log, TEXT("%s"), *Actor->GetName());

	//	}
	//}

}	
