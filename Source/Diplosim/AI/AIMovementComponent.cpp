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

	if (DeltaTime > 1.0f)
		return;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	float distance = FMath::Min(150.0f * DeltaTime, Cast<AAI>(GetOwner())->AttackComponent->RangeComponent->GetUnscaledSphereRadius() / 20.0f);

	if (!Points.IsEmpty() && FVector::DistXY(GetOwner()->GetActorLocation(), Points[0]) < distance) {
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

	if (!Points.IsEmpty()) {
		FHitResult hit;

		APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		ACamera* camera = PController->GetPawn<ACamera>();

		FCollisionQueryParams queryParams;
		queryParams.AddIgnoredActor(GetOwner());
		queryParams.AddIgnoredActor(camera->Grid);

		TArray<FResourceHISMStruct> resourceList;
		resourceList.Append(camera->Grid->TreeStruct);
		resourceList.Append(camera->Grid->FlowerStruct);

		for (FResourceHISMStruct vegetation : resourceList)
			queryParams.AddIgnoredActor(vegetation.Resource);

		FVector startTrace = GetOwner()->GetActorLocation();
		FVector endTrace = startTrace + Velocity.Rotation().Vector() * 50.0f;

		if (GetWorld()->SweepSingleByChannel(hit, startTrace, endTrace, FRotator(0.0f).Quaternion(), ECollisionChannel::ECC_Visibility, FCollisionShape::MakeCapsule(FVector(9.0f, 9.0f, 11.5f)), queryParams)) {
			if (!avoidPoints.Contains(hit.GetActor()) && hit.GetActor()->IsA<ABuilding>() && Cast<AAI>(GetOwner())->AIController->MoveRequest.GetGoalActor() != hit.GetActor() && !(hit.GetActor()->IsA<ARoad>() || hit.GetActor()->IsA<AFestival>())) {
				FVector hitSize = Cast<ABuilding>(hit.GetActor())->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize();

				FVector ownerSize = Cast<AAI>(GetOwner())->Mesh->GetSkeletalMeshAsset()->GetBounds().GetBox().GetSize();

				TArray<FVector> pos;

				for (int32 i = -1; i < 2; i += 2)
					pos.Add(hit.GetActor()->GetActorLocation() + FVector(i * (hitSize.X + ownerSize.X), 0.0f, 0.0f));

				for (int32 i = -1; i < 2; i += 2)
					pos.Add(hit.GetActor()->GetActorLocation() + FVector(0.0f, i * (hitSize.Y + ownerSize.Y), 0.0f));

				FVector closest = pos[0];
				FVector furthest = pos[0];

				for (int32 i = 1; i < pos.Num(); i++) {
					float curClosestDist = FVector::Dist(GetOwner()->GetActorLocation(), closest);
					float curFurthestDist = FVector::Dist(GetOwner()->GetActorLocation(), furthest);

					float newDist = FVector::Dist(GetOwner()->GetActorLocation(), pos[i]);

					if (newDist < curClosestDist)
						closest = pos[i];
					else if (newDist > curFurthestDist)
						furthest = pos[i];
				}

				pos.Remove(closest);
				pos.Remove(furthest);

				if (hit.GetActor()->IsA<AAI>()) {
					FVector v = Cast<AAI>(hit.GetActor())->MovementComponent->Velocity;
					FRotator direction = FRotator(0.0f, FMath::RoundHalfFromZero(v.Rotation().Yaw / 90.0f) * 90.0f, 0.0f);

					for (int32 i = pos.Num() - 1; i > -1; i--) {
						FRotator rotation = FRotator(0.0f, FMath::RoundHalfFromZero(pos[i].Rotation().Yaw / 90.0f) * 90.0f, 0.0f);

						if (rotation.Yaw != direction.Yaw)
							continue;

						pos.RemoveAt(i);

						break;
					}
				}

				closest = pos[0];

				if (pos.Num() > 1) {
					float dist0 = FVector::Dist(GetOwner()->GetActorLocation(), pos[0]);
					float dist1 = FVector::Dist(GetOwner()->GetActorLocation(), pos[1]);

					if (dist0 > dist1)
						closest = pos[1];
				}

				FNavLocation location;
				nav->ProjectPointToNavigation(closest, location, FVector(400.0f, 400.0f, 20.0f));

				TArray<FVector> newPoints = Cast<AAI>(GetOwner())->AIController->GetPathPoints(startTrace, location.Location);

				if (!newPoints.IsEmpty()) {
					TArray<FVector> regeneratedPoints = Cast<AAI>(GetOwner())->AIController->GetPathPoints(newPoints.Last(), Points.Last());

					avoidPoints.Add(hit.GetActor(), newPoints);

					newPoints.Append(regeneratedPoints);

					Points = newPoints;
				}
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
		FHitResult hit;

		FCollisionQueryParams queryParams;
		queryParams.AddIgnoredActor(GetOwner());

		int32 z = 0;

		if (GetWorld()->LineTraceSingleByChannel(hit, GetOwner()->GetActorLocation(), GetOwner()->GetActorLocation() - FVector(0.0f, 0.0f, 200.0f), ECollisionChannel::ECC_GameTraceChannel1, queryParams))
			z = hit.Location.Z;

		FVector location = GetOwner()->GetActorLocation() + deltaV;
		location.Z = z + Cast<AAI>(GetOwner())->Capsule->GetScaledCapsuleHalfHeight();

		GetOwner()->SetActorLocation(location);

		FRotator targetRotation = deltaV.Rotation() - FRotator(0.0f, 180.0f, 0.0f);
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
}

void UAIMovementComponent::SetMaxSpeed(int32 Energy)
{
	MaxSpeed = FMath::Clamp(FMath::LogX(InitialSpeed, InitialSpeed * (Energy / 100.0f)) * InitialSpeed, InitialSpeed * 0.3f, InitialSpeed) * SpeedMultiplier;
}

float UAIMovementComponent::GetMaximumSpeed()
{
	return MaxSpeed;
}