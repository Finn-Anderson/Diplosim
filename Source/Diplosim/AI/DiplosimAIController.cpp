#include "AI/DiplosimAIController.h"

#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "NavFilters/NavigationQueryFilter.h"

#include "AI/AI.h"
#include "AI/Citizen.h"
#include "AI/Enemy.h"
#include "AI/AttackComponent.h"
#include "AI/AIMovementComponent.h"
#include "BuildingComponent.h"
#include "Buildings/Building.h"
#include "Buildings/House.h"
#include "Buildings/Work/Work.h"
#include "Buildings/Work/Service/School.h"
#include "Buildings/Work/Service/Orphanage.h"
#include "Buildings/Misc/Broch.h"
#include "Buildings/Misc/Road.h"
#include "Buildings/Misc/Portal.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/DiplosimTimerManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/EventsManager.h"
#include "Player/Managers/PoliticsManager.h"
#include "Player/Managers/PoliceManager.h"
#include "Universal/HealthComponent.h"
#include "Universal/Resource.h"
#include "Universal/DiplosimGameModeBase.h"

ADiplosimAIController::ADiplosimAIController()
{
	PrimaryActorTick.bCanEverTick = false;

	Camera = nullptr;
}

void ADiplosimAIController::DefaultAction()
{
	if (!IsValid(AI) || AI->HealthComponent->GetHealth() == 0)
		return;

	if (AI->IsA<ACitizen>()) {
		ACitizen* citizen = Cast<ACitizen>(AI);
		FFactionStruct* faction = Camera->ConquestManager->GetFaction("", citizen);

		for (FArmyStruct army : faction->Armies)
			if (army.Citizens.Contains(citizen))
				return;

		if (citizen->BuildingComponent->Employment != nullptr && citizen->BuildingComponent->Employment->bEmergency) {
			AIMoveTo(citizen->BuildingComponent->Employment);
		}
		else if (!GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>()->Enemies.IsEmpty()) {
			ERaidPolicy raidPolicy = Camera->PoliticsManager->GetRaidPolicyStatus(citizen);

			if (raidPolicy != ERaidPolicy::Default) {
				if (raidPolicy == ERaidPolicy::Home)
					AIMoveTo(citizen->BuildingComponent->House);
				else
					AIMoveTo(faction->EggTimer);

				return;
			}
		}

		if (citizen->bConversing || Camera->PoliceManager->IsInAPoliceReport(citizen, faction) || citizen->AttackComponent->IsComponentTickEnabled() || Camera->EventsManager->IsAttendingEvent(citizen))
			return;

		for (auto& element : Camera->EventsManager->OngoingEvents()) {
			if (element.Key->Name != faction->Name)
				continue;

			for (FEventStruct* event : element.Value) {
				Camera->EventsManager->GotoEvent(citizen, event);

				if (Camera->EventsManager->IsAttendingEvent(citizen))
					return;
			}

			break;
		}

		if (IsValid(citizen->BuildingComponent->Employment) && citizen->BuildingComponent->Employment->IsWorking(citizen)) {
			if (IsValid(MoveRequest.GetGoalActor()) && MoveRequest.GetGoalActor()->IsA<AResource>())
				StartMovement();
			else
				AIMoveTo(citizen->BuildingComponent->Employment);
		}
		else if (IsValid(citizen->BuildingComponent->School) && citizen->BuildingComponent->School->IsWorking(citizen->BuildingComponent->School->GetOccupant(citizen)))
			AIMoveTo(citizen->BuildingComponent->School);
		else
			Idle(faction, citizen);
	}
	else {
		if (AI->IsA<AEnemy>() && Cast<AEnemy>(AI)->SpawnLocation != FVector::Zero())
			Wander(Cast<AEnemy>(AI)->SpawnLocation, true);
		else
			AI->MoveToBroch();
	}
}

