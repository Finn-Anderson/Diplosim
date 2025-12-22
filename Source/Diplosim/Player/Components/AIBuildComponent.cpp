#include "Player/Components/AIBuildComponent.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "NavigationSystem.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "AI/BuildingComponent.h"
#include "Buildings/House.h"
#include "Buildings/Misc/Broch.h"
#include "Buildings/Misc/Road.h"
#include "Buildings/Work/Service/Builder.h"
#include "Buildings/Work/Service/Trader.h"
#include "Buildings/Work/Production/ExternalProduction.h"
#include "Buildings/Work/Production/InternalProduction.h"
#include "Map/Grid.h"
#include "Player/Camera.h"
#include "Player/Components/BuildComponent.h"
#include "Player/Managers/ResourceManager.h"

UAIBuildComponent::UAIBuildComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

//
// Tile Distance Calulation
//
double UAIBuildComponent::CalculateTileDistance(FVector EggTimerLocation, FVector TileLocation)
{
	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	FNavLocation targetLoc;
	nav->ProjectPointToNavigation(EggTimerLocation, targetLoc, FVector(20.0f, 20.0f, 20.0f));

	FNavLocation ownerLoc;
	nav->ProjectPointToNavigation(TileLocation, ownerLoc, FVector(20.0f, 20.0f, 20.0f));

	double length;
	ENavigationQueryResult::Type result = nav->GetPathLength(GetWorld(), targetLoc.Location, ownerLoc.Location, length);

	if (result != ENavigationQueryResult::Success)
		return 100000000000.0f;

	return length;
}

void UAIBuildComponent::InitialiseTileLocationDistances(FFactionStruct* Faction)
{
	for (TArray<FTileStruct>& row : Camera->Grid->Storage) {
		for (FTileStruct& tile : row) {
			if (tile.bMineral || tile.bRamp)
				continue;

			FTransform transform = Camera->Grid->GetTransform(&tile);

			if (Faction->Citizens[0]->AIController->CanMoveTo(transform.GetLocation()))
				Faction->AccessibleBuildLocations.Add(transform.GetLocation(), CalculateTileDistance(Faction->EggTimer->GetActorLocation(), transform.GetLocation()));
			else
				Faction->InaccessibleBuildLocations.Add(transform.GetLocation());
		}
	}

	RemoveTileLocations(Faction, Faction->EggTimer);

	SortTileDistances(Faction->AccessibleBuildLocations);
}

void UAIBuildComponent::RecalculateTileLocationDistances(FFactionStruct* Faction)
{
	for (int32 i = Faction->InaccessibleBuildLocations.Num() - 1; i > -1; i--) {
		FVector location = Faction->InaccessibleBuildLocations[i];

		if (!Faction->Citizens[0]->AIController->CanMoveTo(location))
			continue;

		Faction->AccessibleBuildLocations.Add(location, CalculateTileDistance(Faction->EggTimer->GetActorLocation(), location));
		Faction->InaccessibleBuildLocations.RemoveAt(i);
	}

	Faction->FailedBuild = 0;

	SortTileDistances(Faction->AccessibleBuildLocations);
}

void UAIBuildComponent::RemoveTileLocations(FFactionStruct* Faction, ABuilding* Building)
{
	TArray<FVector> roadLocations;

	TArray<FVector> locations;
	Faction->AccessibleBuildLocations.GenerateKeyArray(locations);

	for (FVector location : locations) {
		FVector hitLocation;
		Building->BuildingMesh->GetClosestPointOnCollision(location, hitLocation);

		double distance = FVector::Dist(location, hitLocation);

		if (distance < 100.0f) {
			Faction->AccessibleBuildLocations.Remove(location);

			if (distance > 40.0f)
				roadLocations.Add(location);
		}
	}

	if (!Faction->RoadBuildLocations.IsEmpty()) {
		FVector closestCurrentRoad = FVector::Zero();
		for (FVector location : Faction->RoadBuildLocations) {
			if (closestCurrentRoad == FVector::Zero()) {
				closestCurrentRoad = location;

				continue;
			}

			if (FVector::Dist(Building->GetActorLocation(), location) > FVector::Dist(Building->GetActorLocation(), closestCurrentRoad))
				continue;

			closestCurrentRoad = location;
		}

		FVector closestNewRoad = FVector::Zero();
		for (FVector location : roadLocations) {
			if (closestNewRoad == FVector::Zero()) {
				closestNewRoad = location;

				continue;
			}

			if (FVector::Dist(closestCurrentRoad, location) > FVector::Dist(closestCurrentRoad, closestNewRoad))
				continue;

			closestNewRoad = location;
		}

		FTileStruct* tile = Camera->Grid->GetTileFromLocation(closestNewRoad);

		roadLocations.Append(ConnectRoadTiles(Faction, tile, closestCurrentRoad));
	}

	for (FVector location : roadLocations) {
		if (Faction->RoadBuildLocations.Contains(location))
			continue;

		Faction->RoadBuildLocations.Add(location);
	}

	SortTileDistances(Faction->AccessibleBuildLocations);
}

