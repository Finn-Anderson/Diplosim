#include "AI/AIMovementComponent.h"

#include "Components/CapsuleComponent.h"
#include "Animation/AnimSingleNodeInstance.h"

#include "AI.h"
#include "DiplosimAIController.h"
#include "Universal/DiplosimUserSettings.h"

UAIMovementComponent::UAIMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	MaxSpeed = 200.0f;
	InitialSpeed = 200.0f;
	SpeedMultiplier = 1.0f;

	CurrentAnim = nullptr;

	LastUpdatedTime = 0.0f;
	Transform = FTransform();
}

void UAIMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	AI = Cast<AAI>(GetOwner());
}

void UAIMovementComponent::ComputeMovement(float DeltaTime)
{
	LastUpdatedTime = GetWorld()->GetTimeSeconds();

	if (!IsValid(AI) || DeltaTime < 0.001f || DeltaTime > 1.0f)
		return;

	UAnimSingleNodeInstance* animInst = AI->Mesh->GetSingleNodeInstance();

	if (IsValid(animInst) && IsValid(animInst->GetAnimationAsset()) && animInst->IsPlaying()) {
		animInst->UpdateAnimation(DeltaTime * AI->Mesh->GlobalAnimRateScale, false);

		AI->Mesh->RefreshBoneTransforms(); 
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

		if (CurrentAnim == MoveAnim)
			return;

		//SetAnimation(MoveAnim, true);
	}
	else if (CurrentAnim == MoveAnim) {
		//SetAnimation(nullptr, false);

		AI->AIController->StopMovement();
	}
}

FVector UAIMovementComponent::CalculateVelocity(FVector Vector)
{
	return (Vector - Transform.GetLocation()).Rotation().Vector() * MaxSpeed;
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

	if (IsValid(Anim))
		AI->Mesh->PlayAnimation(MoveAnim, bLooping);
	else
		AI->Mesh->Play(false);

	CurrentAnim = Anim;
}