void ADiplosimAIController::Idle(FFactionStruct* Faction, ACitizen* Citizen)
{
	if (!IsValid(Citizen))
		return;

	MoveRequest.SetGoalActor(nullptr);

	int32 chance = Camera->Stream.RandRange(0, 100);

	int32 hoursLeft = Citizen->HoursSleptToday.Num();

	AHouse* house = Citizen->BuildingComponent->House;

	if (IsValid(house) && (hoursLeft - 1 <= Citizen->IdealHoursSlept || chance < 33))
		AIMoveTo(house);
	else {
		int32 time = Camera->Stream.RandRange(5, 20);

		if (IsValid(ChosenBuilding) && ChosenBuilding->bHideCitizen && chance < 66 && !Faction->Police.Arrested.Contains(Citizen)) {
			AIMoveTo(ChosenBuilding);

			time = 60.0f;
		}
		else {
			Wander(Faction->EggTimer->GetActorLocation(), false);
		}

		Camera->TimerManager->CreateTimer("Idle", Citizen, time, "DefaultAction", {}, false);
	}
}

void ADiplosimAIController::Wander(FVector CentrePoint, bool bTimer)
{
	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* navData = nav->GetDefaultNavDataInstance();

	int32 innerRange = 200;
	int32 outerRange = 1000;

	if (IsValid(ChosenBuilding) && !ChosenBuilding->IsA<ABroch>()) {
		FVector size = ChosenBuilding->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize();

		innerRange = 0;

		if (size.X > size.Y)
			outerRange = size.X;
		else
			outerRange = size.Y;

		CentrePoint = ChosenBuilding->GetActorLocation();
	}

	FVector location = CentrePoint + Async(EAsyncExecution::TaskGraph, [this, innerRange, outerRange]() { return FRotator(0.0f, Camera->Stream.RandRange(0, 360), 0.0f).Vector() * Camera->Stream.RandRange(innerRange, outerRange); }).Get();

	FNavLocation navLoc;
	nav->ProjectPointToNavigation(location, navLoc, FVector(1.0f, 1.0f, 200.0f));

	double length = 0.0f;
	nav->GetPathLength(Camera->GetTargetActorLocation(AI), navLoc, length);

	if (length < 5000.0f && CanMoveTo(navLoc)) {
		UNavigationPath* path = nav->FindPathToLocationSynchronously(GetWorld(), Camera->GetTargetActorLocation(AI), navLoc, AI, AI->NavQueryFilter);

		AI->MovementComponent->SetPoints(path->PathPoints);

		if (AI->IsA<ACitizen>() && IsValid(Cast<ACitizen>(AI)->BuildingComponent->BuildingAt)) {
			ACitizen* citizen = Cast<ACitizen>(AI);

			Async(EAsyncExecution::TaskGraphMainTick, [citizen]() { citizen->BuildingComponent->BuildingAt->Leave(citizen); });
		}
	}

	if (!bTimer)
		return;

	Camera->TimerManager->CreateTimer("Wander", AI, Camera->Stream.RandRange(5, 20), "DefaultAction", {}, false);
}

