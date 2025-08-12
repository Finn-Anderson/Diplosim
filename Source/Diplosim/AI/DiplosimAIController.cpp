#include "AI/DiplosimAIController.h"

#include "Navigation/CrowdFollowingComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Kismet/GameplayStatics.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "Components/SphereComponent.h"

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
	if (GetOwner()->IsA<ACitizen>() && Camera->CitizenManager->Citizens.Contains(GetOwner())) {
		ACitizen* citizen = Cast<ACitizen>(GetOwner());

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

		if (citizen->bConversing || Camera->CitizenManager->IsInAPoliceReport(citizen) || (citizen->AttackComponent->IsComponentTickEnabled() && Camera->CitizenManager->Enemies.IsEmpty()))
			return;

		if (Camera->ConquestManager->IsCitizenMoving(citizen)) {
			AIMoveTo(Camera->ConquestManager->Portal);

			return;
		}

		if (Camera->CitizenManager->IsAttendingEvent(citizen))
			return;

		for (FEventStruct event : Camera->CitizenManager->OngoingEvents()) {
			Camera->CitizenManager->GotoEvent(citizen, event);

			if (Camera->CitizenManager->IsAttendingEvent(citizen))
				return;
		}

		if (citizen->Building.Employment != nullptr && citizen->Building.Employment->IsWorking(citizen)) {
			if (IsValid(MoveRequest.GetGoalActor()) && MoveRequest.GetGoalActor()->IsA<AResource>())
				StartMovement();
			else
				AIMoveTo(citizen->Building.Employment);
		}
		else if (citizen->Building.School != nullptr && citizen->Building.School->IsWorking(citizen->Building.School->GetOccupant(citizen)))
			AIMoveTo(citizen->Building.School);
		else
			Idle(citizen);
	}
	else
		Cast<AAI>(GetOwner())->MoveToBroch();
}

void ADiplosimAIController::Idle(ACitizen* Citizen)
{
	if (!IsValid(Citizen))
		return;

	MoveRequest.SetGoalActor(nullptr);

	int32 chance = Camera->Grid->Stream.RandRange(0, 100);

	int32 hoursLeft = Citizen->HoursSleptToday.Num();

	AHouse* house = Citizen->Building.House;

	if (!IsValid(house)) {
		TArray<AHouse*> familyHouses;

		for (ACitizen* citizen : Citizen->GetLikedFamily(false)) {
			if (!IsValid(citizen->Building.House) || !IsValid(citizen->Building.House->GetOccupant(citizen)) || citizen->Building.House->GetVisitors(citizen->Building.House->GetOccupant(citizen)).Num() == citizen->Building.House->Space)
				continue;

			familyHouses.Add(citizen->Building.House);
		}

		if (!familyHouses.IsEmpty()) {
			int32 index = Camera->Grid->Stream.RandRange(0, familyHouses.Num() - 1);

			house = familyHouses[index];

			ACitizen* citizen = Cast<ACitizen>(GetOwner());

			familyHouses[index]->AddVisitor(house->GetOccupant(citizen), citizen);
		}
	}

	if (IsValid(house) && (hoursLeft - 1 <= Citizen->IdealHoursSlept || chance < 33))
		AIMoveTo(house);
	else {
		int32 time = Camera->Grid->Stream.RandRange(3, 10);

		if (IsValid(ChosenBuilding) && ChosenBuilding->bHideCitizen && chance < 66 && !Camera->CitizenManager->Arrested.Contains(Citizen)) {
			AIMoveTo(ChosenBuilding);

			time = 60.0f;
		}
		else {
			UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
			const ANavigationData* navData = nav->GetDefaultNavDataInstance();

			int32 range = 1000;

			FVector location = Camera->CitizenManager->BrochLocation;
					
			if (IsValid(ChosenBuilding) && !ChosenBuilding->IsA<ABroch>()) {
				FVector size = ChosenBuilding->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize();

				if (size.X > size.Y)
					range = size.X;
				else
					range = size.Y;

				location = ChosenBuilding->GetActorLocation();
			}

			FNavLocation navLoc;
			nav->GetRandomPointInNavigableRadius(location, range, navLoc);

			location = Camera->GetTargetLocation(Citizen);

			double length;
			nav->GetPathLength(location, navLoc.Location, length);

			if (length < 5000.0f) {
				UNavigationPath* path = nav->FindPathToLocationSynchronously(GetWorld(), location, navLoc.Location, Citizen, Citizen->NavQueryFilter);

				Citizen->MovementComponent->SetPoints(path->PathPoints);

				if (IsValid(Citizen->Building.BuildingAt))
					AsyncTask(ENamedThreads::GameThread, [Citizen]() { Citizen->Building.BuildingAt->Leave(Citizen); });
			}
		}

		Camera->CitizenManager->CreateTimer("Idle", Citizen, time, FTimerDelegate::CreateUObject(this, &ADiplosimAIController::DefaultAction), false, true);
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

			double magnitude = GetClosestActor(400.0f, Camera->GetTargetLocation(GetOwner()), target->GetActorLocation(), building->GetActorLocation(), true, target->Storage[targetIndex].Amount, building->Storage[index].Amount);

			if (magnitude <= 0.0f)
				continue;

			target = building;
		}
	}

	if (target != nullptr)
		AIMoveTo(target);
	else
		Camera->CitizenManager->CreateTimer("FindGatherSite", GetOwner(), 30.0f, FTimerDelegate::CreateUObject(this, &ADiplosimAIController::GetGatherSite, Camera, Resource), false);
}

