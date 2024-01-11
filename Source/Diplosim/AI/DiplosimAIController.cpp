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
	UHealthComponent* healthComp = GetOwner()->GetComponentByClass<UHealthComponent>();

	if (!Actor->IsValidLowLevelFast() || healthComp->GetHealth() == 0)
		return false;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* NavData = nav->GetDefaultNavDataInstance();

	MoveRequest.ResetLocation();

	UStaticMeshComponent* comp = Cast<UStaticMeshComponent>(Actor->GetComponentByClass(UStaticMeshComponent::StaticClass()));

	if (comp && comp->DoesSocketExist("Entrance"))
		MoveRequest.SetLocation(comp->GetSocketLocation("Entrance"));

	FVector loc = FVector::Zero();
	if (MoveRequest.GetLocation() != FVector::Zero()) {
		loc = MoveRequest.GetLocation();
	}
	else {
		loc = Actor->GetActorLocation();
	}

	FPathFindingQuery query(this, *NavData, GetOwner()->GetActorLocation(), loc);

	bool path = nav->TestPathSync(query, EPathFindingMode::Hierarchical);

	if (path)
		return true;

	return false;
}

void ADiplosimAIController::AIMoveTo(AActor* Actor)
{
	if (!CanMoveTo(Actor))
		return;

	MoveRequest.SetGoalActor(Actor);

	TSubclassOf<UNavigationQueryFilter> filter = DefaultNavigationFilterClass;

	if (*Cast<AAI>(GetOwner())->NavQueryFilter)
		filter = Cast<AAI>(GetOwner())->NavQueryFilter;

	if (MoveRequest.GetLocation() != FVector::Zero()) {
		MoveToLocation(MoveRequest.GetLocation(), 1.0f, true, true, false, true, filter, true);
	}
	else {
		MoveToActor(MoveRequest.GetGoalActor(), 1.0f, true, true, true, filter, true);
	}

	SetFocus(Actor);

	if (!GetOwner()->IsA<ACitizen>() || Cast<ACitizen>(GetOwner())->Building.BuildingAt == Actor)
		return;

	ACitizen* citizen = Cast<ACitizen>(GetOwner()); 
	
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
	else if (citizen->Building.BuildingAt != nullptr) {
		citizen->Building.BuildingAt->Leave(citizen);
	}
}