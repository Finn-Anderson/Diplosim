#include "AI/DiplosimAIController.h"

#include "Navigation/CrowdFollowingComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Kismet/GameplayStatics.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

#include "AI.h"
#include "Citizen.h"
#include "Enemy.h"
#include "Universal/HealthComponent.h"
#include "Buildings/Building.h"
#include "Buildings/House.h"
#include "Buildings/Work/Work.h"
#include "Buildings/Work/Service/School.h"
#include "Buildings/Work/Service/Orphanage.h"
#include "Buildings/Misc/Broch.h"
#include "Buildings/Misc/Road.h"
#include "Buildings/Misc/Portal.h"
#include "Universal/DiplosimGameModeBase.h"
#include "Universal/Resource.h"
#include "AttackComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/ConquestManager.h"
#include "AIMovementComponent.h"
#include "Map/Grid.h"

ADiplosimAIController::ADiplosimAIController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	PrimaryActorTick.bCanEverTick = false;

	GetPathFollowingComponent()->PrimaryComponentTick.bCanEverTick = false;

	Camera = nullptr;
}

void ADiplosimAIController::DefaultAction()
{
	if (!IsValid(AI) || AI->HealthComponent->GetHealth() == 0)
		return;

	if (AI->IsA<ACitizen>() && Camera->CitizenManager->Citizens.Contains(AI)) {
		ACitizen* citizen = Cast<ACitizen>(AI);

		if (citizen->Building.Employment != nullptr && citizen->Building.Employment->bEmergency) {
			AIMoveTo(citizen->Building.Employment);
		}
		else if (!Camera->CitizenManager->Enemies.IsEmpty() && Camera->CitizenManager->GetRaidPolicyStatus() != ERaidPolicy::Default) {
			ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());

			if (Camera->CitizenManager->GetRaidPolicyStatus() == ERaidPolicy::Home)
				AIMoveTo(citizen->Building.House);
			else
				AIMoveTo(gamemode->Broch);

			return;
		}

		if (citizen->bConversing || Camera->CitizenManager->IsInAPoliceReport(citizen) || citizen->AttackComponent->IsComponentTickEnabled() || Camera->CitizenManager->IsAttendingEvent(citizen))
			return;

		for (FEventStruct event : Camera->CitizenManager->OngoingEvents()) {
			Camera->CitizenManager->GotoEvent(citizen, event);

			if (Camera->CitizenManager->IsAttendingEvent(citizen))
				return;
		}

		if (IsValid(citizen->Building.Employment) && citizen->Building.Employment->IsWorking(citizen)) {
			if (IsValid(MoveRequest.GetGoalActor()) && MoveRequest.GetGoalActor()->IsA<AResource>())
				StartMovement();
			else
				AIMoveTo(citizen->Building.Employment);
		}
		else if (IsValid(citizen->Building.School) && citizen->Building.School->IsWorking(citizen->Building.School->GetOccupant(citizen)))
			AIMoveTo(citizen->Building.School);
		else
			Idle(citizen);
	}
	else
		AI->MoveToBroch();
}

