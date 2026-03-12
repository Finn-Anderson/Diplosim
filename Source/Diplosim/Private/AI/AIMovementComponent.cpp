#include "AI/AIMovementComponent.h"

#include "Components/InstancedStaticMeshComponent.h"

#include "AI/AI.h"
#include "AI/DiplosimAIController.h"
#include "AI/Citizen/Citizen.h"
#include "Map/AIVisualiser.h"
#include "Player/Camera.h"
#include "Universal/HealthComponent.h"
#include "Universal/AttackComponent.h"
#include "Universal/DiplosimGameModeBase.h"

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

	if (bSetPoints) {
		Points = TempPoints;

		TempPoints.Empty();
		bSetPoints = false;
	}

	ComputeCurrentAnimation(goal, DeltaTime);

	if (Points.IsEmpty() || AI->HealthComponent->GetHealth() == 0 || CurrentAnim.Type != EAnim::Move)
		return;

	float range = FMath::Min(150.0f * DeltaTime, AI->Range / 15.0f);
	
	AI->AIController->RecalculateMovement(goal);

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

		if (GetWorld()->LineTraceSingleByChannel(hit, location + FVector(0.0f, 0.0f, 30.0f), location - FVector(0.0f, 0.0f, 30.0f), ECollisionChannel::ECC_GameTraceChannel1))
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

void UAIMovementComponent::ComputeCurrentAnimation(AActor* Goal, float DeltaTime)
{
	if (!CurrentAnim.bPlay)
		return;

	CurrentAnim.Alpha = FMath::Clamp(CurrentAnim.Alpha + (DeltaTime * CurrentAnim.Speed), 0.0f, 1.0f);

	if (IsValid(ActorToLookAt) && Points.IsEmpty()) {
		FRotator rotation = (AI->Camera->GetTargetActorLocation(ActorToLookAt) - Transform.GetLocation()).Rotation();
		rotation.Pitch = Transform.GetRotation().Rotator().Pitch;
		rotation.Roll = Transform.GetRotation().Rotator().Roll;

		Transform.SetRotation(FMath::Lerp(Transform.GetRotation(), rotation.Quaternion(), CurrentAnim.Alpha));

		if (CurrentAnim.Alpha == 1.0f)
			ActorToLookAt = nullptr;
	}

	FVector endLocation = CurrentAnim.EndTransform.GetLocation();
	if (CurrentAnim.Type == EAnim::Melee)
		endLocation = Transform.GetRotation().Vector() * endLocation.X;

	FTransform transform;
	transform.SetLocation(FMath::Lerp(CurrentAnim.StartTransform.GetLocation(), endLocation, CurrentAnim.Alpha));
	transform.SetRotation(FMath::Lerp(CurrentAnim.StartTransform.GetRotation(), CurrentAnim.EndTransform.GetRotation(), CurrentAnim.Alpha));

	AIVisualiser->SetAnimationPoint(AI, transform);

	if (CurrentAnim.Alpha == 1.0f || CurrentAnim.Alpha == 0.0f) {
		CurrentAnim.Speed *= -1.0f;

		CurrentAnim.StartTransform = FTransform();

		if (CurrentAnim.StartTransform.GetLocation() == endLocation || (CurrentAnim.Alpha == 0.0f && !CurrentAnim.bRepeat) || CurrentAnim.Type == EAnim::Decay)
			CurrentAnim.bPlay = false;

		if (CurrentAnim.Alpha == 1.0f && (CurrentAnim.Type == EAnim::Melee || CurrentAnim.Type == EAnim::Throw)) {
			if (CurrentAnim.Type == EAnim::Melee) {
				if (IsValid(Goal) && Goal->IsA<AResource>())
					AIVisualiser->SetHarvestVisuals(Cast<ACitizen>(AI), Cast<AResource>(Goal));
				else
					AI->AttackComponent->Melee();
			}
			else
				AI->AttackComponent->Throw();
		}
	}
}

FVector UAIMovementComponent::CalculateVelocity(FVector Vector)
{
	return (Vector - Transform.GetLocation()).Rotation().Vector() * MaxSpeed;
}

void UAIMovementComponent::SetPoints(TArray<FVector> VectorPoints)
{
	TempPoints = VectorPoints;
	bSetPoints = true;

	if (!VectorPoints.IsEmpty())
		AI->AIController->StartMovement();
	else if (CurrentAnim.Type == EAnim::Move)
		AI->AIController->StopMovement();
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

	if (Type == EAnim::Decay)
		CurrentAnim.EndTransform.SetRotation(CurrentAnim.StartTransform.GetRotation());
	else if (Type == EAnim::Move && Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode())->Snakes.Contains(AI))
		CurrentAnim.EndTransform.SetLocation(FVector(CurrentAnim.EndTransform.GetLocation().X, CurrentAnim.EndTransform.GetLocation().Y, 0.0f));
}

bool UAIMovementComponent::IsAttacking()
{
	if (CurrentAnim.bPlay && (CurrentAnim.Type == EAnim::Melee || CurrentAnim.Type == EAnim::Throw))
		return true;

	return false;
}
