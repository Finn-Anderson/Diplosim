#include "AI/AIMovementComponent.h"

#include "Components/CapsuleComponent.h"
#include "Animation/AnimSingleNodeInstance.h"

#include "AI.h"
#include "DiplosimAIController.h"
#include "Universal/DiplosimUserSettings.h"

UAIMovementComponent::UAIMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bAllowTickBatching = true;
	
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

	if (!IsValid(AI)) {
		SetComponentTickEnabled(false);

		return;
	}

	UAnimSingleNodeInstance* animInst = AI->Mesh->GetSingleNodeInstance();

	if (IsValid(animInst) && IsValid(animInst->GetAnimationAsset()) && animInst->IsPlaying())
		animInst->UpdateAnimation(DeltaTime * AI->Mesh->GlobalAnimRateScale, false);
	else if (Points.IsEmpty())
		SetComponentTickEnabled(false);

	if (DeltaTime < 0.001f || DeltaTime > 1.0f || Points.IsEmpty())
		return;

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

		double z = 0;

		if (GetWorld()->LineTraceSingleByChannel(hit, AI->GetActorLocation(), AI->GetActorLocation() - FVector(0.0f, 0.0f, 200.0f), ECollisionChannel::ECC_GameTraceChannel1, queryParams))
			z = hit.Location.Z;

		FVector location = AI->GetActorLocation() + deltaV;
		location.Z = z + AI->Capsule->GetScaledCapsuleHalfHeight();

		AI->SetActorLocation(location);

		FRotator targetRotation = deltaV.Rotation();
		targetRotation.Pitch = 0.0f;

		if (AI->GetActorRotation() != targetRotation)
			AI->SetActorRotation(FMath::RInterpTo(AI->GetActorRotation(), targetRotation, DeltaTime, 10.0f));

		if (CurrentAnim == MoveAnim)
			return;

		SetAnimation(MoveAnim, true);
	}
	else if (CurrentAnim == MoveAnim) {
		SetAnimation(nullptr, false);

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

	if (!Points.IsEmpty())
		AI->AIController->StartMovement();
	else if (CurrentAnim == MoveAnim)
		SetAnimation(nullptr, false);
}

void UAIMovementComponent::SetMaxSpeed(int32 Energy)
{
	MaxSpeed = FMath::Clamp(FMath::LogX(InitialSpeed, InitialSpeed * (Energy / 100.0f)) * InitialSpeed, InitialSpeed * 0.3f, InitialSpeed) * SpeedMultiplier;
}

float UAIMovementComponent::GetMaximumSpeed()
{
	return MaxSpeed;
}

void UAIMovementComponent::SetAnimation(UAnimSequence* Anim, bool bLooping, bool bSettingsChange)
{
	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();

	if (!settings->GetViewAnimations() && !bSettingsChange)
		return;

	if (IsValid(Anim)) {
		AI->Mesh->PlayAnimation(MoveAnim, bLooping);

		if (!IsComponentTickEnabled())
			SetComponentTickEnabled(true);
	}
	else {
		AI->Mesh->Play(false);
	}

	CurrentAnim = Anim;
}