void ADiplosimAIController::Idle(ACitizen* Citizen)
{
	if (!IsValid(Citizen))
		return;

	MoveRequest.SetGoalActor(nullptr);

	int32 chance = Camera->Grid->Stream.RandRange(0, 100);

	int32 hoursLeft = Citizen->HoursSleptToday.Num();

	AHouse* house = Citizen->Building.House;

	if (IsValid(house) && (hoursLeft - 1 <= Citizen->IdealHoursSlept || chance < 33))
		AIMoveTo(house);
	else {
		int32 time = Camera->Grid->Stream.RandRange(5, 30);

		if (IsValid(ChosenBuilding) && ChosenBuilding->bHideCitizen && chance < 66 && !Camera->CitizenManager->Arrested.Contains(Citizen)) {
			AIMoveTo(ChosenBuilding);

			time = 60.0f;
		}
		else {
			UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
			const ANavigationData* navData = nav->GetDefaultNavDataInstance();

			int32 innerRange = 200;
			int32 outerRange = 1000;

			FVector location = Camera->CitizenManager->BrochLocation;
					
			if (IsValid(ChosenBuilding) && !ChosenBuilding->IsA<ABroch>()) {
				FVector size = ChosenBuilding->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize();

				innerRange = 0;

				if (size.X > size.Y)
					outerRange = size.X;
				else
					outerRange = size.Y;

				location = ChosenBuilding->GetActorLocation();
			}

			location += FRotator(0.0f, Camera->Grid->Stream.RandRange(0, 360), 0.0f).Vector() * Camera->Grid->Stream.RandRange(innerRange, outerRange);

			FNavLocation navLoc;
			nav->ProjectPointToNavigation(location, navLoc, FVector(1.0f, 1.0f, 200.0f));

			double length = 0.0f;
			nav->GetPathLength(Camera->GetTargetActorLocation(Citizen), navLoc, length);

			if (length < 5000.0f && CanMoveTo(navLoc)) {
				UNavigationPath* path = nav->FindPathToLocationSynchronously(GetWorld(), Camera->GetTargetActorLocation(Citizen), navLoc, Citizen, Citizen->NavQueryFilter);

				Citizen->MovementComponent->SetPoints(path->PathPoints);

				if (IsValid(Citizen->Building.BuildingAt))
					AsyncTask(ENamedThreads::GameThread, [Citizen]() { Citizen->Building.BuildingAt->Leave(Citizen); });
			}
		}

		Camera->CitizenManager->CreateTimer("Idle", Citizen, time, FTimerDelegate::CreateUObject(this, &ADiplosimAIController::DefaultAction), false);
	}
}

void ADiplosimAIController::ChooseIdleBuilding(ACitizen* Citizen)
{
	if (Camera->CitizenManager->Arrested.Contains(Citizen))
		return;

	TArray<ABuilding*> buildings;

	for (ABuilding* building : Camera->CitizenManager->Buildings) {
		if (!IsValid(building) || (building->IsA<AWork>() && !building->IsA<AOrphanage>()) || building->IsA<ARoad>() || (building->IsA<AHouse>() && building->Inside.IsEmpty()) || !CanMoveTo(building->GetActorLocation()))
			continue;

		buildings.Add(building);
	}

	if (buildings.IsEmpty()) {
		ChosenBuilding = nullptr;

		return;
	}

	int32 index = Camera->Grid->Stream.RandRange(0, buildings.Num() - 1);

	ChosenBuilding = buildings[index];
}

double ADiplosimAIController::GetClosestActor(float Range, FVector TargetLocation, FVector CurrentLocation, FVector NewLocation, bool bProjectLocation, int32 CurrentValue, int32 NewValue)
{
	if (!CanMoveTo(NewLocation))
		return -1000000.0f;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* navData = nav->GetDefaultNavDataInstance();

	if (bProjectLocation) {
		FNavLocation projectedTarget;
		nav->ProjectPointToNavigation(TargetLocation, projectedTarget, FVector(Range, Range, 20.0f));

		FNavLocation projectedCurrent;
		nav->ProjectPointToNavigation(CurrentLocation, projectedCurrent, FVector(Range, Range, 20.0f));

		FNavLocation projectedNew;
		nav->ProjectPointToNavigation(NewLocation, projectedNew, FVector(Range, Range, 20.0f));

		TargetLocation = projectedTarget.Location;
		CurrentLocation = projectedCurrent.Location;
		NewLocation = projectedNew.Location;
	}
	
	double curLength = 100000000000.0f;
	navData->CalcPathLength(TargetLocation, CurrentLocation, curLength);

	double newLength = 100000000000.0f;
	navData->CalcPathLength(TargetLocation, NewLocation, newLength);

	double magnitude = curLength / CurrentValue - newLength / NewValue;

	return magnitude;
}

