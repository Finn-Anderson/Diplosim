#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "AIController.h"
#include "DiplosimAIController.generated.h"

USTRUCT(BlueprintType)
struct FMoveStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
		AActor* Actor;

	UPROPERTY()
		AActor* LinkedPortal;

	UPROPERTY()
		AActor* UltimateGoal;

	UPROPERTY()
		int32 Instance;

	UPROPERTY()
		FVector Location;

	FMoveStruct()
	{
		Actor = nullptr;
		LinkedPortal = nullptr;
		UltimateGoal = nullptr;
		Instance = -1;
		Location = FVector::Zero();
	}

	void ResetLocation()
	{
		Location = FVector::Zero();
	}

	void SetGoalActor(AActor* actor, AActor* linkedPortal = nullptr, AActor* ultimateGoal = nullptr)
	{
		Actor = actor;
		LinkedPortal = linkedPortal;
		UltimateGoal = ultimateGoal;
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

	AActor* GetUltimateGoalActor()
	{
		return UltimateGoal;
	}

	AActor* GetLinkedPortal()
	{
		return LinkedPortal;
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
	ADiplosimAIController();

	UFUNCTION()
		void DefaultAction();

	void Idle(FFactionStruct* Faction, class ACitizen* Citizen);

	void Wander(FVector CentrePoint, bool bTimer, float MaxLength = 5000.0f, bool bRaid = false);

	UFUNCTION()
		void ChooseIdleBuilding(class ACitizen* Citizen);

	double GetClosestActor(float Range, FVector TargetLocation, FVector CurrentLocation, FVector NewLocation, bool bProjectLocation = true, int32 CurrentValue = 1, int32 NewValue = 1);

	UFUNCTION()
		void GetGatherSite(TSubclassOf<class AResource> Resource);

	bool CanMoveTo(FVector Location, AActor* Target = nullptr, bool bCheckForPortals = true);

	TArray<FVector> GetPathPoints(FVector StartLocation, FVector EndLocation);

	void AIMoveTo(AActor* Actor, FVector Location = FVector::Zero(), int32 Instance = -1);

	void RecalculateMovement(AActor* Actor);

	void StartMovement();

	virtual void StopMovement() override;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
		FMoveStruct MoveRequest;

	UPROPERTY()
		class ABuilding* ChosenBuilding;

	UPROPERTY()
		class ACamera* Camera;

	UPROPERTY()
		class AAI* AI;
};
