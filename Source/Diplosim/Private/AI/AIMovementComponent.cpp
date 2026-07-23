#include "AI/AIMovementComponent.h"

#include "Components/InstancedStaticMeshComponent.h"

#include "AI/AI.h"
#include "AI/Clone.h"
#include "AI/DiplosimAIController.h"
#include "AI/Citizen/Citizen.h"
#include "AI/Citizen/Components/BuildingComponent.h"
#include "Buildings/Misc/Festival.h"
#include "Buildings/Misc/Road.h"
#include "Map/AIVisualiser.h"
#include "Player/Camera.h"
#include "Player/Components/BuildComponent.h"
#include "Universal/HealthComponent.h"
#include "Universal/AttackComponent.h"
#include "Universal/DiplosimGameModeBase.h"

UAIMovementComponent::UAIMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	AI = nullptr;
	AIVisualiser = nullptr;

	InitialSpeed = 200.0f;
	MaxSpeed = InitialSpeed;
	SpeedMultiplier = 1.0f;
	RoadMultiplier = 1.0f;

	LastUpdatedTime = 0.0f;
	Transform = FTransform(FQuat::Identity, FVector(0.0f)); 
	ZOffset = 0.0f;

	ActorToLookAt = nullptr;
	bSetPoints = false;
	SetPosition = FVector::Zero();
}

void UAIMovementComponent::ComputeMovement(float DeltaTime, TArray<int32>& Instances)
{
	LastUpdatedTime = GetWorld()->GetTimeSeconds();

	if (!IsValid(AI) || DeltaTime < 0.001f || DeltaTime > 1.0f)
		return;

	AActor* goal = AI->AIController->MoveRequest.GetGoalActor();

	ComputeCurrentAnimation(goal, DeltaTime, Instances);

	if (AI->HealthComponent->GetHealth() == 0 || (AI->IsA<ACitizen>() && Cast<ACitizen>(AI)->bConversing))
		return;

	if (SetPosition == FVector::Zero())
		AI->AIController->RecalculateMovement(goal);

	if (bSetPoints || SetPosition != FVector::Zero()) {
		Points = TempPoints;

		TempPoints.Empty();
		bSetPoints = false;

		if (SetPosition != FVector::Zero()) {
			Transform.SetLocation(SetPosition);

			SetPosition = FVector::Zero();

			if (AI->AIController->MoveRequest.GetUltimateLocation() != FVector::Zero())
				AI->AIController->AIMoveTo(AI->AIController->MoveRequest.GetUltimateGoalActor(), AI->AIController->MoveRequest.GetUltimateLocation());
			else if (Points.IsEmpty())
				AI->AIController->DefaultAction();
		}
	}

	if (CurrentAnim.Type != EAnim::Move || Points.IsEmpty()) {
		Velocity = FVector::Zero();

		return;
	}

	Velocity = CalculateVelocity(Points[0], DeltaTime);

	FVector deltaV = Velocity * DeltaTime;

	if (!deltaV.IsNearlyZero(1e-6f) && !AI->CanReach(AI, 5.0f, Points[0]))
	{
		FCollisionQueryParams params;
		params.AddIgnoredActor(GetOwner());

		FHitResult hit;

		FVector location = Transform.GetLocation() + deltaV;

		ZOffset = 0.0f;
		if (GetWorld()->LineTraceSingleByChannel(hit, location + FVector(0.0f, 0.0f, 100.0f), location - FVector(0.0f, 0.0f, 100.0f), ECollisionChannel::ECC_GameTraceChannel1, params))
			ZOffset = hit.Location.Z - location.Z;

		FRotator targetRotation = deltaV.Rotation();
		targetRotation.Pitch = 0.0f;

		Transform.SetLocation(location);

		if (Transform.GetRotation().Rotator() != targetRotation)
			Transform.SetRotation(FMath::RInterpTo(Transform.GetRotation().Rotator(), targetRotation, DeltaTime, 10.0f).Quaternion());
	}
	else {
		Points.RemoveAt(0);

		if (Points.IsEmpty())
			AI->AIController->StopMovement();
	}
}

void UAIMovementComponent::ComputeCurrentAnimation(AActor* Goal, float DeltaTime, TArray<int32>& Instances)
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

	AIVisualiser->SetAnimationPoint(AI, transform, Instances);

	if (CurrentAnim.Alpha == 1.0f || CurrentAnim.Alpha == 0.0f) {
		CurrentAnim.Speed *= -1.0f;

		CurrentAnim.StartTransform = FTransform();

		if ((CurrentAnim.Alpha == 0.0f && !CurrentAnim.bRepeat) || CurrentAnim.bOneWay)
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

FVector UAIMovementComponent::CalculateVelocity(FVector Vector, float DeltaTime)
{
	CalculateRoadBonus();

	float speed = GetMaximumSpeed();

	if (DeltaTime > 0.05f) {
		float distance = FVector::Dist(Vector, Transform.GetLocation());
		if (speed > distance)
			speed = FMath::Max(distance, 50.0f);
	}

	return (Vector - Transform.GetLocation()).Rotation().Vector() * speed;
}

void UAIMovementComponent::CalculateRoadBonus()
{
	if (!AI->IsA<ACitizen>() && !AI->IsA<AClone>())
		return;

	float cost = 1.0f;

	FHitResult hit;
	if (GetWorld()->LineTraceSingleByChannel(hit, Transform.GetLocation() + FVector(0.0f, 0.0f, 10.0f), Transform.GetLocation() - FVector(0.0f, 0.0f, 10.0f), ECollisionChannel::ECC_GameTraceChannel1))
		if ((hit.GetActor()->IsA<ARoad>() || hit.GetActor()->IsA<AFestival>()) && !AI->Camera->BuildComponent->Buildings.Contains(hit.GetActor()))
			cost = 1.15f * Cast<ABuilding>(hit.GetActor())->GetTier();

	if (RoadMultiplier == cost)
		return;

	RoadMultiplier = cost;
	
	int32 energy = 100;
	if (IsA<ACitizen>())
		energy = Cast<ACitizen>(AI)->Energy;

	SetMaxSpeed(energy);
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
	MaxSpeed = FMath::Clamp(FMath::LogX(InitialSpeed, InitialSpeed * (Energy / 100.0f)) * InitialSpeed, InitialSpeed * 0.3f, InitialSpeed) * SpeedMultiplier * RoadMultiplier;
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

FTransform UAIMovementComponent::GetMovementTransform()
{
	FTransform transform = Transform;
	transform.SetLocation(transform.GetLocation() + FVector(0.0f, 0.0f, ZOffset));

	return transform;
}
