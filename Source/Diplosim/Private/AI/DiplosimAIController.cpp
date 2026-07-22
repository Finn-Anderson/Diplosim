#include "AI/DiplosimAIController.h"

#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "NavMesh/NavMeshPath.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"

#include "AI/AI.h"
#include "AI/Enemy.h"
#include "AI/AIMovementComponent.h"
#include "AI/Citizen/Citizen.h"
#include "AI/Citizen/Components/BuildingComponent.h"
#include "Buildings/Building.h"
#include "Buildings/House.h"
#include "Buildings/Misc/Broch.h"
#include "Buildings/Misc/Road.h"
#include "Buildings/Misc/Portal.h"
#include "Buildings/Work/Work.h"
#include "Buildings/Work/Production/ExternalProduction.h"
#include "Buildings/Work/Production/InternalProduction.h"
#include "Buildings/Work/Service/School.h"
#include "Buildings/Work/Service/Orphanage.h"
#include "Map/Grid.h"
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
#include "Universal/AttackComponent.h"

UDiplosimAIController::UDiplosimAIController()
{
	PrimaryComponentTick.bCanEverTick = false;

	Camera = nullptr;
}

void UDiplosimAIController::DefaultAction()
{
	if (!IsValid(AI) || AI->HealthComponent->GetHealth() == 0)
		return;

	if (AI->IsA<ACitizen>()) {
		ACitizen* citizen = Cast<ACitizen>(AI);
		FFactionStruct* faction = Camera->ConquestManager->GetFaction("", citizen);

		if (faction->Police.Arrested.Contains(citizen))
			Idle(faction, citizen);

		for (FArmyStruct army : faction->Armies)
			if (army.Citizens.Contains(citizen))
				return;

		if (IsValid(citizen->BuildingComponent->Employment) && citizen->BuildingComponent->Employment->bEmergency) {
			AIMoveTo(citizen->BuildingComponent->Employment);
		}
		else if (!GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>()->Enemies.IsEmpty()) {
			ERaidPolicy raidPolicy = Camera->PoliticsManager->GetRaidPolicyStatus(citizen);

			if (raidPolicy != ERaidPolicy::Default) {
				if (raidPolicy == ERaidPolicy::Home && !faction->BuildingsOnFire.Contains(citizen->BuildingComponent->House))
					AIMoveTo(citizen->BuildingComponent->House);
				else
					Wander(faction->EggTimer->GetActorLocation(), true, nullptr, 500.0f, true);

				return;
			}
		}

		if (!citizen->AttackComponent->OverlappingEnemies.IsEmpty() || Camera->EventsManager->IsAttendingEvent(citizen))
			return;

		for (auto& element : Camera->EventsManager->OngoingEvents()) {
			if (element.Key->Name != faction->Name)
				continue;

			for (FEventStruct* event : element.Value) {
				if (!event->Whitelist.IsEmpty() && !event->Whitelist.Contains(citizen))
					continue;

				Camera->EventsManager->GotoEvent(citizen, event);

				if (Camera->EventsManager->IsAttendingEvent(citizen))
					return;
			}

			break;
		}

		if (IsValid(citizen->BuildingComponent->Employment) && citizen->BuildingComponent->Employment->IsWorking(citizen) && !faction->BuildingsOnFire.Contains(citizen->BuildingComponent->Employment))
			AIMoveTo(citizen->BuildingComponent->Employment);
		else if (IsValid(citizen->BuildingComponent->School) && citizen->BuildingComponent->School->IsWorking(citizen->BuildingComponent->School->GetOccupant(citizen)) && !faction->BuildingsOnFire.Contains(citizen->BuildingComponent->School))
			AIMoveTo(citizen->BuildingComponent->School);
		else
			Idle(faction, citizen);
	}
	else if (AI->IsA<AEnemy>()) {
		if (Cast<AEnemy>(AI)->SpawnLocation != FVector::Zero())
			Wander(Cast<AEnemy>(AI)->SpawnLocation, true);
		else
			AI->MoveToBroch();
	}
}

