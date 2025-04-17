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
#include "Buildings/Misc/Broch.h"
#include "Buildings/Misc/Road.h"
#include "Universal/Resource.h"
#include "AttackComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/CitizenManager.h"
#include "AIMovementComponent.h"

ADiplosimAIController::ADiplosimAIController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickInterval(0.4f);
}

void ADiplosimAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (DeltaTime > 1.0f)
		return;

	if (Cast<AAI>(Owner)->HealthComponent->Health == 0 || !IsValid(MoveRequest.GetGoalActor()))
		return;

	TSubclassOf<UNavigationQueryFilter> filter = Cast<AAI>(GetOwner())->NavQueryFilter;

	FCollidingStruct collidingStruct;
	collidingStruct.Actor = MoveRequest.GetGoalActor();
	collidingStruct.Instance = MoveRequest.GetGoalInstance();

	if (Cast<AAI>(GetOwner())->AttackComponent->MeleeableEnemies.Contains(MoveRequest.GetGoalActor())) {
		FRotator rotation = (MoveRequest.GetGoalActor()->GetActorLocation() - Owner->GetActorLocation()).Rotation();

		GetOwner()->SetActorRotation(FMath::RInterpTo(GetOwner()->GetActorRotation(), rotation, DeltaTime, 100.0f));

		return;
	}

	int32 range = Cast<AAI>(GetOwner())->AttackComponent->RangeComponent->GetUnscaledSphereRadius();

	if ((FMath::IsNearlyZero(GetOwner()->GetVelocity().X) && FMath::IsNearlyZero(GetOwner()->GetVelocity().Y)) && FVector::Dist(GetOwner()->GetActorLocation(), MoveRequest.GetGoalActor()->GetActorLocation()) < (range / 20.0f))
		RecalculateMovement(MoveRequest.GetGoalActor());

	if (MoveRequest.GetGoalActor()->IsA<AAI>())
		MoveToActor(MoveRequest.GetGoalActor(), 1.0f, true, true, true, filter, true);
}

void ADiplosimAIController::DefaultAction()
{
	MoveRequest.SetGoalActor(nullptr);

	SetActorTickEnabled(false);

	if (GetOwner()->IsA<ACitizen>() && !Cast<ACitizen>(GetOwner())->Rebel) {
		ACitizen* citizen = Cast<ACitizen>(GetOwner());

		if (citizen->Camera->CitizenManager->IsAttendingEvent(citizen))
			return;

		for (FEventStruct event : citizen->Camera->CitizenManager->OngoingEvents()) {
			citizen->Camera->CitizenManager->GotoEvent(citizen, event);

			if (citizen->Camera->CitizenManager->IsAttendingEvent(citizen))
				return;
		}

		if (citizen->Building.Employment != nullptr && citizen->Building.Employment->bOpen)
			AIMoveTo(citizen->Building.Employment);
		else if (citizen->Building.School != nullptr && citizen->Building.School->bOpen)
			AIMoveTo(citizen->Building.School);
		else
			Idle(citizen);
	}
	else
		Cast<AAI>(GetOwner())->MoveToBroch();
}

void ADiplosimAIController::Idle(ACitizen* Citizen)
{
	if (!Citizen->IsValidLowLevelFast())
		return;

	int32 chance = FMath::RandRange(0, 100);

	int32 hoursLeft = Citizen->HoursSleptToday.Num();

	if (IsValid(Citizen->Building.Employment)) {
		if (Citizen->Building.Employment->WorkEnd > Citizen->Building.Employment->WorkStart)
			hoursLeft = (24 - Citizen->Building.Employment->WorkEnd) + Citizen->Building.Employment->WorkStart;
		else
			hoursLeft = Citizen->Building.Employment->WorkStart - Citizen->Building.Employment->WorkEnd;
	}

	AHouse* house = Citizen->Building.House;

	if (!IsValid(house)) {
		TArray<AHouse*> familyHouses;

		for (ACitizen* citizen : Citizen->GetLikedFamily(false)) {
			if (!IsValid(citizen->Building.House) || !IsValid(citizen->Building.House->GetOccupant(citizen)) || citizen->Building.House->GetVisitors(citizen->Building.House->GetOccupant(citizen)).Num() == citizen->Building.House->Space)
				continue;

			familyHouses.Add(citizen->Building.House);
		}

		if (!familyHouses.IsEmpty()) {
			int32 index = FMath::RandRange(0, familyHouses.Num() - 1);

			house = familyHouses[index];

			ACitizen* citizen = Cast<ACitizen>(GetOwner());

			familyHouses[index]->AddVisitor(house->GetOccupant(citizen), citizen);
		}
	}

	if (IsValid(house) && (hoursLeft - 1 <= Citizen->IdealHoursSlept || chance < 33))
		AIMoveTo(house);
	else {
		int32 time = FMath::RandRange(3, 10);

		if (IsValid(ChosenBuilding) && ChosenBuilding->bHideCitizen && chance < 66) {
			AIMoveTo(ChosenBuilding);

			time = 60.0f;
		}
		else {
			UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
			const ANavigationData* navData = nav->GetDefaultNavDataInstance();

			int32 range = 1000;

			FVector location = Citizen->Camera->CitizenManager->BrochLocation;
					
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

			double length;
			nav->GetPathLength(Citizen->GetActorLocation(), navLoc.Location, length);

			if (length < 5000.0f) {
				UNavigationPath* path = nav->FindPathToLocationSynchronously(GetWorld(), Citizen->GetActorLocation(), navLoc.Location, Citizen, Citizen->NavQueryFilter);

				Citizen->MovementComponent->SetPoints(path->PathPoints);
			}
		}

		FTimerStruct timer;
		timer.CreateTimer("Idle", Citizen, time, FTimerDelegate::CreateUObject(this, &ADiplosimAIController::DefaultAction), false, true);

		Citizen->Camera->CitizenManager->Timers.Add(timer);
	}
}