void ADiplosimAIController::GetGatherSite(TSubclassOf<AResource> Resource)
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

			double magnitude = GetClosestActor(400.0f, Camera->GetTargetActorLocation(AI), target->GetActorLocation(), building->GetActorLocation(), true, target->Storage[targetIndex].Amount, building->Storage[index].Amount);

			if (magnitude <= 0.0f)
				continue;

			target = building;
		}
	}

	if (target != nullptr)
		AIMoveTo(target);
	else
		Camera->CitizenManager->CreateTimer("FindGatherSite", AI, 30.0f, FTimerDelegate::CreateUObject(this, &ADiplosimAIController::GetGatherSite, Resource), false);
}

bool ADiplosimAIController::CanMoveTo(FVector Location, AActor* Target, bool bCheckForPortals)
{
	if (!IsValid(Target))
		Target = AI;

	if (!IsValid(AI) || !IsValid(Target) || !IsValid(GetOwner()))
		return false;

	UHealthComponent* healthComp = Target->GetComponentByClass<UHealthComponent>();

	if (healthComp && healthComp->GetHealth() == 0)
		return false;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* navData = nav->GetDefaultNavDataInstance();

	FNavLocation targetLoc;
	nav->ProjectPointToNavigation(Location, targetLoc, FVector(400.0f, 400.0f, 20.0f));

	FNavLocation ownerLoc;
	nav->ProjectPointToNavigation(Camera->GetTargetActorLocation(Target), ownerLoc, FVector(400.0f, 400.0f, 20.0f));

	FPathFindingQuery query(Target, *navData, ownerLoc.Location, targetLoc.Location);

	bool path = nav->TestPathSync(query, EPathFindingMode::Hierarchical);

	if (path)
		return true;

	if (bCheckForPortals && AI->IsA<ACitizen>()) {
		ACitizen* citizen = Cast<ACitizen>(AI);

		bool targetCanReachPortal = false;
		bool ownerCanReachPortal = false;

		for (ABuilding* building : Camera->CitizenManager->Buildings) {
			if (!building->IsA<APortal>() || building->HealthComponent->GetHealth() == 0)
				continue;

			FNavLocation buildingLoc;
			nav->ProjectPointToNavigation(building->GetActorLocation(), buildingLoc, FVector(400.0f, 400.0f, 20.0f));

			FPathFindingQuery ownerQuery(AI, *navData, ownerLoc.Location, buildingLoc.Location);
			FPathFindingQuery targetQuery(AI, *navData, targetLoc.Location, buildingLoc.Location);

			bool ownerPath = nav->TestPathSync(ownerQuery, EPathFindingMode::Hierarchical);
			bool targetPath = nav->TestPathSync(targetQuery, EPathFindingMode::Hierarchical);

			if (ownerPath)
				ownerCanReachPortal = true;

			if (targetPath)
				targetCanReachPortal = true;

			if (targetCanReachPortal && ownerCanReachPortal)
				return true;
		}
	}

	return false;
}

TArray<FVector> ADiplosimAIController::GetPathPoints(FVector StartLocation, FVector EndLocation)
{
	TSubclassOf<UNavigationQueryFilter> filter = nullptr;

	if (IsValid(AI) && AI->NavQueryFilter != nullptr)
		filter = AI->NavQueryFilter;

	if (AI->IsA<ACitizen>() && Cast<ACitizen>(AI)->Building.BuildingAt != nullptr)
		StartLocation = Cast<ACitizen>(AI)->Building.EnterLocation;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	UNavigationPath* path = nav->FindPathToLocationSynchronously(GetWorld(), StartLocation, EndLocation, AI, filter);

	return path->PathPoints;
}

