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

	class ACamera* Camera;

	class APlayerController* PController;

	float CameraSpeed;

	float Sensitivity;

	float Timer;

	// Map Bounds
	void SetBounds(FVector start, FVector end);

	float MaxXBounds;

	float MinXBounds;

	float MaxYBounds;

	float MinYBounds;

	// Movement Functions
	void Look(const struct FInputActionInstance& Instance);

	void Move(const struct FInputActionInstance& Instance);

	void Speed(const struct FInputActionInstance& Instance);

	void Scroll(const struct FInputActionInstance& Instance);

	float TargetLength;
};
