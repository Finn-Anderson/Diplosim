#include "CameraMovementComponent.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "Camera/CameraComponent.h"

#include "Player/Camera.h"
#include "Universal/DiplosimUserSettings.h"
#include "AI/AI.h"
#include "Buildings/Building.h"

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

	Settings = UDiplosimUserSettings::GetDiplosimUserSettings();

	PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	PController->SetControlRotation(FRotator(-25.0f, 180.0f, 0.0f));
	PController->SetInputMode(FInputModeGameAndUI());
	PController->bShowMouseCursor = true;

	MovementLocation = Camera->GetActorLocation();
}

void UCameraMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (DeltaTime > 1.0f)
		return;

	float armSpeed = CameraSpeed / 3.0f;
	float movementSpeed = CameraSpeed / 2.0f;

	if (!Settings->GetSmoothCamera()) {
		armSpeed *= 10.0f;
		movementSpeed *= 10.0f;
	}

	SetAttachedMovementLocation(Camera->ActorAttachedTo);

	Camera->SpringArmComponent->TargetArmLength = FMath::FInterpTo(Camera->SpringArmComponent->TargetArmLength, TargetLength, DeltaTime, armSpeed);

	Camera->SetActorLocation(FMath::VInterpTo(Camera->GetActorLocation(), MovementLocation, DeltaTime, movementSpeed));

	if (!bShake)
		return;

	Runtime += DeltaTime * Camera->CustomTimeDilation * CameraSpeed;

	double sin = FMath::Clamp(FMath::Sin(Runtime), 0.0f, 1.0f);

	Camera->CameraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -4.0f * sin));

	if (Camera->CameraComponent->GetRelativeLocation() == FVector::Zero()) {
		Runtime = 0.0f;

		bShake = false;
	}
}

void UCameraMovementComponent::SetAttachedMovementLocation(AActor* Actor)
{
	if (Actor == nullptr)
		return;

	FVector height = FVector::Zero();

	if (Actor->IsA<AAI>())
		height.Z = Cast<AAI>(Actor)->Mesh->GetSkeletalMeshAsset()->GetBounds().GetBox().GetSize().Z / 2;
	else if (Actor->IsA<ABuilding>())
		height.Z = Cast<ABuilding>(Actor)->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize().Z / 2;

	MovementLocation = Actor->GetActorLocation() + FVector(0.0f, 0.0f, 5.0f) + height;
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
	double deltaTime = GetWorld()->DeltaTimeSeconds / UGameplayStatics::GetGlobalTimeDilation(GetWorld()) * 200.0f;
	
	if (deltaTime > 100.0f)
		return;
	
	FVector2D value = Instance.GetValue().Get<FVector2D>();

	const FRotator rotation = Camera->Controller->GetControlRotation();
	const FRotator yawRotation(0, rotation.Yaw, 0);

	FVector directionX = FRotationMatrix(yawRotation).GetScaledAxis(EAxis::X);
	FVector directionY = FRotationMatrix(rotation).GetScaledAxis(EAxis::Y);

	FVector loc = MovementLocation;

	loc += (directionX * value.X * CameraSpeed * deltaTime) + (directionY * value.Y * CameraSpeed * deltaTime);

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
	float target = 500.0f * Instance.GetValue().Get<float>();

	TargetLength = FMath::Clamp(TargetLength + target, 50.0f, 12000.0f);
}