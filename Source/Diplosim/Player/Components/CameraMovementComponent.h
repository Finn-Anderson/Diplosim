#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CameraMovementComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UCameraMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCameraMovementComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction);

	UPROPERTY()
		class ACamera* Camera;

	UPROPERTY()
		class APlayerController* PController;

	UPROPERTY()
		float CameraSpeed;

	UPROPERTY()
		float Sensitivity;

	// Map Bounds
	void SetBounds(FVector start, FVector end);

	UPROPERTY()
		float MaxXBounds;

	UPROPERTY()
		float MinXBounds;

	UPROPERTY()
		float MaxYBounds;

	UPROPERTY()
		float MinYBounds;

	// Movement Functions
	void Look(const struct FInputActionInstance& Instance);

	void Move(const struct FInputActionInstance& Instance);

	void Speed(const struct FInputActionInstance& Instance);

	void Scroll(const struct FInputActionInstance& Instance);

	UPROPERTY()
		float TargetLength;

	UPROPERTY()
		FVector MovementLocation;

	// Build Camera Shake
	UPROPERTY()
		float Runtime;

	UPROPERTY()
		bool bShake;
};
