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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Speed")
		float MaxSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Speed")
		float InitialSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Speed")
		float SpeedMultiplier;

	TMap<AActor*, TArray<FVector>> avoidPoints;

	UPROPERTY()
		TArray<FVector> Points;

	UPROPERTY()
		UAnimSequence* CurrentAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		UAnimSequence* MoveAnim;
};