void UDiplosimAIController::Idle(FFactionStruct* Faction, ACitizen* Citizen)
{
	if (!IsValid(Citizen))
		return;

	int32 chance = Camera->Stream.RandRange(0, 100);

	int32 hoursLeft = Citizen->HoursSleptToday.Num();

	ABuilding* house = Citizen->BuildingComponent->House;

	if (IsValid(house) && (hoursLeft - 1 <= Citizen->IdealHoursSlept || chance < 30 || Citizen->Energy < 100) && !Faction->BuildingsOnFire.Contains(house))
		AIMoveTo(house);
	else {
		int32 time = Camera->Stream.RandRange(5, 20);
		ABuilding* building = nullptr;
		bool bArrested = Faction->Police.Arrested.Contains(Citizen);

		if (bArrested)
			building = Citizen->BuildingComponent->BuildingAt;
		else if (chance < 32)
			building = ChooseIdleBuilding(Citizen);

		if (!bArrested && IsValid(building) && building->bHideCitizen && !Faction->BuildingsOnFire.Contains(building)) {
			AIMoveTo(building);

			time = Camera->Stream.RandRange(20, 60);
		}
		else
			Wander(Faction->EggTimer->GetActorLocation(), false, building);

		Camera->TimerManager->CreateTimer("Idle", AI, time, "DefaultAction", {}, false);
	}
}

void UDiplosimAIController::Wander(FVector CentrePoint, bool bTimer, ABuilding* Building, float MaxLength, bool bRaid)
{
	MoveRequest.SetGoalActor(nullptr);

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* navData = nav->GetDefaultNavDataInstance();

	int32 innerRange = 200;
	int32 outerRange = 1000;

	if (!bRaid && IsValid(Building)) {
		FVector size = Building->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize();

		if (AI->IsA<ACitizen>() && Cast<ACitizen>(AI)->BuildingComponent->House == Building) {
			if (size.X > size.Y)
				innerRange = size.X;
			else
				innerRange = size.Y;
		}
		else {
			innerRange = 0;

			if (size.X > size.Y)
				outerRange = size.X;
			else
				outerRange = size.Y;
		}

		CentrePoint = Building->GetActorLocation();
	}

	int32 attempts = 0;

	while (attempts < 5) {
		FVector location = CentrePoint + FRotator(0.0f, Camera->Stream.RandRange(0, 360), 0.0f).Vector() * Camera->Stream.RandRange(innerRange, outerRange);

		FNavLocation navLoc;
		nav->ProjectPointToNavigation(location, navLoc, FVector(1.0f, 1.0f, 200.0f));

		double length = 0.0f;
		ENavigationQueryResult::Type result = nav->GetPathLength(GetActualLocation(AI), navLoc, length);

		if (AI->IsA<ACitizen>()) {
			ACitizen* citizen = Cast<ACitizen>(AI);
			FFactionStruct* faction = Camera->ConquestManager->GetFaction("", citizen);

			double nearLength = 1000000.0f;
			double farLength = nearLength;
			double testLength = 0.0f;

			for (ABuilding* building : faction->Buildings) {
				if (!building->IsA<APortal>() || building->HealthComponent->GetHealth() == 0)
					continue;

				FNavLocation navBuildingLoc;
				nav->ProjectPointToNavigation(GetActualLocation(building), navBuildingLoc, FVector(100.0f, 100.0f, 200.0f));

				FNavLocation navAILoc;
				nav->ProjectPointToNavigation(GetActualLocation(AI), navAILoc, FVector(100.0f, 100.0f, 200.0f));

				nav->GetPathLength(navAILoc, navBuildingLoc, testLength);
				if (testLength != 0.0f && testLength < nearLength)
					nearLength = testLength;

				nav->GetPathLength(navLoc, navBuildingLoc, testLength);
				if (testLength != 0.0f && testLength < farLength)
					farLength = testLength;
			}

			if (length > nearLength + farLength) {
				length = nearLength + farLength;
				result = ENavigationQueryResult::Success;
			}
		}

		if (result == ENavigationQueryResult::Success && length <= MaxLength) {
			AIMoveTo(nullptr, navLoc.Location);

			break;
		}

		attempts++;
	}

	if (!bTimer)
		return;

	Camera->TimerManager->CreateTimer("Wander", AI, Camera->Stream.RandRange(5, 20), "DefaultAction", {}, false);
}

ABuilding* UDiplosimAIController::ChooseIdleBuilding(ACitizen* Citizen)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", Citizen);

	if (faction->Police.Arrested.Contains(Citizen))
		return nullptr;

	TArray<ABuilding*> buildings;
	buildings.Add(nullptr);

	for (int32 i = 0; i < 3; i++)
		buildings.Add(Citizen->BuildingComponent->House);

	for (ABuilding* building : faction->Buildings) {
		if (!IsValid(building) || building->IsA<ARoad>() || (building->IsA<AHouse>() && building->Inside.IsEmpty()) || !CanMoveTo(building->GetActorLocation()))
			continue;

		buildings.Add(building);
	}

	int32 index = Camera->Stream.RandRange(0, buildings.Num() - 1);

	return buildings[index];
}