void UAIBuildComponent::SortTileDistances(TMap<FVector, double>& Locations)
{
	Locations.ValueSort([](double s1, double s2)
		{
			return s1 < s2;
		});
}

TArray<FVector> UAIBuildComponent::ConnectRoadTiles(FFactionStruct* Faction, FTileStruct* Tile, FVector Location)
{
	TArray<FVector> locations;

	FTransform transform = Camera->Grid->GetTransform(Tile);

	if (transform.GetLocation() == Location)
		return locations;

	locations.Add(transform.GetLocation());

	FTileStruct* closestTile = nullptr;

	for (auto& element : Tile->AdjacentTiles) {
		if (closestTile == nullptr) {
			closestTile = element.Value;

			continue;
		}

		FTransform closestTransform = Camera->Grid->GetTransform(closestTile);
		FTransform newTransform = Camera->Grid->GetTransform(element.Value);

		if (FVector::Dist(Location, newTransform.GetLocation()) > FVector::Dist(Location, closestTransform.GetLocation()))
			continue;

		closestTile = element.Value;
	}

	locations.Append(ConnectRoadTiles(Faction, closestTile, Location));

	return locations;
}

//
// Building
//
void UAIBuildComponent::EvaluateAIBuild(FFactionStruct* Faction)
{
	int32 numBuildings = Faction->Buildings.Num();

	AITrade(Faction);

	BuildFirstBuilder(Faction);

	for (ABuilding* building : Faction->RuinedBuildings)
		building->Rebuild(Faction->Name);

	BuildAIBuild(Faction);

	BuildAIHouse(Faction);

	BuildAIRoads(Faction);

	if (Faction->Buildings.Num() > numBuildings)
		RecalculateTileLocationDistances(Faction);
	else
		Faction->FailedBuild++;

	BuildAIAccessibility(Faction);
}

void UAIBuildComponent::AITrade(FFactionStruct* Faction)
{
	ATrader* trader = nullptr;

	for (ABuilding* building : Faction->Buildings) {
		if (!building->IsA<ATrader>())
			continue;

		trader = Cast<ATrader>(building);

		break;
	}

	if (!IsValid(trader) || !trader->Orders.IsEmpty())
		return;

	FQueueStruct order;

	for (FFactionResourceStruct resourceStruct : Faction->Resources) {
		if (resourceStruct.Type == Camera->ResourceManager->Money)
			continue;

		int32 amount = Camera->ResourceManager->GetResourceAmount(Faction->Name, resourceStruct.Type);

		if (amount <= 100)
			continue;

		FItemStruct item;
		item.Resource = resourceStruct.Type;
		item.Amount = amount - 100;

		order.SellingItems.Add(item);
	}

	trader->SetNewOrder(order);
}

void UAIBuildComponent::BuildFirstBuilder(FFactionStruct* Faction)
{
	bool bContainsBuilder = false;

	for (ABuilding* building : Faction->Buildings) {
		if (!building->IsA<ABuilder>())
			continue;

		bContainsBuilder = true;

		break;
	}

	if (bContainsBuilder) {
		for (FAIBuildStruct aibuild : AIBuilds) {
			if (!aibuild.Building->GetDefaultObject()->IsA<ABuilder>())
				continue;

			AIBuild(Faction, aibuild.Building, nullptr);

			return;
		}
	}
}

