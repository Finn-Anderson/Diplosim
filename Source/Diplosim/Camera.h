#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Camera.generated.h"

UCLASS()
class DIPLOSIM_API ACamera : public APawn
{
	GENERATED_BODY()

public:
	ACamera();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		class UCameraComponent* CameraComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		class USpringArmComponent* SpringArmComponent;

	UPROPERTY()
		class APlayerController* PController;

	UPROPERTY()
		float CameraSpeed;

public:	
	virtual void Tick(float DeltaTime) override;


public:
	void SetBounds(FVector start, FVector end);

	float MaxXBounds;

	float MinXBounds;

	float MaxYBounds;

	float MinYBounds;

public:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Camera Movement
	void Turn(float Value);

	void LookUp(float Value);

	void MoveForward(float Value);

	void MoveRight(float Value);

	void SpeedUp();

	void SlowDown();

	void Scroll(float Value);

	float TargetLength;
};