bool ADiplosimAIController::CanMoveTo(FVector Location, AActor* Target, bool bCheckForPortals)
{
	if (!IsValid(Target))
		Target = GetOwner();

	if (!IsValid(GetOwner()) || !IsValid(Target))
		return false;

	UHealthComponent* healthComp = Target->GetComponentByClass<UHealthComponent>();

	if (healthComp && healthComp->GetHealth() == 0)
		return false;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* navData = nav->GetDefaultNavDataInstance();

	FNavLocation targetLoc;
	nav->ProjectPointToNavigation(Location, targetLoc, FVector(400.0f, 400.0f, 20.0f));

	FNavLocation ownerLoc;
	nav->ProjectPointToNavigation(Camera->GetTargetLocation(Target), ownerLoc, FVector(400.0f, 400.0f, 20.0f));

	FPathFindingQuery query(Target, *navData, ownerLoc.Location, targetLoc.Location);

	bool path = nav->TestPathSync(query, EPathFindingMode::Hierarchical);

	if (path)
		return true;

	if (bCheckForPortals && GetOwner()->IsA<ACitizen>()) {
		ACitizen* citizen = Cast<ACitizen>(GetOwner());

		bool targetCanReachPortal = false;
		bool ownerCanReachPortal = false;

		for (ABuilding* building : Camera->CitizenManager->Buildings) {
			if (!building->IsA<APortal>() || building->HealthComponent->GetHealth() == 0)
				continue;

			FNavLocation buildingLoc;
			nav->ProjectPointToNavigation(building->GetActorLocation(), buildingLoc, FVector(400.0f, 400.0f, 20.0f));

			FPathFindingQuery ownerQuery(GetOwner(), *navData, ownerLoc.Location, buildingLoc.Location);
			FPathFindingQuery targetQuery(GetOwner(), *navData, targetLoc.Location, buildingLoc.Location);

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

	if (GetOwner()->IsValidLowLevelFast() && Cast<AAI>(GetOwner())->NavQueryFilter != nullptr)
		filter = Cast<AAI>(GetOwner())->NavQueryFilter;

	if (GetOwner()->IsA<ACitizen>() && Cast<ACitizen>(GetOwner())->Building.BuildingAt != nullptr)
		StartLocation = Cast<ACitizen>(GetOwner())->Building.EnterLocation;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	UNavigationPath* path = nav->FindPathToLocationSynchronously(GetWorld(), StartLocation, EndLocation, GetOwner(), filter);

	return path->PathPoints;
}

void ADiplosimAIController::AIMoveTo(AActor* Actor, FVector Location, int32 Instance)
{
	if (!IsValid(this) || !IsValid(Owner))
		return;
	
	int32 reach = Cast<AAI>(GetOwner())->Range / 15.0f;
	
	if ((IsValid(MoveRequest.GetGoalActor()) && Cast<AAI>(GetOwner())->CanReach(MoveRequest.GetGoalActor(), reach)) || (GetOwner()->IsA<ACitizen>() && Cast<ACitizen>(GetOwner())->Building.BuildingAt == Actor))
		return;

	StartMovement();

	MoveRequest.SetGoalActor(Actor);
	MoveRequest.SetGoalInstance(Instance);
	MoveRequest.SetLocation(Camera->GetTargetLocation(Actor));

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	if (GetOwner()->IsA<ACitizen>()) {
		ACitizen* citizen = Cast<ACitizen>(GetOwner());

		ABuilding* ownerNearestPortal = nullptr;
		ABuilding* targetNearestPortal = nullptr;

		for (ABuilding* building : Camera->CitizenManager->Buildings) {
			if (!building->IsA<APortal>() || building->HealthComponent->GetHealth() == 0)
				continue;

			if (!IsValid(ownerNearestPortal) && CanMoveTo(building->GetActorLocation(), GetOwner(), false))
				ownerNearestPortal = building;

			if (!IsValid(targetNearestPortal) && CanMoveTo(building->GetActorLocation(), Actor, false))
				targetNearestPortal = building;
		}

		if (IsValid(ownerNearestPortal) && IsValid(targetNearestPortal)) {
			double originalPathLength = 0.0f;
			auto originalResult = nav->GetPathLength(Camera->GetTargetLocation(GetOwner()), MoveRequest.GetLocation(), originalPathLength);

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

	TArray<FVector> points = GetPathPoints(Camera->GetTargetLocation(GetOwner()), MoveRequest.GetLocation());

	Cast<AAI>(GetOwner())->MovementComponent->SetPoints(points);

	SetFocus(Actor);

	if (!GetOwner()->IsA<ACitizen>() || Cast<ACitizen>(GetOwner())->Building.BuildingAt == Actor)
		return;

	ACitizen* citizen = Cast<ACitizen>(GetOwner());

	Camera->CitizenManager->RemoveTimer("Idle", GetOwner());

	if (citizen->Building.BuildingAt != nullptr)
		citizen->Building.BuildingAt->Leave(citizen);
}

void ADiplosimAIController::RecalculateMovement(AActor* Actor)
{
	if (!IsValid(Actor) || !IsValid(GetOwner()) || !IsValid(Cast<AAI>(GetOwner())->MovementComponent) || MoveRequest.GetGoalActor() != Actor || (!Cast<AAI>(GetOwner())->MovementComponent->Points.IsEmpty() && FVector::Dist(Camera->GetTargetLocation(Actor), Cast<AAI>(GetOwner())->MovementComponent->Points.Last()) < 20.0f))
		return;

	AIMoveTo(Actor, MoveRequest.GetLocation(), MoveRequest.GetGoalInstance());
}

void ADiplosimAIController::StartMovement()
{
	if (!Cast<AAI>(GetOwner())->MovementComponent->Points.IsEmpty())
		return;
	
	Cast<AAI>(GetOwner())->MovementComponent->SetAnimation(EAnim::Move, true);
}

void ADiplosimAIController::StopMovement()
{
	if (!IsValid(GetOwner()))
		return;
	
	MoveRequest.SetGoalActor(nullptr);
	
	Cast<AAI>(GetOwner())->MovementComponent->SetPoints({});
}