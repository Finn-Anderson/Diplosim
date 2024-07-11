#include "AI/DiplosimAIController.h"

#include "Navigation/CrowdFollowingComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Kismet/GameplayStatics.h"

#include "AI.h"
#include "Citizen.h"
#include "Enemy.h"
#include "Universal/HealthComponent.h"
#include "Buildings/Building.h"
#include "Buildings/Misc/Broch.h"
#include "Universal/Resource.h"
#include "AttackComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"

ADiplosimAIController::ADiplosimAIController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickInterval(0.4f);

	PrevGoal = nullptr;
}

void ADiplosimAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!Owner->IsActorTickEnabled() || !IsValid(MoveRequest.GetGoalActor()))
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

	if (GetOwner()->GetVelocity() == FVector::Zero() && (GetOwner()->IsA<AEnemy>() || !Cast<ACitizen>(GetOwner())->StillColliding.Contains(collidingStruct)))
		RecalculateMovement(MoveRequest.GetGoalActor());

	if (MoveRequest.GetGoalActor()->IsA<AAI>())
		MoveToActor(MoveRequest.GetGoalActor(), 1.0f, true, true, true, filter, true);
}

void ADiplosimAIController::DefaultAction()
{
	if (MoveRequest.GetGoalActor() == PrevGoal)
		return;

	MoveRequest.SetGoalActor(nullptr);

	SetActorTickEnabled(false);

	if (PrevGoal == nullptr || !PrevGoal->IsValidLowLevelFast()) {
		if (GetOwner()->IsA<ACitizen>()) {
			ACitizen* citizen = Cast<ACitizen>(GetOwner());

			if (citizen->Building.Employment != nullptr)
				AIMoveTo(citizen->Building.Employment);
			else
				Idle();
		}
		else
			Cast<AAI>(GetOwner())->MoveToBroch();
	}
	else
		AIMoveTo(PrevGoal);
}

void ADiplosimAIController::Idle()
{
	if (!GetOwner()->IsValidLowLevelFast())
		return;

	PrevGoal = nullptr;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	FNavLocation location;

	TArray<AActor*> brochs;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABroch::StaticClass(), brochs);

	nav->GetRandomPointInNavigableRadius(brochs[0]->GetActorLocation(), 1000, location);

	MoveToLocation(location);

	int32 time = FMath::RandRange(2, 10);
	GetWorld()->GetTimerManager().SetTimer(IdleTimer, this, &ADiplosimAIController::Idle, time, false);
}

double ADiplosimAIController::GetClosestActor(FVector TargetLocation, FVector CurrentLocation, FVector NewLocation, int32 CurrentValue, int32 NewValue)
{
	if (!CanMoveTo(NewLocation))
		return -1000000.0f;

	double curLength = FVector::Dist(TargetLocation, CurrentLocation);

	double newLength = FVector::Dist(TargetLocation, NewLocation);

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

			if (building->Storage < 1)
				continue;

			if (target == nullptr) {
				target = building;

				continue;
			}

			int32 storage = 0;

			if (target != nullptr)
				storage = target->Storage;

			double magnitude = GetClosestActor(GetOwner()->GetActorLocation(), target->GetActorLocation(), building->GetActorLocation(), storage, building->Storage);

			if (magnitude <= 0.0f)
				continue;

			target = building;
		}
	}

	if (target != nullptr)
		AIMoveTo(target);
	else {
		FTimerHandle checkSitesTimer;
		GetWorldTimerManager().SetTimer(checkSitesTimer, FTimerDelegate::CreateUObject(this, &ADiplosimAIController::GetGatherSite, Camera, Resource), 30.0f, false);
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
	nav->ProjectPointToNavigation(Location, targetLoc, FVector(200.0f, 200.0f, 10.0f));

	FNavLocation ownerLoc;
	nav->ProjectPointToNavigation(GetOwner()->GetActorLocation(), ownerLoc, FVector(200.0f, 200.0f, 10.0f));

	FPathFindingQuery query(GetOwner(), *navData, ownerLoc.Location, targetLoc.Location);

	bool path = nav->TestPathSync(query, EPathFindingMode::Hierarchical);

	if (path)
		return true;

	return false;
}

void ADiplosimAIController::AIMoveTo(AActor* Actor, FVector Location, int32 Instance)
{
	if (!Actor->IsValidLowLevelFast()) {
		DefaultAction();

		return;
	}

	if (Actor == MoveRequest.GetGoalActor())
		return;

	if (MoveRequest.GetGoalActor()->IsValidLowLevelFast() && !MoveRequest.GetGoalActor()->IsA<AAI>())
		PrevGoal = MoveRequest.GetGoalActor();

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

	if (GetWorldTimerManager().IsTimerActive(IdleTimer))
		GetWorldTimerManager().ClearTimer(IdleTimer);

	TSubclassOf<UNavigationQueryFilter> filter = nullptr;

	if (GetOwner()->IsValidLowLevelFast() && Cast<AAI>(GetOwner())->NavQueryFilter != nullptr)
		filter = Cast<AAI>(GetOwner())->NavQueryFilter;

	MoveToLocation(MoveRequest.GetLocation(), 1.0f, true, true, false, true, filter, true);

	SetFocus(Actor);

	if (!GetOwner()->IsA<ACitizen>() || Cast<ACitizen>(GetOwner())->Building.BuildingAt == Actor)
		return;

	ACitizen* citizen = Cast<ACitizen>(GetOwner());

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