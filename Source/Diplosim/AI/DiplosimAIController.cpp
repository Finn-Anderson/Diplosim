#include "AI/DiplosimAIController.h"

#include "Navigation/CrowdFollowingComponent.h"
#include "NavigationSystem.h"

#include "AI.h"
#include "Citizen.h"
#include "Buildings/Building.h"

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

FVector ADiplosimAIController::GetLocationOrDestination()
{
	if (MoveRequest.GetLocation() != FVector::Zero())
		return MoveRequest.GetLocation();

	return MoveRequest.GetGoalActor()->GetActorLocation();
}

bool ADiplosimAIController::CanMoveTo(AActor* Actor)
{
	if (!Actor->IsValidLowLevelFast())
		return false;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* NavData = nav->GetDefaultNavDataInstance();

	MoveRequest.Reset();

	UStaticMeshComponent* comp = Cast<UStaticMeshComponent>(Actor->GetComponentByClass(UStaticMeshComponent::StaticClass()));

	if (comp && comp->DoesSocketExist("Entrance"))
		MoveRequest.SetLocation(comp->GetSocketLocation("Entrance"));

	MoveRequest.SetGoalActor(Actor);

	FPathFindingQuery query(this, *NavData, GetOwner()->GetActorLocation(), GetLocationOrDestination());

	bool path = nav->TestPathSync(query, EPathFindingMode::Hierarchical);

	if (path)
		return true;

	return false;
}

void ADiplosimAIController::AIMoveTo(AActor* Actor)
{
	if (CanMoveTo(Actor)) {
		MoveToLocation(GetLocationOrDestination());

		if (GetOwner()->IsA<ACitizen>()) {
			ACitizen* citizen = Cast<ACitizen>(GetOwner());

			if (citizen->Building.BuildingAt != nullptr && citizen->Building.BuildingAt != Actor) {
				citizen->Building.BuildingAt->Leave(citizen);
			}
		}
	}
}