#include "AI/AIMovementComponent.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"

#include "AI.h"
#include "Citizen.h"
#include "Enemy.h"
#include "AttackComponent.h"
#include "DiplosimAIController.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Player/Camera.h"
#include "Map/Grid.h"
#include "Universal/Resource.h"
#include "Buildings/Building.h"
#include "Buildings/Misc/Road.h"
#include "Buildings/Misc/Festival.h"

#include "Buildings/Work/Work.h"

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

	if (DeltaTime > 1.0f || DeltaTime < 0.0001f)
		return;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	float range = FMath::Min(150.0f * DeltaTime, Cast<AAI>(GetOwner())->Range / 15.0f);
	
	ADiplosimAIController* aicontroller = Cast<AAI>(GetOwner())->AIController;
	AActor* goal = aicontroller->MoveRequest.GetGoalActor();

	if (IsValid(goal)) {
		if (Cast<AAI>(GetOwner())->CanReach(goal, range)) {
			FRotator rotation = (GetOwner()->GetActorLocation() - goal->GetActorLocation()).Rotation();
			rotation.Pitch = 0.0f;

			GetOwner()->SetActorRotation(FMath::RInterpTo(GetOwner()->GetActorRotation(), rotation, DeltaTime, 100.0f));

			return;
		}
		else if (Velocity.IsNearlyZero(1e-6f) || goal->IsA<AAI>()) {
			aicontroller->RecalculateMovement(goal);
		}
	}

	if (!Points.IsEmpty() && FVector::DistXY(GetOwner()->GetActorLocation(), Points[0]) < range) {
		for (auto& element : avoidPoints) {
			if (!element.Value.Contains(Points[0]))
				continue;

			element.Value.RemoveAt(0);

			if (element.Value.IsEmpty())
				avoidPoints.Remove(element.Key);

			break;
		}

		Points.RemoveAt(0);
	}

	if (Points.IsEmpty())
		Velocity = FVector::Zero();
	else
		Velocity = CalculateVelocity(Points[0]);

	UpdateComponentVelocity();

	FVector deltaV = Velocity * DeltaTime;

	if (!deltaV.IsNearlyZero(1e-6f))
	{
		FHitResult hit;

		FCollisionQueryParams queryParams;
		queryParams.AddIgnoredActor(GetOwner());

		int32 z = 0;

		if (GetWorld()->LineTraceSingleByChannel(hit, GetOwner()->GetActorLocation(), GetOwner()->GetActorLocation() - FVector(0.0f, 0.0f, 200.0f), ECollisionChannel::ECC_GameTraceChannel1, queryParams))
			z = hit.Location.Z;

		FVector location = GetOwner()->GetActorLocation() + deltaV;
		location.Z = z + Cast<AAI>(GetOwner())->Capsule->GetScaledCapsuleHalfHeight();

		GetOwner()->SetActorLocation(location);

		FRotator targetRotation = deltaV.Rotation();
		targetRotation.Pitch = 0.0f;

		if (GetOwner()->GetActorRotation() != targetRotation)
			GetOwner()->SetActorRotation(FMath::RInterpTo(GetOwner()->GetActorRotation(), targetRotation, DeltaTime, 10.0f));

		if (IsValid(CurrentAnim))
			return;

		Cast<AAI>(GetOwner())->Mesh->PlayAnimation(MoveAnim, true);

		CurrentAnim = MoveAnim;
	}
	else if (CurrentAnim == MoveAnim) {
		Cast<AAI>(GetOwner())->Mesh->Play(false);

		CurrentAnim = nullptr;
	}
}

FVector UAIMovementComponent::CalculateVelocity(FVector Vector)
{
	return (Vector - GetOwner()->GetActorLocation()).Rotation().Vector() * MaxSpeed;
}

void UAIMovementComponent::SetPoints(TArray<FVector> VectorPoints)
{
	Points = VectorPoints;

	if (!Points.IsEmpty())
		Cast<AAI>(GetOwner())->AIController->StartMovement();
	else if (CurrentAnim == MoveAnim) {
		Cast<AAI>(GetOwner())->Mesh->Play(false);

		CurrentAnim = nullptr;
	}
}

void UAIMovementComponent::SetMaxSpeed(int32 Energy)
{
	MaxSpeed = FMath::Clamp(FMath::LogX(InitialSpeed, InitialSpeed * (Energy / 100.0f)) * InitialSpeed, InitialSpeed * 0.3f, InitialSpeed) * SpeedMultiplier;
}

float UAIMovementComponent::GetMaximumSpeed()
{
	return MaxSpeed;
}