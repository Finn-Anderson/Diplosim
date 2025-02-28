#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "AIMovementComponent.generated.h"

UCLASS()
class DIPLOSIM_API UAIMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()
	
public:
	UAIMovementComponent();

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FVector CalculateVelocity(FVector Vector);

	void SetPoints(TArray<FVector> VectorPoints);

	void SetMaxSpeed(int32 Energy);

	UFUNCTION(BlueprintCallable)
		float GetMaximumSpeed();

	void SetMultiplier(float Multiplier);

	UPROPERTY()
		float MaxSpeed;

	UPROPERTY()
		float InitialSpeed;

	UPROPERTY()
		float SpeedMultiplier;

	UPROPERTY()
		TArray<FVector> Points;

	UPROPERTY()
		UAnimSequence* CurrentAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		UAnimSequence* MoveAnim;
};
