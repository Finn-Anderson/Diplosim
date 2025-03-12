#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "DiplosimAIController.generated.h"

USTRUCT(BlueprintType)
struct FMoveStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
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

	void Tick(float DeltaTime) override;

	void DefaultAction();

	void Idle(class ACitizen* Citizen);

	double GetClosestActor(float Range, FVector TargetLocation, FVector CurrentLocation, FVector NewLocation, bool bProjectLocation = true, int32 CurrentValue = 1, int32 NewValue = 1);

	void GetGatherSite(class ACamera* Camera, TSubclassOf<class AResource> Resource);

	bool CanMoveTo(FVector Location);

	TArray<FVector> GetPathPoints(FVector StartLocation, FVector EndLocation);

	void AIMoveTo(AActor* Actor, FVector Location = FVector::Zero(), int32 Instance = -1);

	void RecalculateMovement(AActor* Actor);

	virtual void StopMovement() override;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
		FMoveStruct MoveRequest;
};
