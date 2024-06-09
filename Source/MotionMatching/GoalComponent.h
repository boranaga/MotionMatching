// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "GoalComponent.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MOTIONMATCHING_API UGoalComponent : public UBoxComponent
{
	GENERATED_BODY()

public:
	UGoalComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY(EditAnywhere, Category = "TriggerTag")
	FName Tag = TEXT("unlock1");

	
};
