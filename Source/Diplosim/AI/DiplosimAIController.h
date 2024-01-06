#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "DiplosimAIController.generated.h"

USTRUCT()
struct FMoveStruct
{
	GENERATED_USTRUCT_BODY()

	AActor* Actor;

	FVector Location;

	FMoveStruct()
	{
		Actor = nullptr;
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

	void SetLocation(FVector location)
	{
		Location = location;
	}

	AActor* GetGoalActor()
	{
		return Actor;
	}

	FVector GetLocation()
	{
		return Location;
	}
};

USTRUCT()
struct FClosestStruct
{
	GENERATED_USTRUCT_BODY()

	AActor* Actor;

	double Magnitude;

	FClosestStruct()
	{
		Actor = nullptr;
		Magnitude = 1.0f;
	}
};

UCLASS()
class DIPLOSIM_API ADiplosimAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	ADiplosimAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	FClosestStruct GetClosestActor(AActor* CurrentActor, AActor* NewActor, int32 CurrentValue = 1, int32 NewValue = 1);

	bool CanMoveTo(AActor* Actor);

	virtual void AIMoveTo(AActor* Actor);

	FMoveStruct MoveRequest;
};
