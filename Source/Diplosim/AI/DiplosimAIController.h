#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "DiplosimAIController.generated.h"

USTRUCT()
struct FMoveStruct
{
	GENERATED_USTRUCT_BODY()

	AActor* Actor;

	int32 Instance;

	FVector Location;

	FMoveStruct()
	{
		Actor = nullptr;
		Instance = -1;
		Location = FVector::Zero();
	}

	void ResetLocation()
	{
		Location = FVector::Zero();
	}

	void SetGoalActor(AActor* actor)
	{
		Actor = actor;
	}

	void SetGoalInstance(int32 instance)
	{
		Instance = instance;
	}

	void SetLocation(FVector location)
	{
		Location = location;
	}

	AActor* GetGoalActor()
	{
		return Actor;
	}

	int32 GetGoalInstance()
	{
		return Instance;
	}

	FVector GetLocation()
	{
		return Location;
	}
};

UCLASS()
class DIPLOSIM_API ADiplosimAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	ADiplosimAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void Idle();

	double GetClosestActor(FVector TargetLocation, FVector CurrentLocation, FVector NewLocation, int32 CurrentValue = 1, int32 NewValue = 1);

	bool CanMoveTo(FVector Location);

	virtual void AIMoveTo(AActor* Actor, int32 Instance = -1);

	FMoveStruct MoveRequest;

	FTimerHandle IdleTimer;
};
