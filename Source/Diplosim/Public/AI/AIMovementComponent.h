#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "GameFramework/PawnMovementComponent.h"
#include "AIMovementComponent.generated.h"

UCLASS()
class DIPLOSIM_API UAIMovementComponent : public UNavMovementComponent
{
	GENERATED_BODY()
	
public:
	UAIMovementComponent();

	void ComputeMovement(float DeltaTime, TArray<int32>& Instances);

	void SetPoints(TArray<FVector> VectorPoints);

	void SetMaxSpeed(int32 Energy);

	UFUNCTION(BlueprintCallable)
		float GetMaximumSpeed();

	void SetAnimation(EAnim Type, bool bRepeat = false, float Speed = 2.0f);

	bool IsAttacking();

	FTransform GetMovementTransform();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Speed")
		float MaxSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Speed")
		float InitialSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Speed")
		float SpeedMultiplier;

	UPROPERTY()
		float RoadMultiplier;

	UPROPERTY()
		class AAI* AI;

	UPROPERTY()
		class UAIVisualiser* AIVisualiser;

	UPROPERTY()
		FTransform Transform;

	UPROPERTY()
		float ZOffset;

	UPROPERTY()
		class AActor* ActorToLookAt;

	UPROPERTY()
		TArray<FVector> Points;

	UPROPERTY()
		double LastUpdatedTime;

	UPROPERTY()
		FAnimStruct CurrentAnim;

	UPROPERTY()
		TArray<FVector> TempPoints;

	UPROPERTY()
		bool bSetPoints;

private:
	void ComputeCurrentAnimation(AActor* Goal, float DeltaTime, TArray<int32>& Instances);

	FVector CalculateVelocity(FVector Vector, float DeltaTime);

	void CalculateRoadBonus();
};
