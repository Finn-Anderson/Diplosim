#include "CameraMovementComponent.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/SpringArmComponent.h"

#include "Camera.h"

UCameraMovementComponent::UCameraMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	Camera = Cast<ACamera>(GetOwner());

	TargetLength = 1000.0f;

	CameraSpeed = 10.0f;
}

void UCameraMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	PController->SetControlRotation(FRotator(-45.0f, 0.0f, 0.0f));
	PController->SetInputMode(FInputModeGameAndUI());
	PController->bShowMouseCursor = true;
}

void UCameraMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UCameraMovementComponent::SetBounds(FVector start, FVector end) {
	MinXBounds = end.X;
	MinYBounds = end.Y;

	MaxXBounds = start.X;
	MaxYBounds = start.Y;
}

void UCameraMovementComponent::Turn(float Value)
{
	bool rmbHeld = PController->IsInputKeyDown(EKeys::RightMouseButton);
	bool keyQ = PController->IsInputKeyDown(EKeys::Q);
	bool keyE = PController->IsInputKeyDown(EKeys::E);

	if (rmbHeld || keyQ || keyE)
	{
		Camera->AddControllerYawInput(Value / 6);
	}
}

void UCameraMovementComponent::LookUp(float Value)
{
	bool keyHeld = PController->IsInputKeyDown(EKeys::RightMouseButton);

	if (keyHeld)
	{
		Camera->AddControllerPitchInput(Value / 6);
	}
}

void UCameraMovementComponent::MoveForward(float Value)
{
	const FRotator rotation = Camera->Controller->GetControlRotation();
	const FRotator yawRotation(0, rotation.Yaw, 0);

	FVector direction = FRotationMatrix(yawRotation).GetScaledAxis(EAxis::X);

	FVector loc = Camera->GetActorLocation();

	loc += direction * Value * CameraSpeed;

	if (loc.X > MaxXBounds || loc.X < MinXBounds) {
		if (loc.X > MaxXBounds) {
			loc.X = MaxXBounds;
		}
		else {
			loc.X = MinXBounds;
		}
	}
	else if (loc.Y > MaxYBounds || loc.Y < MinYBounds) {
		if (loc.Y > MaxYBounds) {
			loc.Y = MaxYBounds;
		}
		else {
			loc.Y = MinYBounds;
		}
	}

	Camera->SetActorLocation(loc);
}

void UCameraMovementComponent::MoveRight(float Value)
{
	FVector direction = FRotationMatrix(Camera->Controller->GetControlRotation()).GetScaledAxis(EAxis::Y);

	FVector loc = Camera->GetActorLocation();

	loc += direction * Value * CameraSpeed;

	if (loc.X > MaxXBounds || loc.X < MinXBounds) {
		if (loc.X > MaxXBounds) {
			loc.X = MaxXBounds;
		}
		else {
			loc.X = MinXBounds;
		}
	}
	else if (loc.Y > MaxYBounds || loc.Y < MinYBounds) {
		if (loc.Y > MaxYBounds) {
			loc.Y = MaxYBounds;
		}
		else {
			loc.Y = MinYBounds;
		}
	}

	Camera->SetActorLocation(loc);
}

void UCameraMovementComponent::SpeedUp()
{
	CameraSpeed *= 2;
}

void UCameraMovementComponent::SlowDown()
{
	CameraSpeed /= 2;
}

void UCameraMovementComponent::Scroll(float Value)
{
	float target = 250.0f * Value;

	if (target != 0.0f) {
		TargetLength = FMath::Clamp(TargetLength + target, 0.0f, 2000.0f);
	}

	Camera->SpringArmComponent->TargetArmLength = FMath::FInterpTo(Camera->SpringArmComponent->TargetArmLength, TargetLength, GetWorld()->DeltaTimeSeconds, 10.0f);
}