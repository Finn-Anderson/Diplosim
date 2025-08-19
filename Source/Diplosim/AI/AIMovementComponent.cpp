#include "AI/AIMovementComponent.h"

#include "Components/CapsuleComponent.h"
#include "Animation/AnimSingleNodeInstance.h"

#include "AI.h"
#include "DiplosimAIController.h"
#include "Player/Camera.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"

UAIMovementComponent::UAIMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	MaxSpeed = 200.0f;
	InitialSpeed = 200.0f;
	SpeedMultiplier = 1.0f;

	LastUpdatedTime = 0.0f;
	Transform = FTransform();
}

void UAIMovementComponent::ComputeMovement(float DeltaTime)
{
	LastUpdatedTime = GetWorld()->GetTimeSeconds();

	if (!IsValid(AI) || DeltaTime < 0.001f || DeltaTime > 1.0f)
		return;

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
		}
	}

	if (Points.IsEmpty())
		return;

	float range = FMath::Min(150.0f * DeltaTime, AI->Range / 15.0f);
	
	ADiplosimAIController* aicontroller = AI->AIController;
	AActor* goal = aicontroller->MoveRequest.GetGoalActor();
	
	if (IsValid(goal)) {
		FVector location = goal->GetActorLocation();
		if (goal->IsA<AAI>())
			location = Cast<AAI>(goal)->MovementComponent->Transform.GetLocation();

		if (Points.Last() != location)
			aicontroller->RecalculateMovement(goal);
	}

	if (!Points.IsEmpty() && FVector::DistXY(Transform.GetLocation(), Points[0]) < range)
		Points.RemoveAt(0);

	if (Points.IsEmpty())
		Velocity = FVector::Zero();
	else
		Velocity = CalculateVelocity(Points[0]);

	UpdateComponentVelocity();

	FVector deltaV = Velocity * DeltaTime;

	if (!deltaV.IsNearlyZero(1e-6f))
	{
		FHitResult hit;

		double z = 0;

		if (GetWorld()->LineTraceSingleByChannel(hit, Transform.GetLocation() + deltaV + FVector(0.0f, 0.0f, 100.0f), Transform.GetLocation() + deltaV - FVector(0.0f, 0.0f, 100.0f), ECollisionChannel::ECC_GameTraceChannel1))
			z = hit.Location.Z - Transform.GetLocation().Z;
		
		deltaV.Z = z;

		FRotator targetRotation = deltaV.Rotation();
		targetRotation.Pitch = 0.0f;

		Transform.SetLocation(Transform.GetLocation() + deltaV);
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

	Points = VectorPoints;
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