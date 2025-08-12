#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "AIMovementComponent.generated.h"

UCLASS()
class DIPLOSIM_API UAIMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()
	
public:
	UAIMovementComponent();

protected:
	virtual void BeginPlay() override;

public:
	void ComputeMovement(float DeltaTime);

	FVector CalculateVelocity(FVector Vector);

	void SetPoints(TArray<FVector> VectorPoints);

	void SetMaxSpeed(int32 Energy);

	UFUNCTION(BlueprintCallable)
		float GetMaximumSpeed();

	void SetAnimation(EAnim Type, bool bRepeat = false, float Speed = 2.0f);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Speed")
		float MaxSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Speed")
		float InitialSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Speed")
		float SpeedMultiplier;

	UPROPERTY()
		TArray<FVector> Points;

	UPROPERTY()
		FAnimStruct CurrentAnim;

	UPROPERTY()
		class AAI* AI;

	UPROPERTY()
		class UAIVisualiser* AIVisualiser;

	UPROPERTY()
		double LastUpdatedTime;

	UPROPERTY()
		FTransform Transform;
};
