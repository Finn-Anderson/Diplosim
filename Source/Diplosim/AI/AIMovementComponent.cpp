#include "AI/AIMovementComponent.h"

#include "AI.h"
#include "DiplosimAIController.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"

UAIMovementComponent::UAIMovementComponent()
{
	MaxSpeed = 200.0f;
	InitialSpeed = 200.0f;
	SpeedMultiplier = 1.0f;

	CurrentAnim = nullptr;
}

void UAIMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Velocity == FVector::Zero() && CurrentAnim != nullptr) {
		Cast<AAI>(GetOwner())->Mesh->Play(false);

		CurrentAnim = nullptr;
	}
	else if (CurrentAnim == nullptr) {
		Cast<AAI>(GetOwner())->Mesh->PlayAnimation(MoveAnim, true);

		CurrentAnim = MoveAnim;
	}
}

FVector UAIMovementComponent::GetGravitationalForce(const FVector& MoveVelocity)
{
	return MoveVelocity + FVector(0.0f, 0.0f, -100.0f);
}

void UAIMovementComponent::RequestPathMove(const FVector& MoveVelocity)
{
	FVector velocity = GetGravitationalForce(MoveVelocity);

	Super::RequestPathMove(velocity);
}

void UAIMovementComponent::RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed)
{
	FVector velocity = GetGravitationalForce(MoveVelocity);

	Super::RequestDirectMove(velocity, bForceMaxSpeed);
}

void UAIMovementComponent::SetMaxSpeed(int32 Energy)
{
	MaxSpeed = FMath::Clamp(FMath::LogX(InitialSpeed, InitialSpeed * (Energy / 100.0f)) * InitialSpeed, InitialSpeed * 0.3f, InitialSpeed) * SpeedMultiplier;
}

void UAIMovementComponent::SetMultiplier(float Multiplier)
{
	SpeedMultiplier = Multiplier;

	MaxSpeed *= Multiplier;
}