double UDiplosimAIController::GetClosestActor(float Range, FVector TargetLocation, FVector CurrentLocation, FVector NewLocation, bool bProjectLocation, int32 CurrentValue, int32 NewValue)
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

void UDiplosimAIController::GetGatherSite(TSubclassOf<AResource> Resource)
{
	TMap<TSubclassOf<class ABuilding>, int32> buildings = Camera->ResourceManager->GetBuildings(Resource);

	ABuilding* target = nullptr;
	int32 i = INDEX_NONE;

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
				i = index;

				continue;
			}

			int32 targetIndex = target->Storage.Find(itemStruct);

			double magnitude = GetClosestActor(400.0f, GetActualLocation(AI), target->GetActorLocation(), building->GetActorLocation(), true, target->Storage[targetIndex].Amount, building->Storage[index].Amount);

			if (magnitude <= 0.0f)
				continue;

			target = building;
			i = index;
		}
	}

	if (target != nullptr)
		AIMoveTo(target, FVector::Zero(), i);
	else {
		TArray<FTimerParameterStruct> params;
		Camera->TimerManager->SetParameter(Resource, params);
		Camera->TimerManager->CreateTimer("FindGatherSite", AI, 10.0f, "GetGatherSite", params, false);
	}
}

bool UDiplosimAIController::CanMoveTo(FVector Location, AActor* Target, bool bCheckForPortals, FVector SecondLocation)
{
	if (!IsValid(Target))
		Target = AI;

	if (!IsValid(AI))
		return false;

	if (IsValid(Target)) {
		UHealthComponent* healthComp = Target->GetComponentByClass<UHealthComponent>();

		if (healthComp && healthComp->GetHealth() == 0)
			return false;
	}

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* navData = nav->GetDefaultNavDataInstance();

	FNavLocation targetLoc;
	nav->ProjectPointToNavigation(Location, targetLoc, FVector(400.0f, 400.0f, 40.0f));

	if (SecondLocation == FVector::Zero())
		SecondLocation = GetActualLocation(Target);

	FNavLocation ownerLoc;
	nav->ProjectPointToNavigation(SecondLocation, ownerLoc, FVector(400.0f, 400.0f, 40.0f));

	FPathFindingQuery query(AI, *navData, ownerLoc.Location, targetLoc.Location);

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
			nav->ProjectPointToNavigation(building->GetActorLocation(), buildingLoc, FVector(400.0f, 400.0f, 40.0f));

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

TArray<FVector> UDiplosimAIController::GetPathPoints(FVector StartLocation, FVector EndLocation)
{
	TSubclassOf<UNavigationQueryFilter> filter = nullptr;

	if (IsValid(AI) && AI->NavQueryFilter != nullptr)
		filter = AI->NavQueryFilter;

	if (AI->IsA<ACitizen>() && Cast<ACitizen>(AI)->BuildingComponent->EnterLocation != FVector::Zero())
		StartLocation = Cast<ACitizen>(AI)->BuildingComponent->EnterLocation;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	UNavigationPath* path = nav->FindPathToLocationSynchronously(GetWorld(), StartLocation, EndLocation, AI, filter);

	if (path == nullptr)
		return {};

	path->EnableRecalculationOnInvalidation(ENavigationOptionFlag::Disable);

	return path->PathPoints;
}

void UDiplosimAIController::AIMoveTo(AActor* Actor, FVector Location, int32 Instance)
{
	if (!IsValid(AI) || (!IsValid(Actor) && Location == FVector::Zero()))
		return;

	MoveRequest.SetGoalActor(Actor);
	MoveRequest.SetGoalInstance(Instance);

	if (Location != FVector::Zero())
		MoveRequest.SetLocation(Location);
	else
		MoveRequest.SetLocation(GetActualLocation(Actor));

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	FNavLocation navLoc;
	nav->ProjectPointToNavigation(MoveRequest.GetLocation(), navLoc, FVector(100.0f, 100.0f, 200.0f));
	MoveRequest.SetLocation(navLoc);
	
	if (AI->CanReach(Actor, AI->GetReach(), MoveRequest.GetLocation()) || (IsValid(Actor) && AI->IsA<ACitizen>() && Cast<ACitizen>(AI)->BuildingComponent->BuildingAt == Actor))
		return;

	FNavLocation navAILoc;
	nav->ProjectPointToNavigation(GetActualLocation(AI), navAILoc, FVector(400.0f, 400.0f, 40.0f));

	if (AI->IsA<ACitizen>()) {
		ACitizen* citizen = Cast<ACitizen>(AI);
		FFactionStruct* faction = Camera->ConquestManager->GetFaction("", citizen);

		ABuilding* ownerNearestPortal = nullptr;
		ABuilding* targetNearestPortal = nullptr;

		for (ABuilding* building : faction->Buildings) {
			if (!building->IsA<APortal>() || building->HealthComponent->GetHealth() == 0)
				continue;

			FNavLocation navBuildingLoc;
			nav->ProjectPointToNavigation(GetActualLocation(building), navBuildingLoc, FVector(100.0f, 100.0f, 200.0f));

			double length1 = 0.0f;
			double length2 = 0.0f;

			if (!IsValid(ownerNearestPortal))
				ownerNearestPortal = building;
			else {
				FNavLocation navPortalLoc;
				nav->ProjectPointToNavigation(GetActualLocation(ownerNearestPortal), navPortalLoc, FVector(100.0f, 100.0f, 200.0f));

				nav->GetPathLength(navAILoc, navPortalLoc, length1);
				nav->GetPathLength(navAILoc, navBuildingLoc, length2);

				if (length2 != 0.0f && (length2 < length1 || length1 == 0.0f))
					ownerNearestPortal = building;
			}
			
			if (!IsValid(targetNearestPortal))
				targetNearestPortal = building;
			else {
				FNavLocation navPortalLoc;
				nav->ProjectPointToNavigation(GetActualLocation(targetNearestPortal), navPortalLoc, FVector(100.0f, 100.0f, 200.0f));

				nav->GetPathLength(MoveRequest.GetLocation(), navPortalLoc, length1);
				nav->GetPathLength(MoveRequest.GetLocation(), navBuildingLoc, length2);

				if (length2 != 0.0f && (length2 < length1 || length1 == 0.0f))
					targetNearestPortal = building;
			}
		}

		if (IsValid(ownerNearestPortal) && IsValid(targetNearestPortal) && ownerNearestPortal != targetNearestPortal) {
			double originalPathLength = 0.0f;
			nav->GetPathLength(GetActualLocation(AI), MoveRequest.GetLocation(), originalPathLength);

			double ownerPathLength = 0.0f;
			nav->GetPathLength(ownerNearestPortal->GetActorLocation(), GetActualLocation(AI), ownerPathLength);

			double targetPathLength = 0.0f;
			nav->GetPathLength(targetNearestPortal->GetActorLocation(), MoveRequest.GetLocation(), targetPathLength);

			if (originalPathLength > ownerPathLength + targetPathLength) {
				MoveRequest.SetUltimateLocation(MoveRequest.GetLocation());
				MoveRequest.SetGoalActor(ownerNearestPortal, targetNearestPortal, Actor);
				MoveRequest.SetLocation(GetActualLocation(ownerNearestPortal));

				nav->ProjectPointToNavigation(MoveRequest.GetLocation(), navLoc, FVector(100.0f, 100.0f, 200.0f));
				MoveRequest.SetLocation(navLoc);
			}
		}
	}

	if (IsInGameThread())
		SetNewMovementPath(Actor, navAILoc);
	else
		Async(EAsyncExecution::TaskGraphMainTick, [this, Actor, navAILoc]() { SetNewMovementPath(Actor, navAILoc); });
}

void UDiplosimAIController::SetNewMovementPath(AActor* Actor, FNavLocation NavLocation)
{
	if (!IsValid(AI))
		return;

	TArray<FVector> points = GetPathPoints(NavLocation, MoveRequest.GetLocation());
	AI->MovementComponent->SetPoints(points);

	if (IsValid(Actor))
		ClearMovementTimers();

	if (!AI->IsA<ACitizen>())
		return;

	ACitizen* citizen = Cast<ACitizen>(AI);

	if (IsValid(citizen->BuildingComponent->BuildingAt) && citizen->BuildingComponent->BuildingAt != Actor && !citizen->Camera->PoliceManager->IsInJail(citizen))
		citizen->BuildingComponent->BuildingAt->Leave(citizen);
}

void UDiplosimAIController::RecalculateMovement(AActor* Actor)
{
	if (!IsValid(AI) || !IsValid(Actor) || Actor->IsA<AGrid>() || (Actor->IsA<AResource>() && MoveRequest.GetGoalInstance() == INDEX_NONE) || (AI->IsA<ACitizen>() && Cast<ACitizen>(AI)->BuildingComponent->BuildingAt == Actor))
		return;

	if (Actor->IsA<ABuilding>() && Cast<ABuilding>(Actor)->HealthComponent->GetHealth() == 0 && AI->AttackComponent->OverlappingEnemies.IsEmpty()) {
		if (!Camera->TimerManager->DoesTimerExist("Wander", AI) && !Camera->TimerManager->DoesTimerExist("Idle", AI))
			DefaultAction();

		return;
	}

	UHealthComponent* healthComp = Actor->FindComponentByClass<UHealthComponent>();

	if (healthComp && healthComp->GetHealth() == 0) {
		DefaultAction();

		return;
	}

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	FVector targetLoc = GetActualLocation(Actor);
	FVector currentLoc = GetActualLocation(AI);

	if (!AI->MovementComponent->TempPoints.IsEmpty())
		currentLoc = AI->MovementComponent->TempPoints.Last();
	else if (!AI->MovementComponent->Points.IsEmpty())
		currentLoc = AI->MovementComponent->Points.Last();

	FNavLocation navLoc;
	nav->ProjectPointToNavigation(targetLoc, navLoc, FVector(200.0f, 200.0f, 30.0f));

	if (FVector::Dist(currentLoc, navLoc) < AI->GetReach())
		return;

	if (Actor->IsA<ABroch>())
		AI->MoveToBroch();
	else
		AIMoveTo(Actor, navLoc.Location, MoveRequest.GetGoalInstance());
}

FVector UDiplosimAIController::GetActualLocation(AActor* Actor)
{
	FVector location = Actor->GetActorLocation();

	if (Actor->IsA<AAI>()) {
		location = Cast<AAI>(Actor)->MovementComponent->Transform.GetLocation();
	}
	if (Actor->IsA<ABuilding>()) {
		UStaticMeshComponent* comp = Cast<UStaticMeshComponent>(Actor->GetRootComponent());

		if (comp && comp->DoesSocketExist("Entrance"))
			location = comp->GetSocketLocation("Entrance");
		else if (IsValid(AI))
			comp->GetClosestPointOnCollision(AI->MovementComponent->Transform.GetLocation(), location); 
	}
	else if (Actor->IsA<AResource>()) {
		FTransform transform;
		Cast<AResource>(Actor)->ResourceHISM->GetInstanceTransform(MoveRequest.GetGoalInstance(), transform);

		location = transform.GetLocation();
	}

	return location;
}

void UDiplosimAIController::StartMovement()
{
	if (!IsValid(AI) || (AI->MovementComponent->Points.IsEmpty() && !AI->MovementComponent->bSetPoints))
		return;
	
	AI->MovementComponent->SetAnimation(EAnim::Move, true, 6.0f);

	if (AI->IsA<ACitizen>() && IsValid(MoveRequest.GetGoalActor())) {
		Camera->TimerManager->PauseTimer("Idle", AI, false);
		Camera->TimerManager->PauseTimer("Wander", AI, false);
		Camera->TimerManager->PauseTimer("Harvest", AI, false);
	}
}

void UDiplosimAIController::StopMovement()
{
	if (!IsValid(AI))
		return;

	AI->MovementComponent->SetAnimation(EAnim::Still);

	if (AI->IsA<ACitizen>() && !AI->MovementComponent->Points.IsEmpty() && IsValid(MoveRequest.GetGoalActor())) {
		Camera->TimerManager->PauseTimer("Idle", AI, true);
		Camera->TimerManager->PauseTimer("Wander", AI, true);
		Camera->TimerManager->PauseTimer("Harvest", AI, true);
	}
}

void UDiplosimAIController::ClearMovementTimers()
{
	ACitizen* citizen = Cast<ACitizen>(AI);

	Camera->TimerManager->RemoveTimer("Idle", citizen);
	Camera->TimerManager->RemoveTimer("Wander", citizen);
	Camera->TimerManager->RemoveTimer("Harvest", citizen);
}