void ADiplosimAIController::AIMoveTo(AActor* Actor, FVector Location, int32 Instance)
{
	if (!IsValid(AI))
		return;
	
	int32 reach = AI->Range / 15.0f;
	
	if ((IsValid(MoveRequest.GetGoalActor()) && AI->CanReach(MoveRequest.GetGoalActor(), reach)) || (AI->IsA<ACitizen>() && Cast<ACitizen>(AI)->Building.BuildingAt == Actor))
		return;

	StartMovement();

	MoveRequest.SetGoalActor(Actor);
	MoveRequest.SetGoalInstance(Instance);
	MoveRequest.SetLocation(Camera->GetTargetActorLocation(Actor));

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	if (AI->IsA<ACitizen>()) {
		ACitizen* citizen = Cast<ACitizen>(AI);

		ABuilding* ownerNearestPortal = nullptr;
		ABuilding* targetNearestPortal = nullptr;

		for (ABuilding* building : Camera->CitizenManager->Buildings) {
			if (!building->IsA<APortal>() || building->HealthComponent->GetHealth() == 0)
				continue;

			if (!IsValid(ownerNearestPortal) && CanMoveTo(building->GetActorLocation(), AI, false))
				ownerNearestPortal = building;

			if (!IsValid(targetNearestPortal) && CanMoveTo(building->GetActorLocation(), Actor, false))
				targetNearestPortal = building;
		}

		if (IsValid(ownerNearestPortal) && IsValid(targetNearestPortal)) {
			double originalPathLength = 0.0f;
			auto originalResult = nav->GetPathLength(Camera->GetTargetActorLocation(AI), MoveRequest.GetLocation(), originalPathLength);

			double ownerPathLength = 0.0f;
			auto ownerResult = nav->GetPathLength(ownerNearestPortal->GetActorLocation(), MoveRequest.GetLocation(), ownerPathLength);

			double targetPathLength = 0.0f;
			auto targetResult = nav->GetPathLength(targetNearestPortal->GetActorLocation(), MoveRequest.GetLocation(), targetPathLength);

			if (ownerResult == ENavigationQueryResult::Success && targetResult == ENavigationQueryResult::Success && (originalResult != ENavigationQueryResult::Success || originalPathLength > ownerPathLength + targetPathLength))
				MoveRequest.SetGoalActor(ownerNearestPortal, targetNearestPortal, Actor);
		}
	}

	if (Location != FVector::Zero()) {
		MoveRequest.SetLocation(Location);
	}
	else if (Actor->IsA<ABuilding>()) {
		UStaticMeshComponent* comp = Actor->GetComponentByClass<UStaticMeshComponent>();

		if (comp && comp->DoesSocketExist("Entrance"))
			MoveRequest.SetLocation(comp->GetSocketLocation("Entrance"));
	}

	FNavLocation navLoc;
	nav->ProjectPointToNavigation(MoveRequest.GetLocation(), navLoc, FVector(400.0f, 400.0f, 40.0f));

	MoveRequest.SetLocation(navLoc.Location);

	TArray<FVector> points = GetPathPoints(Camera->GetTargetActorLocation(AI), MoveRequest.GetLocation());

	AI->MovementComponent->SetPoints(points);

	SetFocus(Actor);

	if (!AI->IsA<ACitizen>() || Cast<ACitizen>(AI)->Building.BuildingAt == Actor)
		return;

	ACitizen* citizen = Cast<ACitizen>(AI);

	Camera->CitizenManager->RemoveTimer("Idle", AI);

	if (citizen->Building.BuildingAt != nullptr)
		citizen->Building.BuildingAt->Leave(citizen);
}

void ADiplosimAIController::RecalculateMovement(AActor* Actor)
{
	if (!IsValid(Actor) || !IsValid(AI) || !IsValid(AI->MovementComponent) || MoveRequest.GetGoalActor() != Actor || (!AI->MovementComponent->Points.IsEmpty() && FVector::Dist(Camera->GetTargetActorLocation(Actor), AI->MovementComponent->Points.Last()) < 20.0f))
		return;

	AIMoveTo(Actor, MoveRequest.GetLocation(), MoveRequest.GetGoalInstance());
}

void ADiplosimAIController::StartMovement()
{
	if (!AI->MovementComponent->Points.IsEmpty())
		return;
	
	AI->MovementComponent->SetAnimation(EAnim::Move, true, 6.0f);
}

void ADiplosimAIController::StopMovement()
{
	if (!IsValid(AI))
		return;
	
	MoveRequest.SetGoalActor(nullptr);
	
	AI->MovementComponent->SetPoints({});
}