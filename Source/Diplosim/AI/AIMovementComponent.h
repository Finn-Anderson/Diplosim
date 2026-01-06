#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "GameFramework/PawnMovementComponent.h"
#include "AIMovementComponent.generated.h"

UCLASS()
class DIPLOSIM_API UAIMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()
	
public:
	UAIMovementComponent();

	void ComputeMovement(float DeltaTime);

	void SetPoints(TArray<FVector> VectorPoints);

	void SetMaxSpeed(int32 Energy);

	UFUNCTION(BlueprintCallable)
		float GetMaximumSpeed();

	void SetAnimation(EAnim Type, bool bRepeat = false, float Speed = 2.0f);

	bool IsAttacking();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Speed")
		float MaxSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Speed")
		float InitialSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Speed")
		float SpeedMultiplier;

	UPROPERTY()
		class AAI* AI;

	UPROPERTY()
		class UAIVisualiser* AIVisualiser;

	UPROPERTY()
		FTransform Transform;

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
	void ComputeCurrentAnimation(AActor* Goal, float DeltaTime);

	FVector CalculateVelocity(FVector Vector);

	void AvoidCollisions(FVector& DeltaV, float DeltaTime);
};
