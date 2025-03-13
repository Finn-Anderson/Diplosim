#include "AI/AIMovementComponent.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

#include "AI.h"
#include "Citizen.h"
#include "Enemy.h"
#include "DiplosimAIController.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Player/Camera.h"
#include "Map/Grid.h"
#include "Universal/Resource.h"

UAIMovementComponent::UAIMovementComponent()
{
	MaxSpeed = 200.0f;
	InitialSpeed = 200.0f;
	SpeedMultiplier = 1.0f;

	CurrentAnim = nullptr;

	avoidPoints = 0;
}

void UAIMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (DeltaTime < 0.0001f)
		return;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	if (!Points.IsEmpty() && FVector::DistXY(GetOwner()->GetActorLocation(), Points[0]) < 12.0f) {
		if (avoidPoints > 0)
			avoidPoints--;

		Points.RemoveAt(0);
	}

	if (avoidPoints == 0 && !Points.IsEmpty()) {
		FHitResult hit;

		APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		ACamera* camera = PController->GetPawn<ACamera>();

		FCollisionQueryParams queryParams;
		queryParams.AddIgnoredActor(GetOwner());
		queryParams.AddIgnoredActor(camera->Grid);

		for (FResourceHISMStruct vegetation : camera->Grid->VegetationStruct)
			queryParams.AddIgnoredActor(vegetation.Resource);

		FVector startTrace = GetOwner()->GetActorLocation();
		FVector endTrace = startTrace + GetOwner()->GetActorRotation().Vector() * 30.0f;

		if (GetWorld()->SweepSingleByChannel(hit, startTrace, endTrace, FRotator(0.0f).Quaternion(), ECollisionChannel::ECC_Visibility, FCollisionShape::MakeCapsule(FVector(9.0f, 9.0f, 11.5f)), queryParams)) {
			if ((hit.GetActor()->IsA<ACitizen>() && GetOwner()->IsA<ACitizen>() && Cast<ACitizen>(hit.GetActor())->Rebel == Cast<ACitizen>(GetOwner())->Rebel) || (hit.GetActor()->IsA<AEnemy>() && GetOwner()->IsA<AEnemy>())) {
				if (FVector::DistXY(hit.GetActor()->GetActorLocation(), Points[0]) >= 12.0f) {
					FRotator rotation = (hit.GetActor()->GetActorLocation() - startTrace).Rotation();
					rotation.Roll = 0.0f;
					rotation.Pitch = 0.0f;

					if (rotation.Yaw < GetOwner()->GetActorRotation().Yaw)
						rotation.Yaw += 45.0f;
					else
						rotation.Yaw -= 45.0f;

					FNavLocation location;
					nav->ProjectPointToNavigation(startTrace + (rotation.Vector() * 90.0f), location, FVector(400.0f, 400.0f, 20.0f));

					TArray<FVector> newPoints = Cast<AAI>(GetOwner())->AIController->GetPathPoints(startTrace, location.Location);

					if (!newPoints.IsEmpty()) {
						TArray<FVector> regeneratedPoints = Cast<AAI>(GetOwner())->AIController->GetPathPoints(newPoints.Last(), Points.Last());

						avoidPoints = newPoints.Num();

						newPoints.Append(regeneratedPoints);

						Points = newPoints;
					}
				}
				else
					Points.RemoveAt(0);
			}
		}
	}
		
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

		FVector location = GetOwner()->GetActorLocation() + deltaV;
		location.Z = deltaTarget.Location.Z + 6.0f;

		GetOwner()->SetActorLocation(location);
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

float UAIMovementComponent::GetMaximumSpeed()
{
	return MaxSpeed;
}