void ADiplosimAIController::ChooseIdleBuilding(ACitizen* Citizen)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", Citizen);

	if (faction->Police.Arrested.Contains(Citizen))
		return;

	TArray<ABuilding*> buildings;

	for (ABuilding* building : faction->Buildings) {
		if (!IsValid(building) || (building->IsA<AWork>() && !building->IsA<AOrphanage>()) || building->IsA<ARoad>() || (building->IsA<AHouse>() && building->Inside.IsEmpty()) || !CanMoveTo(building->GetActorLocation()))
			continue;

		buildings.Add(building);
	}

	if (buildings.IsEmpty()) {
		ChosenBuilding = nullptr;

		return;
	}

	int32 index = Camera->Stream.RandRange(0, buildings.Num() - 1);

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
	TMap<TSubclassOf<class ABuilding>, int32> buildings = Camera->ResourceManager->GetBuildings(Resource);

	ABuilding* target = nullptr;

	for (auto& element : buildings) {
		TArray<ABuilding*> foundBuildings = Camera->ResourceManager->GetBuildingsOfClass(Camera->ConquestManager->GetFaction("", AI), element.Key);

		for (ABuilding* building : foundBuildings) {
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
	else {
		TArray<FTimerParameterStruct> params;
		Camera->TimerManager->SetParameter(Resource, params);
		Camera->TimerManager->CreateTimer("FindGatherSite", AI, 30.0f, "GetGatherSite", params, false);
	}
}

bool ADiplosimAIController::CanMoveTo(FVector Location, AActor* Target, bool bCheckForPortals)
{
	if (!IsValid(Target))
		Target = AI;

	if (!IsValid(AI) || !IsValid(Target) || !IsValid(GetWorld()))
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
		FFactionStruct* faction = Camera->ConquestManager->GetFaction("", citizen);

		bool targetCanReachPortal = false;
		bool ownerCanReachPortal = false;

		for (ABuilding* building : faction->Buildings) {
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

	if (AI->IsA<ACitizen>() && Cast<ACitizen>(AI)->BuildingComponent->BuildingAt != nullptr)
		StartLocation = Cast<ACitizen>(AI)->BuildingComponent->EnterLocation;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	UNavigationPath* path = nav->FindPathToLocationSynchronously(GetWorld(), StartLocation, EndLocation, AI, filter);

	return path->PathPoints;
}

void ADiplosimAIController::AIMoveTo(AActor* Actor, FVector Location, int32 Instance)
{
	if (!IsValid(AI) || !IsValid(Actor))
		return;
	
	int32 reach = AI->Range / 15.0f;
	
	if ((IsValid(MoveRequest.GetGoalActor()) && AI->CanReach(MoveRequest.GetGoalActor(), reach)) || (AI->IsA<ACitizen>() && Cast<ACitizen>(AI)->BuildingComponent->BuildingAt == Actor))
		return;

	StartMovement();

	MoveRequest.SetGoalActor(Actor);
	MoveRequest.SetGoalInstance(Instance);
	MoveRequest.SetLocation(Camera->GetTargetActorLocation(Actor));

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	if (AI->IsA<ACitizen>()) {
		ACitizen* citizen = Cast<ACitizen>(AI);
		FFactionStruct* faction = Camera->ConquestManager->GetFaction("", citizen);

		ABuilding* ownerNearestPortal = nullptr;
		ABuilding* targetNearestPortal = nullptr;

		for (ABuilding* building : faction->Buildings) {
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

	if (!AI->IsA<ACitizen>() || Cast<ACitizen>(AI)->BuildingComponent->BuildingAt == Actor)
		return;

	ACitizen* citizen = Cast<ACitizen>(AI);

	Camera->TimerManager->RemoveTimer("Idle", citizen);

	if (citizen->BuildingComponent->BuildingAt != nullptr)
		Async(EAsyncExecution::TaskGraphMainTick, [citizen]() { citizen->BuildingComponent->BuildingAt->Leave(citizen); });
}

void ADiplosimAIController::RecalculateMovement(AActor* Actor)
{
	if (!IsValid(Actor) || !IsValid(AI))
		return;

	FVector targetLoc = Camera->GetTargetActorLocation(Actor);
	FVector currentLoc = Camera->GetTargetActorLocation(AI);
	UStaticMeshComponent* mesh = Actor->GetComponentByClass<UStaticMeshComponent>();

	if (!AI->MovementComponent->Points.IsEmpty())
		currentLoc = AI->MovementComponent->Points.Last();

	if (mesh)
		mesh->GetClosestPointOnCollision(currentLoc, targetLoc);

	if (FVector::Dist(currentLoc, targetLoc) < AI->Range)
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

	AI->MovementComponent->SetAnimation(EAnim::Still);
}