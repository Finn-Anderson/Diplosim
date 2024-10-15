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

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	if (!Points.IsEmpty() && FVector::DistXY(GetOwner()->GetActorLocation(), Points[0]) < 12.0f)
		Points.RemoveAt(0);

	if (Points.IsEmpty())
		Velocity = FVector::Zero();
	else
		Velocity = CalculateVelocity(Points[0]);

	UpdateComponentVelocity();
		
	FVector deltaV = Velocity * DeltaTime;

	if (!deltaV.IsNearlyZero(1e-6f))
	{
		FNavLocation deltaTarget;
		nav->ProjectPointToNavigation(GetOwner()->GetActorLocation() + deltaV, deltaTarget, FVector(200.0f, 200.0f, 20.0f));

		GetOwner()->SetActorLocation(deltaTarget.Location + FVector(0.0f, 0.0f, 6.0f));
	}

	if (!Points.IsEmpty()) {
		FRotator targetRotation = (GetOwner()->GetActorLocation() - Points[0]).Rotation();
		targetRotation.Pitch = 0.0f; 
		
		if (GetOwner()->GetActorRotation() != targetRotation)
			GetOwner()->SetActorRotation(FMath::RInterpTo(GetOwner()->GetActorRotation(), targetRotation, DeltaTime, 10.0f));
	}
	
	if (Velocity == FVector::Zero() && CurrentAnim != nullptr) {
		Cast<AAI>(GetOwner())->Mesh->Play(false);

		CurrentAnim = nullptr;
	}
	else if (CurrentAnim == nullptr) {
		Cast<AAI>(GetOwner())->Mesh->PlayAnimation(MoveAnim, true);

		CurrentAnim = MoveAnim;
	}
}

FVector UAIMovementComponent::CalculateVelocity(FVector Vector)
{
	return (Vector - GetOwner()->GetActorLocation()).Rotation().Vector() * MaxSpeed;
}

void UAIMovementComponent::SetPoints(TArray<FVector> VectorPoints)
{
	Points = VectorPoints;
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