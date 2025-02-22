#include "AI/DiplosimAIController.h"

#include "Navigation/CrowdFollowingComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Kismet/GameplayStatics.h"
#include "NavFilters/NavigationQueryFilter.h"

#include "AI.h"
#include "Citizen.h"
#include "Enemy.h"
#include "Universal/HealthComponent.h"
#include "Buildings/Building.h"
#include "Buildings/House.h"
#include "Buildings/Work/Work.h"
#include "Buildings/Misc/Broch.h"
#include "Universal/Resource.h"
#include "AttackComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/CitizenManager.h"
#include "AIMovementComponent.h"

ADiplosimAIController::ADiplosimAIController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickInterval(0.4f);
}

void ADiplosimAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (DeltaTime < 0.0001f)
		return;

	if (Cast<AAI>(Owner)->HealthComponent->Health == 0 || !IsValid(MoveRequest.GetGoalActor()))
		return;

	TSubclassOf<UNavigationQueryFilter> filter = Cast<AAI>(GetOwner())->NavQueryFilter;

	FCollidingStruct collidingStruct;
	collidingStruct.Actor = MoveRequest.GetGoalActor();
	collidingStruct.Instance = MoveRequest.GetGoalInstance();

	if (Cast<AAI>(GetOwner())->AttackComponent->MeleeableEnemies.Contains(MoveRequest.GetGoalActor())) {
		FRotator rotation = (MoveRequest.GetGoalActor()->GetActorLocation() - Owner->GetActorLocation()).Rotation();

		GetOwner()->SetActorRotation(FMath::RInterpTo(GetOwner()->GetActorRotation(), rotation, DeltaTime, 100.0f));

		return;
	}

	if ((FMath::IsNearlyZero(GetOwner()->GetVelocity().X) && FMath::IsNearlyZero(GetOwner()->GetVelocity().Y)) && (GetOwner()->IsA<AEnemy>() || !Cast<ACitizen>(GetOwner())->StillColliding.Contains(collidingStruct)))
		RecalculateMovement(MoveRequest.GetGoalActor());

	if (MoveRequest.GetGoalActor()->IsA<AAI>())
		MoveToActor(MoveRequest.GetGoalActor(), 1.0f, true, true, true, filter, true);
}

void ADiplosimAIController::DefaultAction()
{
	MoveRequest.SetGoalActor(nullptr);

	SetActorTickEnabled(false);

	if (GetOwner()->IsA<ACitizen>() && !Cast<ACitizen>(GetOwner())->Rebel) {
		ACitizen* citizen = Cast<ACitizen>(GetOwner());

		if (citizen->Building.Employment != nullptr && citizen->Building.Employment->bOpen)
			AIMoveTo(citizen->Building.Employment);
		else if (citizen->Building.School != nullptr)
			AIMoveTo(citizen->Building.School);
		else if (citizen->Building.House != nullptr)
			AIMoveTo(citizen->Building.House);
		else
			Idle();
	}
	else
		Cast<AAI>(GetOwner())->MoveToBroch();
}

void ADiplosimAIController::Idle()
{
	AsyncTask(ENamedThreads::GameThread, [this]() {
		if (!GetOwner()->IsValidLowLevelFast())
			return;

		UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
		const ANavigationData* navData = nav->GetDefaultNavDataInstance();

		FNavLocation location;
		nav->GetRandomPointInNavigableRadius(Cast<ACitizen>(GetOwner())->Camera->CitizenManager->BrochLocation, 1000, location);

		double length;
		nav->GetPathLength(GetOwner()->GetActorLocation(), location, length);

		FTimerStruct timer;
		timer.CreateTimer("Idle", GetOwner(), FMath::RandRange(3, 10), FTimerDelegate::CreateUObject(this, &ADiplosimAIController::Idle), false);

		Cast<ACitizen>(GetOwner())->Camera->CitizenManager->Timers.Add(timer);

		if (length > 5000.0f)
			return;

		UNavigationPath* path = nav->FindPathToLocationSynchronously(GetWorld(), GetOwner()->GetActorLocation(), location, GetOwner(), Cast<AAI>(GetOwner())->NavQueryFilter);

		Cast<AAI>(GetOwner())->MovementComponent->SetPoints(path->PathPoints);
	});
}

double ADiplosimAIController::GetClosestActor(FVector TargetLocation, FVector CurrentLocation, FVector NewLocation, int32 CurrentValue, int32 NewValue)
{
	if (!CanMoveTo(NewLocation))
		return -1000000.0f;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* navData = nav->GetDefaultNavDataInstance();

	FNavLocation projectedTarget;
	nav->ProjectPointToNavigation(TargetLocation, projectedTarget, FVector(400.0f, 400.0f, 20.0f));

	FNavLocation projectedCurrent;
	nav->ProjectPointToNavigation(CurrentLocation, projectedCurrent, FVector(400.0f, 400.0f, 20.0f));

	FNavLocation projectedNew;
	nav->ProjectPointToNavigation(NewLocation, projectedNew, FVector(400.0f, 400.0f, 20.0f));

	double curLength = 100000000000.0f;
	navData->CalcPathLength(projectedTarget, projectedCurrent, curLength);

	double newLength = 100000000000.0f;
	navData->CalcPathLength(projectedTarget, projectedNew, newLength);

	double magnitude = curLength / CurrentValue - newLength / NewValue;

	return magnitude;
}