void UAIBuildComponent::BuildAIBuild(FFactionStruct* Faction)
{
	int32 count = 0;

	TArray<TSubclassOf<ABuilding>> buildingsClassList;

	for (FResourceStruct resource : Camera->ResourceManager->ResourceList) {
		if (resource.Category != "Food")
			continue;

		if (count == 2)
			break;

		int32 trend = Camera->ResourceManager->GetCategoryTrend(Faction->Name, resource.Category);

		if (trend >= 0)
			continue;

		TArray<TSubclassOf<AResource>> resources = Camera->ResourceManager->GetResourcesFromCategory(resource.Category);

		for (TSubclassOf<AResource> r : resources) {
			TArray<TSubclassOf<ABuilding>> buildings;
			Camera->ResourceManager->GetBuildings(r).GenerateKeyArray(buildings);

			buildingsClassList.Append(buildings);
		}
	}

	if (buildingsClassList.IsEmpty()) {
		for (FAIBuildStruct& aibuild : AIBuilds) {
			if (Faction->Citizens.Num() < aibuild.NumCitizens || aibuild.CurrentAmount == aibuild.Limit || !AICanAfford(Faction, aibuild.Building))
				continue;

			if (Camera->Stream.RandRange(1, 100) < 50) {
				aibuild.NumCitizens *= 1.1f;

				continue;
			}

			if (aibuild.Building->GetDefaultObject()->IsA<AInternalProduction>()) {
				AInternalProduction* internalBuilding = Cast<AInternalProduction>(aibuild.Building->GetDefaultObject());

				if (!internalBuilding->Intake.IsEmpty()) {
					bool bMakesResources = true;

					for (FItemStruct item : internalBuilding->Intake) {
						int32 trend = Camera->ResourceManager->GetResourceTrend(Faction->Name, item.Resource);

						if (trend > 0.0f)
							continue;

						bMakesResources = false;

						break;
					}

					if (!bMakesResources)
						continue;
				}
			}

			buildingsClassList.Add(aibuild.Building);
		}
	}

	ChooseBuilding(Faction, buildingsClassList);
}

void UAIBuildComponent::BuildAIHouse(FFactionStruct* Faction)
{
	TArray<TSubclassOf<ABuilding>> buildingsClassList;

	TArray<ACitizen*> homeless;

	for (ACitizen* citizen : Faction->Citizens) {
		if (IsValid(citizen->BuildingComponent->House))
			continue;

		homeless.Add(citizen);
	}

	if (homeless.IsEmpty())
		return;

	for (TSubclassOf<ABuilding> house : Houses) {
		if (!AICanAfford(Faction, house))
			continue;

		int32 amount = 0;

		for (ACitizen* citizen : homeless)
			if (citizen->BuildingComponent->Employment->GetWage(citizen) >= Cast<AHouse>(house->GetDefaultObject())->Rent)
				amount++;

		if (amount / homeless.Num() < 0.33f)
			continue;

		buildingsClassList.Add(house);
	}

	ChooseBuilding(Faction, buildingsClassList);
}

void UAIBuildComponent::BuildAIRoads(FFactionStruct* Faction)
{
	if (Faction->RoadBuildLocations.IsEmpty())
		return;

	int32 amount = Camera->ResourceManager->GetResourceAmount(Faction->Name, Camera->ResourceManager->Money);

	if (amount <= 100)
		return;

	int32 cost = Cast<ABuilding>(RoadClass->GetDefaultObject())->CostList[0].Amount;
	int32 numRoads = FMath::CeilToInt((amount - 100.0f) / cost);

	for (int32 i = 0; i < numRoads; i++) {
		if (Faction->RoadBuildLocations.IsEmpty())
			return;

		AIBuild(Faction, RoadClass, nullptr);
	}
}

void UAIBuildComponent::BuildMiscBuild(FFactionStruct* Faction)
{
	if (Camera->Stream.RandRange(1, 100) != 100)
		return;

	TArray<TSubclassOf<ABuilding>> buildingsClassList;

	for (TSubclassOf<ABuilding> misc : MiscBuilds) {
		if (!AICanAfford(Faction, misc))
			continue;

		buildingsClassList.Add(misc);
	}

	ChooseBuilding(Faction, buildingsClassList);
}

void UAIBuildComponent::BuildAIAccessibility(FFactionStruct* Faction)
{
	bool bCanAffordRamp = AICanAfford(Faction, RampClass);
	bool bCanAffordRoad = AICanAfford(Faction, RoadClass);

	if (Faction->FailedBuild <= 3 || Faction->AccessibleBuildLocations.Num() > 50 || (!bCanAffordRamp && !bCanAffordRoad))
		return;

	TSubclassOf<ABuilding> chosenClass = nullptr;

	FTileStruct* chosenTile = nullptr;
	FRotator rotation = FRotator::ZeroRotator;

	if (bCanAffordRamp) {
		for (auto& element : Faction->AccessibleBuildLocations) {
			if (chosenTile != nullptr && FVector::Dist(Faction->EggTimer->GetActorLocation(), element.Key) > FVector::Dist(Faction->EggTimer->GetActorLocation(), Camera->Grid->GetTransform(chosenTile).GetLocation()))
				continue;

			FTileStruct* tile = Camera->Grid->GetTileFromLocation(element.Key);
			bool bValid = false;

			for (auto& e : tile->AdjacentTiles) {
				if (!Faction->InaccessibleBuildLocations.Contains(Camera->Grid->GetTransform(e.Value).GetLocation()))
					continue;

				bValid = true;
			}

			if (!bValid)
				continue;

			chosenTile = tile;
		}
	}

	if (chosenTile == nullptr) {
		if (!bCanAffordRoad)
			return;

		chosenClass = RoadClass;
	}
	else {
		chosenClass = RampClass;
	}

	AIBuild(Faction, chosenClass, nullptr, true, chosenTile);
}

