#pragma once

#include "CoreMinimal.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "AIMovementComponent.generated.h"

UCLASS()
class DIPLOSIM_API UAIMovementComponent : public UFloatingPawnMovement
{
	GENERATED_BODY()
	
public:
	UAIMovementComponent();

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FVector GetGravitationalForce(const FVector& MoveVelocity);

	virtual void RequestPathMove(const FVector& MoveVelocity) override;

	virtual void RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed) override;

	void SetMaxSpeed(int32 Energy);

	void SetMultiplier(float Multiplier);

	UPROPERTY()
		float InitialSpeed;

	UPROPERTY()
		float SpeedMultiplier;

	UPROPERTY()
		UAnimSequence* CurrentAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		UAnimSequence* MoveAnim;
};