void ADiplosimAIController::GetGatherSite(ACamera* Camera, TSubclassOf<AResource> Resource)
{
	TArray<TSubclassOf<class ABuilding>> buildings = Camera->ResourceManager->GetBuildings(Resource);

	ABuilding* target = nullptr;

	for (int32 j = 0; j < buildings.Num(); j++) {
		TArray<AActor*> foundBuildings;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), buildings[j], foundBuildings);

		for (int32 k = 0; k < foundBuildings.Num(); k++) {
			ABuilding* building = Cast<ABuilding>(foundBuildings[k]);

			FItemStruct itemStruct;
			itemStruct.Resource = Resource;

			int32 index = building->Storage.Find(itemStruct);

			if (building->Storage[index].Amount < 1)
				continue;

			if (target == nullptr) {
				target = building;

				continue;
			}

			int32 targetIndex = target->Storage.Find(itemStruct);

			double magnitude = GetClosestActor(GetOwner()->GetActorLocation(), target->GetActorLocation(), building->GetActorLocation(), target->Storage[targetIndex].Amount, building->Storage[index].Amount);

			if (magnitude <= 0.0f)
				continue;

			target = building;
		}
	}

	if (target != nullptr)
		AIMoveTo(target);
	else {
		FTimerStruct timer;
		timer.CreateTimer("FindGatherSite", GetOwner(), 30.0f, FTimerDelegate::CreateUObject(this, &ADiplosimAIController::GetGatherSite, Camera, Resource), false);

		Camera->CitizenManager->Timers.Add(timer);
	}
}

bool ADiplosimAIController::CanMoveTo(FVector Location)
{
	if (!Owner->IsValidLowLevelFast())
		return false;

	UHealthComponent* healthComp = GetOwner()->GetComponentByClass<UHealthComponent>();

	if (healthComp && healthComp->GetHealth() == 0)
		return false;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* navData = nav->GetDefaultNavDataInstance();

	FNavLocation targetLoc;
	nav->ProjectPointToNavigation(Location, targetLoc, FVector(400.0f, 400.0f, 20.0f));

	FNavLocation ownerLoc;
	nav->ProjectPointToNavigation(GetOwner()->GetActorLocation(), ownerLoc, FVector(400.0f, 400.0f, 20.0f));

	FPathFindingQuery query(GetOwner(), *navData, ownerLoc.Location, targetLoc.Location);

	bool path = nav->TestPathSync(query, EPathFindingMode::Hierarchical);

	if (path)
		return true;

	return false;
}

void ADiplosimAIController::AIMoveTo(AActor* Actor, FVector Location, int32 Instance)
{
	if (Cast<AAI>(GetOwner())->AttackComponent->MeleeableEnemies.Contains(MoveRequest.GetGoalActor()) || (GetOwner()->IsA<ACitizen>() && Cast<ACitizen>(GetOwner())->Building.BuildingAt == Actor))
		return;
	
	MoveRequest.SetGoalActor(Actor);
	MoveRequest.SetGoalInstance(Instance);
	MoveRequest.SetLocation(Actor->GetActorLocation());

	SetActorTickEnabled(true);

	if (Location != FVector::Zero())
		MoveRequest.SetLocation(Location);

	if (Actor->IsA<ABuilding>()) {
		UStaticMeshComponent* comp = Actor->GetComponentByClass<UStaticMeshComponent>();

		if (comp && comp->DoesSocketExist("Entrance"))
			MoveRequest.SetLocation(comp->GetSocketLocation("Entrance"));
	}

	TSubclassOf<UNavigationQueryFilter> filter = nullptr;

	if (GetOwner()->IsValidLowLevelFast() && Cast<AAI>(GetOwner())->NavQueryFilter != nullptr)
		filter = Cast<AAI>(GetOwner())->NavQueryFilter;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	FNavLocation navLoc;
	nav->ProjectPointToNavigation(MoveRequest.GetLocation(), navLoc, FVector(400.0f, 400.0f, 20.0f));

	MoveRequest.SetLocation(navLoc.Location);

	FVector startLocation = GetOwner()->GetActorLocation();

	if (GetOwner()->IsA<ACitizen>() && Cast<ACitizen>(GetOwner())->Building.BuildingAt != nullptr)
		startLocation = Cast<ACitizen>(GetOwner())->Building.EnterLocation;

	UNavigationPath* path = nav->FindPathToLocationSynchronously(GetWorld(), startLocation, MoveRequest.GetLocation(), GetOwner(), filter);

	Cast<AAI>(GetOwner())->MovementComponent->SetPoints(path->PathPoints);

	SetFocus(Actor);

	if (!GetOwner()->IsA<ACitizen>() || Cast<ACitizen>(GetOwner())->Building.BuildingAt == Actor)
		return;

	ACitizen* citizen = Cast<ACitizen>(GetOwner());

	citizen->Camera->CitizenManager->RemoveTimer("Idle", GetOwner());

	if (citizen->Building.BuildingAt != nullptr)
		citizen->Building.BuildingAt->Leave(citizen);

	FCollidingStruct collidingStruct;
	collidingStruct.Actor = Actor;
	collidingStruct.Instance = Instance;
	
	if (!citizen->StillColliding.Contains(collidingStruct))
		return;

	if (Actor->IsA<ABuilding>()) {
		ABuilding* building = Cast<ABuilding>(Actor);

		building->Enter(citizen);
	}
	else if (Actor->IsA<AResource>()) {
		AResource* resource = Cast<AResource>(Actor);

		citizen->StartHarvestTimer(resource, Instance);
	}
}

void ADiplosimAIController::RecalculateMovement(AActor* Actor)
{
	if (MoveRequest.GetGoalActor() != Actor)
		return;

	AIMoveTo(Actor);
}

void ADiplosimAIController::StopMovement()
{
	Cast<AAI>(GetOwner())->MovementComponent->SetPoints({});
}