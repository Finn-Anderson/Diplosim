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
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetComponentTickInterval(0.01f);
	
	MaxSpeed = 200.0f;
	InitialSpeed = 200.0f;
	SpeedMultiplier = 1.0f;

	CurrentAnim = nullptr;
}

void UAIMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	AI = Cast<AAI>(GetOwner());
}

void UAIMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (DeltaTime < 0.009f || DeltaTime > 1.0f)
		return;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	float range = FMath::Min(150.0f * DeltaTime, AI->Range / 15.0f);
	
	ADiplosimAIController* aicontroller = AI->AIController;
	AActor* goal = aicontroller->MoveRequest.GetGoalActor();

	if (IsValid(goal)) {
		if (AI->CanReach(goal, range)) {
			FRotator rotation = (goal->GetActorLocation() - AI->GetActorLocation()).Rotation();
			rotation.Pitch = 0.0f;

			AI->SetActorRotation(FMath::RInterpTo(AI->GetActorRotation(), rotation, DeltaTime, 10.0f));
		}
		else if (Velocity.IsNearlyZero(1e-6f) || goal->IsA<AAI>()) {
			aicontroller->RecalculateMovement(goal);
		}
	}

	if (!Points.IsEmpty() && FVector::DistXY(AI->GetActorLocation(), Points[0]) < range) {
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
		queryParams.AddIgnoredActor(AI);

		int32 z = 0;

		if (GetWorld()->LineTraceSingleByChannel(hit, AI->GetActorLocation(), AI->GetActorLocation() - FVector(0.0f, 0.0f, 200.0f), ECollisionChannel::ECC_GameTraceChannel1, queryParams))
			z = hit.Location.Z;

		FVector location = AI->GetActorLocation() + deltaV;
		location.Z = z + AI->Capsule->GetScaledCapsuleHalfHeight();

		AI->SetActorLocation(location);

		FRotator targetRotation = deltaV.Rotation();
		targetRotation.Pitch = 0.0f;

		if (AI->GetActorRotation() != targetRotation)
			AI->SetActorRotation(FMath::RInterpTo(AI->GetActorRotation(), targetRotation, DeltaTime, 10.0f));

		if (IsValid(CurrentAnim))
			return;

		AI->Mesh->PlayAnimation(MoveAnim, true);

		CurrentAnim = MoveAnim;
	}
	else if (CurrentAnim == MoveAnim) {
		AI->Mesh->Play(false);

		CurrentAnim = nullptr;

		AI->AIController->StopMovement();
	}
}

FVector UAIMovementComponent::CalculateVelocity(FVector Vector)
{
	return (Vector - AI->GetActorLocation()).Rotation().Vector() * MaxSpeed;
}

void UAIMovementComponent::SetPoints(TArray<FVector> VectorPoints)
{
	Points = VectorPoints;

	if (!Points.IsEmpty()) {
		AI->AIController->StartMovement();
	}
	else if (CurrentAnim == MoveAnim) {
		AI->Mesh->Play(false);

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