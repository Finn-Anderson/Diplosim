#include "AI/DiplosimAIController.h"

#include "Navigation/CrowdFollowingComponent.h"
#include "NavigationSystem.h"

#include "AI.h"
#include "Citizen.h"
#include "Buildings/Building.h"

ADiplosimAIController::ADiplosimAIController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{

}

FAIMoveRequest ADiplosimAIController::CreateMoveRequest()
{
	FAIMoveRequest moveRequest; // create own struct

	return moveRequest;
}

FClosestStruct ADiplosimAIController::GetClosestActor(AActor* CurrentActor, AActor* NewActor, int32 CurrentValue, int32 NewValue)
{
	FClosestStruct closestStruct;
	closestStruct.Actor = NewActor;

	if (CurrentActor == nullptr)
		return closestStruct;

	double outLength = 0.0f;

	TArray<AActor*> actors = { CurrentActor, NewActor };
	TArray<double> paths;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* NavData = nav->GetDefaultNavDataInstance();

	for (AActor* actor : actors) {
		MoveRequest = CreateMoveRequest();

		MoveRequest.SetGoalActor(actor);

		NavData->CalcPathLength(GetOwner()->GetActorLocation(), actor->GetActorLocation(), outLength);

		paths.Add(outLength);
	}

	closestStruct.Magnitude = paths[0] / CurrentValue - paths[1] / NewValue;

	if (closestStruct.Magnitude < 0.0f) {
		closestStruct.Actor = CurrentActor;
	}

	return closestStruct;
}

FVector ADiplosimAIController::GetLocationOrDestination(FAIMoveRequest MoveReq)
{
	UStaticMeshComponent* comp = Cast<UStaticMeshComponent>(MoveReq.GetGoalActor()->GetComponentByClass(UStaticMeshComponent::StaticClass()));

	if (comp && comp->DoesSocketExist("Entrance"))
		return comp->GetSocketLocation("Entrance");

	return MoveReq.GetDestination();
}

bool ADiplosimAIController::CanMoveTo(AActor* Actor)
{
	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* NavData = nav->GetDefaultNavDataInstance();

	MoveRequest = CreateMoveRequest();

	MoveRequest.SetGoalActor(Actor);

	FPathFindingQuery query(this, *NavData, GetOwner()->GetActorLocation(), GetLocationOrDestination(MoveRequest));

	bool path = nav->TestPathSync(query, EPathFindingMode::Hierarchical);

	if (path)
		return true;

	return false;
}

void ADiplosimAIController::AIMoveTo(AActor* Actor)
{
	if (Actor->IsValidLowLevelFast() && CanMoveTo(Actor)) {
		MoveToLocation(GetLocationOrDestination(MoveRequest));

		if (GetOwner()->IsA<ACitizen>()) {
			ACitizen* citizen = Cast<ACitizen>(GetOwner());

			if (citizen->Building.BuildingAt != nullptr && citizen->Building.BuildingAt != Actor) {
				citizen->Building.BuildingAt->Leave(citizen);
			}
		}
	}
}