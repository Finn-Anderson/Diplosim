#include "CameraMovementComponent.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"

#include "Player/Camera.h"

UCameraMovementComponent::UCameraMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	TargetLength = 1000.0f;

	CameraSpeed = 10.0f;

	Sensitivity = 2.0f;

	Timer = 0.0f;
}

void UCameraMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	Camera = Cast<ACamera>(GetOwner());

	PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	PController->SetControlRotation(FRotator(-45.0f, -90.0f, 0.0f));
	PController->SetInputMode(FInputModeGameAndUI());
	PController->bShowMouseCursor = true;
}

void UCameraMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Camera->SpringArmComponent->TargetArmLength = FMath::FInterpTo(Camera->SpringArmComponent->TargetArmLength, TargetLength, GetWorld()->DeltaTimeSeconds, 10.0f);

	Timer += DeltaTime;

	if (Timer > 0.2f) {
		Timer = 0.0f;

		Async(EAsyncExecution::Thread, [this]() { Camera->SetAmbienceSound(); });
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
	FVector2D value = Instance.GetValue().Get<FVector2D>() * (GetWorld()->DeltaTimeSeconds * 200);

	const FRotator rotation = Camera->Controller->GetControlRotation();
	const FRotator yawRotation(0, rotation.Yaw, 0);

	FVector directionX = FRotationMatrix(yawRotation).GetScaledAxis(EAxis::X);
	FVector directionY = FRotationMatrix(rotation).GetScaledAxis(EAxis::Y);

	FVector loc = Camera->GetActorLocation();

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

	Camera->SetActorLocation(loc);
}

void UCameraMovementComponent::Speed(const struct FInputActionInstance& Instance)
{
	bool value = Instance.GetValue().Get<bool>();

	if (value) {
		CameraSpeed *= 2;
	}
	else {
		CameraSpeed /= 2;
	}
}

void UCameraMovementComponent::Scroll(const struct FInputActionInstance& Instance)
{
	float target = 250.0f * Instance.GetValue().Get<float>();

	TargetLength = FMath::Clamp(TargetLength + target, 50.0f, 6000.0f);
}