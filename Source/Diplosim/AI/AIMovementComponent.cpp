#include "AI/AIMovementComponent.h"

#include "NavigationSystem.h"

#include "AI.h"
#include "Citizen.h"
#include "AttackComponent.h"
#include "DiplosimAIController.h"
#include "Player/Camera.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Universal/HealthComponent.h"

UAIMovementComponent::UAIMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	InitialSpeed = 200.0f;
	MaxSpeed = InitialSpeed;
	SpeedMultiplier = 1.0f;

	LastUpdatedTime = 0.0f;
	Transform = FTransform(FQuat::Identity, FVector(0.0f));

	ActorToLookAt = nullptr;
	bSetPoints = false;
}

void UAIMovementComponent::ComputeMovement(float DeltaTime)
{
	LastUpdatedTime = GetWorld()->GetTimeSeconds();

	if (!IsValid(AI) || DeltaTime < 0.001f || DeltaTime > 1.0f)
		return;

	AActor* goal = AI->AIController->MoveRequest.GetGoalActor();

	if (CurrentAnim.bPlay) {
		CurrentAnim.Alpha = FMath::Clamp(CurrentAnim.Alpha + (DeltaTime * CurrentAnim.Speed), 0.0f, 1.0f);

		FTransform transform;
		transform.SetLocation(FMath::Lerp(CurrentAnim.StartTransform.GetLocation(), CurrentAnim.EndTransform.GetLocation(), CurrentAnim.Alpha));
		transform.SetRotation(FMath::Lerp(CurrentAnim.StartTransform.GetRotation(), CurrentAnim.EndTransform.GetRotation(), CurrentAnim.Alpha));

		AIVisualiser->SetAnimationPoint(AI, transform);

		if (CurrentAnim.Alpha == 1.0f || CurrentAnim.Alpha == 0.0f) {
			CurrentAnim.Speed *= -1.0f;

			CurrentAnim.StartTransform = FTransform();

			if (CurrentAnim.StartTransform.GetLocation() == CurrentAnim.EndTransform.GetLocation() || (CurrentAnim.Alpha == 0.0f && !CurrentAnim.bRepeat))
				CurrentAnim.bPlay = false;

			if (CurrentAnim.Alpha == 1.0f && (CurrentAnim.Type == EAnim::Melee || CurrentAnim.Type == EAnim::Throw)) {
				if (CurrentAnim.Type == EAnim::Melee) {
					if (goal->IsA<AResource>())
						Cast<ACitizen>(AI)->SetHarvestVisuals(Cast<AResource>(goal));
					else
						AI->AttackComponent->Melee();
				}
				else
					AI->AttackComponent->Throw();
			}
		}

		if (IsValid(ActorToLookAt)) {
			FRotator rotation = (AI->Camera->GetTargetActorLocation(ActorToLookAt) - Transform.GetLocation()).Rotation() * CurrentAnim.Alpha;
			rotation.Pitch = 0.0f;
			rotation.Roll = 0.0f;

			Transform.SetRotation((Transform.GetRotation() + rotation.Quaternion()).GetNormalized());

			if (CurrentAnim.Alpha == 1.0f)
				ActorToLookAt = nullptr;
		}
	}

	if (bSetPoints) {
		Points = TempPoints;

		TempPoints.Empty();
		bSetPoints = false;
	}

	if (Points.IsEmpty() || AI->HealthComponent->GetHealth() == 0)
		return;

	float range = FMath::Min(150.0f * DeltaTime, AI->Range / 15.0f);
	
	if (IsValid(goal)) {
		FVector location = AI->Camera->GetTargetLocation(goal);

		if (Points.Last() != location && (!goal->IsA<AAI>() || !AI->AttackComponent->OverlappingEnemies.IsEmpty()))
			AI->AIController->RecalculateMovement(goal);
	}

	if (!Points.IsEmpty() && FVector::DistXY(Transform.GetLocation(), Points[0]) < range)
		Points.RemoveAt(0);

	if (Points.IsEmpty())
		Velocity = FVector::Zero();
	else
		Velocity = CalculateVelocity(Points[0]);

	FVector deltaV = Velocity * DeltaTime;

	if (!deltaV.IsNearlyZero(1e-6f))
	{
		FHitResult hit;

		FVector location = Transform.GetLocation() + deltaV;

		if (GetWorld()->LineTraceSingleByChannel(hit, location + FVector(0.0f, 0.0f, 20.0f), location - FVector(0.0f, 0.0f, 20.0f), ECollisionChannel::ECC_GameTraceChannel1))
			deltaV.Z = hit.Location.Z - Transform.GetLocation().Z;

		FRotator targetRotation = deltaV.Rotation();
		targetRotation.Pitch = 0.0f;

		Transform.SetLocation(Transform.GetLocation() + deltaV);

		if (Transform.GetRotation().Rotator() != targetRotation)
			Transform.SetRotation(FMath::RInterpTo(Transform.GetRotation().Rotator(), targetRotation, DeltaTime, 10.0f).Quaternion());
	}
	else if (CurrentAnim.Type == EAnim::Move)
		AI->AIController->StopMovement();
}

FVector UAIMovementComponent::CalculateVelocity(FVector Vector)
{
	return (Vector - Transform.GetLocation()).Rotation().Vector() * MaxSpeed;
}

void UAIMovementComponent::SetPoints(TArray<FVector> VectorPoints)
{
	if (!VectorPoints.IsEmpty())
		AI->AIController->StartMovement();
	else if (CurrentAnim.Type == EAnim::Move)
		SetAnimation(EAnim::Still);

	TempPoints = VectorPoints;
	bSetPoints = true;
}

void UAIMovementComponent::SetMaxSpeed(int32 Energy)
{
	MaxSpeed = FMath::Clamp(FMath::LogX(InitialSpeed, InitialSpeed * (Energy / 100.0f)) * InitialSpeed, InitialSpeed * 0.3f, InitialSpeed) * SpeedMultiplier;
}

float UAIMovementComponent::GetMaximumSpeed()
{
	return MaxSpeed;
}

void UAIMovementComponent::SetAnimation(EAnim Type, bool bRepeat, float Speed)
{
	FAnimStruct animStruct;
	animStruct.Type = Type;

	int32 index = AIVisualiser->Animations.Find(animStruct);

	CurrentAnim = AIVisualiser->Animations[index];
	CurrentAnim.StartTransform = AIVisualiser->GetAnimationPoint(AI);
	CurrentAnim.bRepeat = bRepeat;
	CurrentAnim.Speed = Speed;
	CurrentAnim.bPlay = true;
}

bool UAIMovementComponent::IsAttacking()
{
	if (CurrentAnim.bPlay && (CurrentAnim.Type == EAnim::Melee || CurrentAnim.Type == EAnim::Throw))
		return true;

	return false;
}