void UAIBuildComponent::ChooseBuilding(FFactionStruct* Faction, TArray<TSubclassOf<ABuilding>> BuildingsClasses)
{
	if (BuildingsClasses.IsEmpty())
		return;

	int32 chosenIndex = Camera->Stream.RandRange(0, BuildingsClasses.Num() - 1);
	TSubclassOf<ABuilding> chosenBuildingClass = BuildingsClasses[chosenIndex];

	TSubclassOf<AResource> resource = nullptr;

	FAIBuildStruct aibuild;
	aibuild.Building = chosenBuildingClass;

	int32 i = AIBuilds.Find(aibuild);

	if (i != INDEX_NONE)
		resource = AIBuilds[i].Resource;

	AIBuild(Faction, chosenBuildingClass, resource);
}

bool UAIBuildComponent::AIValidBuildingLocation(FFactionStruct* Faction, ABuilding* Building, float Extent, FVector Location)
{
	if (!Faction->Citizens[0]->AIController->CanMoveTo(Location) || !Camera->BuildComponent->IsValidLocation(Building, Extent, Location))
		return false;

	return true;
}

bool UAIBuildComponent::AICanAfford(FFactionStruct* Faction, TSubclassOf<ABuilding> BuildingClass, int32 Amount)
{
	TArray<FItemStruct> items = Cast<ABuilding>(BuildingClass->GetDefaultObject())->CostList;

	if (Amount > 1)
		for (FItemStruct& item : items)
			item.Amount *= Amount;

	for (FItemStruct item : items) {
		int32 maxAmount = Camera->ResourceManager->GetResourceAmount(Faction->Name, item.Resource);

		if (maxAmount < item.Amount)
			return false;
	}

	return true;
}

