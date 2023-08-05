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
	class ACamera* Camera;

	class APlayerController* PController;

	float CameraSpeed;

	float Sensitivity;

public:
	void SetBounds(FVector start, FVector end);

	float MaxXBounds;

	float MinXBounds;

	float MaxYBounds;

	float MinYBounds;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void Turn(float Value);

	void LookUp(float Value);

	void MoveForward(float Value);

	void MoveRight(float Value);

	void SpeedUp();

	void SlowDown();

	void Scroll(float Value);

	float TargetLength;
};
