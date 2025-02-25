#include "CameraMovementComponent.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "Camera/CameraComponent.h"

#include "Player/Camera.h"

UCameraMovementComponent::UCameraMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	TargetLength = 3000.0f;

	CameraSpeed = 10.0f;

	Sensitivity = 2.0f;

	Runtime = 0.0f;

	bShake = false;

	MovementLocation = FVector::Zero();
}

void UCameraMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	Camera = Cast<ACamera>(GetOwner());

	PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	PController->SetControlRotation(FRotator(-25.0f, 180.0f, 0.0f));
	PController->SetInputMode(FInputModeGameAndUI());
	PController->bShowMouseCursor = true;

	MovementLocation = Camera->GetActorLocation();
}

void UCameraMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Camera->SpringArmComponent->TargetArmLength = FMath::FInterpTo(Camera->SpringArmComponent->TargetArmLength, TargetLength, 0.01f, CameraSpeed / 3.0f);

	Camera->SetActorLocation(FMath::VInterpTo(MovementLocation, Camera->GetActorLocation(), 0.01f, CameraSpeed / 24.0f));

	if (!bShake)
		return;

	Runtime += DeltaTime * CameraSpeed;

	double sin = FMath::Clamp(FMath::Sin(Runtime), 0.0f, 1.0f);

	Camera->CameraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -4.0f * sin));

	if (Camera->CameraComponent->GetRelativeLocation() == FVector::Zero()) {
		Runtime = 0.0f;

		bShake = false;
	}
}

void UCameraMovementComponent::SetBounds(FVector start, FVector end) {
	MinXBounds = end.X;
	MinYBounds = end.Y;

	MaxXBounds = start.X;
	MaxYBounds = start.Y;
}

void UCameraMovementComponent::Look(const struct FInputActionInstance& Instance)
{
	FVector2D value = Instance.GetValue().Get<FVector2D>();

	FRotator rot = PController->GetControlRotation();

	FVector2D change = (value / 3) * Sensitivity;

	PController->SetControlRotation(FRotator(rot.Pitch + change.Y, rot.Yaw + change.X, rot.Roll));
}

void UCameraMovementComponent::Move(const struct FInputActionInstance& Instance)
{
	FVector2D value = Instance.GetValue().Get<FVector2D>();

	const FRotator rotation = Camera->Controller->GetControlRotation();
	const FRotator yawRotation(0, rotation.Yaw, 0);

	FVector directionX = FRotationMatrix(yawRotation).GetScaledAxis(EAxis::X);
	FVector directionY = FRotationMatrix(rotation).GetScaledAxis(EAxis::Y);

	FVector loc = MovementLocation;

	loc += (directionX * value.X * CameraSpeed) + (directionY * value.Y * CameraSpeed);

	if (loc.X > MaxXBounds || loc.X < MinXBounds) {
		if (loc.X > MaxXBounds) {
			loc.X = MaxXBounds;
		}
		else {
			loc.X = MinXBounds;
		}
	}

	if (loc.Y > MaxYBounds || loc.Y < MinYBounds) {
		if (loc.Y > MaxYBounds) {
			loc.Y = MaxYBounds;
		}
		else {
			loc.Y = MinYBounds;
		}
	}

	MovementLocation = loc;
}

void UCameraMovementComponent::Speed(const struct FInputActionInstance& Instance)
{
	if (CameraSpeed == 10.0f)
		CameraSpeed *= 2;
	else
		CameraSpeed /= 2;
}

void UCameraMovementComponent::Scroll(const struct FInputActionInstance& Instance)
{
	float target = 250.0f * Instance.GetValue().Get<float>();

	TargetLength = FMath::Clamp(TargetLength + target, 50.0f, 6000.0f);
}