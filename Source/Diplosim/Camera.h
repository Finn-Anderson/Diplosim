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

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		class UCameraComponent* CameraComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		class USpringArmComponent* SpringArmComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		class UBuildComponent* BuildComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement")
		class UCameraMovementComponent* MovementComponent;

	class AGrid* Grid;

public:	
	virtual void Tick(float DeltaTime) override;

public:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Commands
	void Action();


	// Map
	void NewMap();

	void GridStatus();


	// Building
	void BuildStatus();

	void Rotate();


	// Camera Movement
	void Turn(float Value);

	void LookUp(float Value);

	void MoveForward(float Value);

	void MoveRight(float Value);

	void SpeedUp();

	void SlowDown();

	void Scroll(float Value);
};
