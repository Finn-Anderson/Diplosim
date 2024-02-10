#include "AI/DiplosimAIController.h"

#include "Navigation/CrowdFollowingComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"

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

FClosestStruct ADiplosimAIController::GetClosestActor(AActor* CurrentActor, AActor* NewActor, int32 CurrentValue, int32 NewValue)
{
	FClosestStruct closestStruct;
	closestStruct.Actor = CurrentActor;

	if (!CanMoveTo(NewActor))
		return closestStruct;
	
	closestStruct.Actor = NewActor;

	if (CurrentActor == nullptr)
		return closestStruct;

	double curLength = FVector::Dist(GetOwner()->GetActorLocation(), CurrentActor->GetActorLocation());

	double newLength = FVector::Dist(GetOwner()->GetActorLocation(), NewActor->GetActorLocation());

	closestStruct.Magnitude = curLength / CurrentValue - newLength / NewValue;

	if (closestStruct.Magnitude < 0.0f) {
		closestStruct.Actor = CurrentActor;
	}

	return closestStruct;
}

bool ADiplosimAIController::CanMoveTo(AActor* Actor)
{
	if (!GetOwner()->IsValidLowLevelFast() || !Actor->IsValidLowLevelFast())
		return false;

	UHealthComponent* healthComp = GetOwner()->GetComponentByClass<UHealthComponent>();

	if (healthComp->GetHealth() == 0)
		return false;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* navData = nav->GetDefaultNavDataInstance();

	FVector loc = Actor->GetActorLocation();

	if (Actor->IsA<ABuilding>())
		loc.Y -= Cast<ABuilding>(Actor)->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize().Y / 2;

	FPathFindingQuery query(Owner, *navData, GetOwner()->GetActorLocation(), loc);

	bool path = nav->TestPathSync(query, EPathFindingMode::Hierarchical);

	if (path)
		return true;

	return false;
}

void ADiplosimAIController::AIMoveTo(AActor* Actor)
{
	if (!CanMoveTo(Actor))
		return;

	GetWorld()->GetTimerManager().ClearTimer(IdleTimer);

	MoveRequest.ResetLocation();

	MoveRequest.SetGoalActor(Actor);

	UStaticMeshComponent* comp = Cast<UStaticMeshComponent>(Actor->GetComponentByClass(UStaticMeshComponent::StaticClass()));

	if (comp && comp->DoesSocketExist("Entrance"))
		MoveRequest.SetLocation(comp->GetSocketLocation("Entrance"));

	TSubclassOf<UNavigationQueryFilter> filter = DefaultNavigationFilterClass;

	if (*Cast<AAI>(GetOwner())->NavQueryFilter)
		filter = Cast<AAI>(GetOwner())->NavQueryFilter;

	if (MoveRequest.GetLocation() != FVector::Zero())
		MoveToLocation(MoveRequest.GetLocation(), 1.0f, true, true, false, true, filter, true);
	else
		MoveToActor(MoveRequest.GetGoalActor(), -1.0f, true, true, true, filter, true);

	SetFocus(Actor);

	if (!GetOwner()->IsA<ACitizen>() || Cast<ACitizen>(GetOwner())->Building.BuildingAt == Actor)
		return;

	ACitizen* citizen = Cast<ACitizen>(GetOwner());

	if (citizen->Building.BuildingAt != nullptr) {
		citizen->Building.BuildingAt->Leave(citizen);
	} 
	
	if (citizen->StillColliding.Contains(Actor)) {
		if (Actor->IsA<ABuilding>()) {
			ABuilding* building = Cast<ABuilding>(Actor);

			building->Enter(citizen);
		}
		else if (Actor->IsA<AResource>()) {
			AResource* resource = Cast<AResource>(Actor);

			citizen->StartHarvestTimer(resource);
		}
	}
}