void UAIBuildComponent::AIBuild(FFactionStruct* Faction, TSubclassOf<ABuilding> BuildingClass, TSubclassOf<AResource> Resource, bool bAccessibility, FTileStruct* Tile)
{
	FActorSpawnParameters spawnParams;
	spawnParams.bNoFail = true;

	FRotator rotation = FRotator(0.0f, FMath::RoundHalfFromZero(Camera->Stream.FRandRange(0.0f, 270.0f) / 90.0f) * 90.0f, 0.0f);

	ABuilding* building = GetWorld()->SpawnActor<ABuilding>(BuildingClass, FVector(0.0f, 0.0f, -1000.0f), FRotator::ZeroRotator, spawnParams);
	building->FactionName = Faction->Name;

	for (int32 i = 0; i < building->Seeds.Num(); i++) {
		if (Resource != nullptr) {
			if (building->Seeds[i].Resource != Resource)
				continue;

			building->SetSeed(i);

			break;
		}
		else if (!building->Seeds[i].Cost.IsEmpty()) {
			bool bCanAfford = true;

			for (FItemStruct item : building->Seeds[i].Cost)
				if (item.Amount > Camera->ResourceManager->GetResourceAmount(Faction->Name, item.Resource))
					bCanAfford = false;

			if (bCanAfford)
				building->SetSeed(i);
		}
		else {
			building->SetSeed(Camera->Stream.RandRange(0, building->Seeds.Num() - 1));

			break;
		}
	}

	FVector size = building->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize() / 2.0f;
	float extent = size.X / (size.X + 100.0f);
	float yExtent = size.Y / (size.Y + 100.0f);

	if (yExtent > extent)
		extent = yExtent;

	FVector location = FVector(10000000000.0f);
	FVector offset = building->BuildingMesh->GetComponentLocation() + FVector(0.0f, 0.0f, 1000.0f);

	if (bAccessibility) {
		if (BuildingClass == RoadClass) {
			for (int32 i = 0; i < Camera->Grid->HISMRiver->GetInstanceCount(); i++) {
				FTransform transform;
				Camera->Grid->HISMRiver->GetInstanceTransform(i, transform);

				bool bClose = false;
				for (ABuilding* b : Faction->Buildings) {
					if (FVector::Dist(b->GetActorLocation(), transform.GetLocation()) > 500.0f)
						continue;

					bClose = true;
				}

				FTileStruct* tile = Camera->Grid->GetTileFromLocation(transform.GetLocation());

				if (AIValidBuildingLocation(Faction, building, 2.0f, transform.GetLocation()))
					continue;

				Tile = tile;

				break;
			}
		}
		else {
			FTileStruct* chosenInaccessibleTile = nullptr;

			for (auto& e : Tile->AdjacentTiles) {
				if (!Faction->InaccessibleBuildLocations.Contains(Camera->Grid->GetTransform(e.Value).GetLocation()))
					continue;

				chosenInaccessibleTile = e.Value;

				break;
			}

			if (Tile->Level > chosenInaccessibleTile->Level) {
				rotation = (Camera->Grid->GetTransform(chosenInaccessibleTile).GetLocation() - Camera->Grid->GetTransform(Tile).GetLocation()).Rotation();

				Tile = chosenInaccessibleTile;
			}
			else
				rotation = (Camera->Grid->GetTransform(Tile).GetLocation() - Camera->Grid->GetTransform(chosenInaccessibleTile).GetLocation()).Rotation();
		}

		location = Camera->Grid->GetTransform(Tile).GetLocation();
	}
	if (building->IsA<AExternalProduction>()) {
		AExternalProduction* externalBuilding = Cast<AExternalProduction>(building);

		for (auto& element : externalBuilding->GetValidResources()) {
			for (int32 inst : element.Value) {
				FTransform transform;
				element.Key->ResourceHISM->GetInstanceTransform(inst, transform);

				transform.SetLocation(transform.GetLocation() + offset);

				if (FVector::Dist(Faction->EggTimer->GetActorLocation(), location) < FVector::Dist(Faction->EggTimer->GetActorLocation(), transform.GetLocation()) || !AIValidBuildingLocation(Faction, building, extent, transform.GetLocation()))
					continue;

				location = transform.GetLocation();
			}
		}
	}
	else if (building->IsA<AInternalProduction>() && Cast<AInternalProduction>(building)->ResourceToOverlap != nullptr) {
		AInternalProduction* internalBuilding = Cast<AInternalProduction>(building);

		TArray<FResourceHISMStruct> resourceStructs;
		resourceStructs.Append(Camera->Grid->MineralStruct);

		for (FResourceHISMStruct resourceStruct : resourceStructs) {
			if (internalBuilding->ResourceToOverlap != resourceStruct.ResourceClass)
				continue;

			for (int32 i = 0; i < resourceStruct.Resource->ResourceHISM->GetInstanceCount(); i++) {
				FTransform transform;
				resourceStruct.Resource->ResourceHISM->GetInstanceTransform(i, transform);

				if (FVector::Dist(Faction->EggTimer->GetActorLocation(), location) < FVector::Dist(Faction->EggTimer->GetActorLocation(), transform.GetLocation()) || !AIValidBuildingLocation(Faction, building, extent, transform.GetLocation()))
					continue;

				location = transform.GetLocation();
			}
		}
	}
	else if (building->IsA<ARoad>()) {
		int32 index = Camera->Stream.RandRange(0, Faction->RoadBuildLocations.Num() - 1);

		location = Faction->RoadBuildLocations[index];

		Faction->RoadBuildLocations.RemoveAt(index);
	}
	else {
		for (auto& element : Faction->AccessibleBuildLocations) {
			if (!AIValidBuildingLocation(Faction, building, extent, element.Key))
				continue;

			location = element.Key;

			break;
		}
	}

	if (location == FVector(10000000000.0f)) {
		building->DestroyBuilding();

		return;
	}

	Camera->BuildComponent->GetBuildLocationZ(building, location);
	building->SetActorRotation(rotation);
	building->SetActorLocation(location);
	building->Build();

	Camera->BuildComponent->SetTreeStatus(building, true);

	if (building->IsA<ARoad>())
		Cast<ARoad>(building)->RegenerateMesh();

	RemoveTileLocations(Faction, building);

	if (Houses.Contains(BuildingClass) || MiscBuilds.Contains(BuildingClass) || RoadClass == BuildingClass || RampClass == BuildingClass)
		return;

	FAIBuildStruct aibuild;
	aibuild.Building = BuildingClass;

	int32 i = AIBuilds.Find(aibuild);

	if (i == INDEX_NONE)
		return;

	AIBuilds[i].CurrentAmount++;
	AIBuilds[i].NumCitizens += AIBuilds[i].NumCitizensIncrement;
}