void ADiplosimAIController::ChooseIdleBuilding(ACitizen* Citizen)
{
	TArray<ABuilding*> buildings;

	for (ABuilding* building : Citizen->Camera->CitizenManager->Buildings) {
		if (building->IsA<AWork>() || building->IsA<ARoad>() || (building->IsA<AHouse>() && building->Inside.IsEmpty()) || !CanMoveTo(building->GetActorLocation()))
			continue;

		buildings.Add(building);
	}

	if (buildings.IsEmpty()) {
		ChosenBuilding = nullptr;

		return;
	}

	int32 index = FMath::RandRange(0, buildings.Num() - 1);

	ChosenBuilding = buildings[index];

	FTimerStruct timer;
	timer.CreateTimer("ChooseIdleBuilding", Citizen, 60.0f, FTimerDelegate::CreateUObject(this, &ADiplosimAIController::ChooseIdleBuilding, Citizen), true);

	Citizen->Camera->CitizenManager->Timers.Add(timer);
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

void ADiplosimAIController::GetGatherSite(ACamera* Camera, TSubclassOf<AResource> Resource)
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

			double magnitude = GetClosestActor(400.0f, GetOwner()->GetActorLocation(), target->GetActorLocation(), building->GetActorLocation(), true, target->Storage[targetIndex].Amount, building->Storage[index].Amount);

			if (magnitude <= 0.0f)
				continue;

			target = building;
		}
	}

	if (target != nullptr)
		AIMoveTo(target);
	else {
		FTimerStruct timer;
		timer.CreateTimer("FindGatherSite", GetOwner(), 30.0f, FTimerDelegate::CreateUObject(this, &ADiplosimAIController::GetGatherSite, Camera, Resource), false);

		Camera->CitizenManager->Timers.Add(timer);
	}
}

bool ADiplosimAIController::CanMoveTo(FVector Location)
{
	if (!IsValid(GetOwner()))
		return false;

	UHealthComponent* healthComp = GetOwner()->GetComponentByClass<UHealthComponent>();

	if (healthComp && healthComp->GetHealth() == 0)
		return false;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* navData = nav->GetDefaultNavDataInstance();

	FNavLocation targetLoc;
	nav->ProjectPointToNavigation(Location, targetLoc, FVector(400.0f, 400.0f, 20.0f));

	FNavLocation ownerLoc;
	nav->ProjectPointToNavigation(GetOwner()->GetActorLocation(), ownerLoc, FVector(400.0f, 400.0f, 20.0f));

	FPathFindingQuery query(GetOwner(), *navData, ownerLoc.Location, targetLoc.Location);

	bool path = nav->TestPathSync(query, EPathFindingMode::Hierarchical);

	if (path)
		return true;

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
	if (Cast<AAI>(GetOwner())->AttackComponent->MeleeableEnemies.Contains(MoveRequest.GetGoalActor()) || (GetOwner()->IsA<ACitizen>() && Cast<ACitizen>(GetOwner())->Building.BuildingAt == Actor))
		return;
	
	MoveRequest.SetGoalActor(Actor);
	MoveRequest.SetGoalInstance(Instance);
	MoveRequest.SetLocation(Actor->GetActorLocation());

	SetActorTickEnabled(true);

	if (Location != FVector::Zero()) {
		MoveRequest.SetLocation(Location);
	}
	else if (Actor->IsA<ABuilding>()) {
		UStaticMeshComponent* comp = Actor->GetComponentByClass<UStaticMeshComponent>();

		if (comp && comp->DoesSocketExist("Entrance"))
			MoveRequest.SetLocation(comp->GetSocketLocation("Entrance"));
	}

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	FNavLocation navLoc;
	nav->ProjectPointToNavigation(MoveRequest.GetLocation(), navLoc, FVector(400.0f, 400.0f, 40.0f));

	MoveRequest.SetLocation(navLoc.Location);

	TArray<FVector> points = GetPathPoints(GetOwner()->GetActorLocation(), MoveRequest.GetLocation());

	Cast<AAI>(GetOwner())->MovementComponent->SetPoints(points);

	SetFocus(Actor);

	if (!GetOwner()->IsA<ACitizen>() || Cast<ACitizen>(GetOwner())->Building.BuildingAt == Actor)
		return;

	ACitizen* citizen = Cast<ACitizen>(GetOwner());

	citizen->Camera->CitizenManager->RemoveTimer("Idle", GetOwner());

	if (citizen->Building.BuildingAt != nullptr)
		citizen->Building.BuildingAt->Leave(citizen);
}

void ADiplosimAIController::RecalculateMovement(AActor* Actor)
{
	if (MoveRequest.GetGoalActor() != Actor)
		return;

	AIMoveTo(Actor, MoveRequest.GetLocation(), MoveRequest.GetGoalInstance());
}

void ADiplosimAIController::StartMovement()
{
	Cast<AAI>(GetOwner())->MovementComponent->CurrentAnim = nullptr;

	Cast<AAI>(GetOwner())->MovementComponent->SetComponentTickEnabled(true);
}

void ADiplosimAIController::StopMovement()
{
	Cast<AAI>(GetOwner())->MovementComponent->SetPoints({});

	Cast<AAI>(GetOwner())->MovementComponent->SetComponentTickEnabled(false);
}