#include "AI/DiplosimAIController.h"

#include "Navigation/CrowdFollowingComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Kismet/GameplayStatics.h"

#include "AI.h"
#include "Citizen.h"
#include "HealthComponent.h"
#include "Buildings/Building.h"
#include "Buildings/Broch.h"
#include "Resource.h"
#include "AttackComponent.h"

ADiplosimAIController::ADiplosimAIController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickInterval(0.4f);

	PrevGoal = nullptr;
}

void ADiplosimAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MoveRequest.GetGoalActor() == nullptr || !MoveRequest.GetGoalActor()->IsValidLowLevelFast())
		return;

	TSubclassOf<UNavigationQueryFilter> filter = Cast<AAI>(GetOwner())->NavQueryFilter;

	if (MoveRequest.GetGoalActor()->IsA<AAI>() && !Cast<AAI>(GetOwner())->AttackComponent->MeleeableEnemies.Contains(MoveRequest.GetGoalActor()))
		MoveToActor(MoveRequest.GetGoalActor(), 1.0f, true, true, true, filter, true);
}

void ADiplosimAIController::DefaultAction()
{
	if (MoveRequest.GetGoalActor() == PrevGoal)
		return;

	MoveRequest.SetGoalActor(nullptr);

	if (PrevGoal == nullptr || !PrevGoal->IsValidLowLevelFast())
		Idle();
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

bool ADiplosimAIController::CanMoveTo(FVector Location)
{
	if (!GetOwner()->IsValidLowLevelFast())
		return false;

	UHealthComponent* healthComp = GetOwner()->GetComponentByClass<UHealthComponent>();

	if (healthComp->GetHealth() == 0)
		return false;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* navData = nav->GetDefaultNavDataInstance();

	FNavLocation loc;
	nav->ProjectPointToNavigation(Location, loc, FVector(200.0f, 200.0f, 10.0f));

	UNavigationPath* ResultPath = nav->FindPathToLocationSynchronously(GetWorld(), GetNavAgentLocation(), loc.Location, GetOwner());

	if (ResultPath->GetPath().IsValid())
		return true;

	return false;
}

void ADiplosimAIController::AIMoveTo(AActor* Actor, FVector Location, int32 Instance)
{
	if (Actor->IsA<AAI>() && (MoveRequest.GetGoalActor() == nullptr || !MoveRequest.GetGoalActor()->IsA<AAI>()))
		PrevGoal = MoveRequest.GetGoalActor();

	MoveRequest.SetGoalActor(Actor);
	MoveRequest.SetGoalInstance(Instance);
	MoveRequest.SetLocation(Actor->GetActorLocation());

	if (Location != FVector::Zero())
		MoveRequest.SetLocation(Location);

	if (Actor->IsA<ABuilding>()) {
		UStaticMeshComponent* comp = Actor->GetComponentByClass<UStaticMeshComponent>();

		if (comp && comp->DoesSocketExist("Entrance"))
			MoveRequest.SetLocation(comp->GetSocketLocation("Entrance"));
	}

	GetWorld()->GetTimerManager().ClearTimer(IdleTimer);

	TSubclassOf<UNavigationQueryFilter> filter = Cast<AAI>(GetOwner())->NavQueryFilter;

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