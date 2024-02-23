#include "AI/DiplosimAIController.h"

#include "Navigation/CrowdFollowingComponent.h"
#include "NavigationSystem.h"

#include "AI.h"
#include "Citizen.h"
#include "HealthComponent.h"
#include "Buildings/Building.h"
#include "Resource.h"

ADiplosimAIController::ADiplosimAIController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	
}

void ADiplosimAIController::Idle()
{
	if (!GetOwner()->IsValidLowLevelFast())
		return;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	FNavLocation location;

	nav->GetRandomPointInNavigableRadius(GetOwner()->GetActorLocation(), 500, location);

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

bool ADiplosimAIController::CanMoveTo(FVector Location)
{
	if (!GetOwner()->IsValidLowLevelFast())
		return false;

	UHealthComponent* healthComp = GetOwner()->GetComponentByClass<UHealthComponent>();

	if (healthComp->GetHealth() == 0)
		return false;

	MoveRequest.ResetLocation();

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* navData = nav->GetDefaultNavDataInstance();

	FNavLocation loc;
	nav->ProjectPointToNavigation(Location, loc, FVector(200.0f, 200.0f, 10.0f));

	MoveRequest.SetLocation(loc.Location);

	FPathFindingQuery query(Owner, *navData, GetNavAgentLocation(), loc.Location);

	bool path = nav->TestPathSync(query, EPathFindingMode::Hierarchical);

	if (path)
		return true;

	return false;
}

void ADiplosimAIController::AIMoveTo(AActor* Actor, FVector Location, int32 Instance)
{
	if (!Actor->IsValidLowLevelFast())
		return;

	FTransform transform;
	transform.SetLocation(Actor->GetActorLocation());

	if (Location != FVector::Zero())
		transform.SetLocation(Location);

	if (!CanMoveTo(transform.GetLocation()))
		return;

	MoveRequest.SetGoalActor(Actor);
	MoveRequest.SetGoalInstance(Instance);

	UStaticMeshComponent* comp = Cast<UStaticMeshComponent>(Actor->GetComponentByClass(UStaticMeshComponent::StaticClass()));

	if (comp && comp->DoesSocketExist("Entrance"))
		MoveRequest.SetLocation(comp->GetSocketLocation("Entrance"));

	GetWorld()->GetTimerManager().ClearTimer(IdleTimer);

	TSubclassOf<UNavigationQueryFilter> filter = DefaultNavigationFilterClass;

	if (*Cast<AAI>(GetOwner())->NavQueryFilter)
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