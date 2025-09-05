#include "ResourceManager.h"

#include "Kismet/GameplayStatics.h"

#include "Universal/Resource.h"
#include "Player/Camera.h"
#include "Player/Components/BuildComponent.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/ConstructionManager.h"
#include "Buildings/Building.h"
#include "Buildings/Work/Service/Stockpile.h"
#include "Buildings/House.h"
#include "Buildings/Work/Production/Farm.h"
#include "Universal/DiplosimGameModeBase.h"
#include "Universal/HealthComponent.h"
#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "AI/Citizen.h"

UResourceManager::UResourceManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UResourceManager::StoreBasket(TSubclassOf<AResource> Resource, ABuilding* Building)
{
	for (FBasketStruct product : Building->Basket) {
		if (product.Item.Resource != Resource)
			continue;

		Building->Camera->CitizenManager->RemoveTimer("Basket", Building);

		int32 extra = AddLocalResource(Resource, Building, product.Item.Amount);

		if (extra > 0) {
			Building->AddToBasket(Resource, extra);

			return;
		}
	}

	UpdateResourceUI(Cast<ACamera>(GetOwner())->ConquestManager->GetFaction(Building->FactionName), Resource);
}

FFactionResourceStruct* UResourceManager::GetFactionResourceStruct(FFactionStruct* Faction, TSubclassOf<class AResource> Resource)
{
	FFactionResourceStruct resourceStruct;
	resourceStruct.Type = Resource;

	int32 index = Faction->Resources.Find(resourceStruct);

	return &Faction->Resources[index];
}

void UResourceManager::AddCommittedResource(FFactionStruct* Faction, TSubclassOf<AResource> Resource, int32 Amount)
{
	FFactionResourceStruct* resourceStruct = GetFactionResourceStruct(Faction, Resource);
	resourceStruct->Committed += Amount;

	UpdateResourceUI(Faction, Resource);
}

void UResourceManager::TakeCommittedResource(FFactionStruct* Faction, TSubclassOf<AResource> Resource, int32 Amount)
{
	FFactionResourceStruct* resourceStruct = GetFactionResourceStruct(Faction, Resource);
	resourceStruct->Committed -= Amount;
}

int32 UResourceManager::AddLocalResource(TSubclassOf<AResource> Resource, ABuilding* Building, int32 Amount)
{
	GameMode->TallyEnemyData(Resource, Amount);

	FItemStruct itemStruct;
	itemStruct.Resource = Resource;

	int32 index = Building->Storage.Find(itemStruct);

	int32 currentAmount = 0;

	for (FItemStruct item : Building->Storage)
		currentAmount += item.Amount;

	int32 target = currentAmount + Amount;
	int32 storageCap = *GetBuildingCapacities(Building, Resource).Find(Resource);

	if (target > storageCap) {
		Building->Storage[index].Amount = storageCap;

		Amount = target - storageCap;
	}
	else {
		Building->Storage[index].Amount = target;

		Amount = 0;
	}

	GetNearestStockpile(Resource, Building, Building->Storage[index].Amount);

	UpdateResourceUI(Cast<ACamera>(GetOwner())->ConquestManager->GetFaction(Building->FactionName), Resource);

	return Amount;
}

bool UResourceManager::AddUniversalResource(FFactionStruct* Faction, TSubclassOf<AResource> Resource, int32 Amount)
{
	GameMode->TallyEnemyData(Resource, Amount);

	int32 stored = 0;
	int32 capacity = 0;

	FResourceStruct resourceStruct;
	resourceStruct.Type = Resource;

	int32 index = ResourceList.Find(resourceStruct);

	TMap<ABuilding*, int32> foundBuildings;
	for (auto& element : ResourceList[index].Buildings)
		for (ABuilding* building : GetBuildingsOfClass(Faction, element.Key))
			foundBuildings.Add(building, *GetBuildingCapacities(building, Resource).Find(Resource));

	for (auto& element : foundBuildings) {
		int32 currentAmount = 0;

		for (FItemStruct item : element.Key->Storage)
			currentAmount += item.Amount;

		stored += currentAmount;
		capacity += element.Value;
	}

	if (stored == capacity)
		return false;

	int32 AmountLeft = Amount;
	for (auto& element : foundBuildings) {
		FItemStruct itemStruct;
		itemStruct.Resource = Resource;

		index = element.Key->Storage.Find(itemStruct);

		AmountLeft -= (element.Value - element.Key->Storage[index].Amount);

		int32 currentAmount = 0;

		for (FItemStruct item : element.Key->Storage)
			currentAmount += item.Amount;

		element.Key->Storage[index].Amount += FMath::Clamp(Amount, 0, element.Value - currentAmount);

		GetNearestStockpile(Resource, element.Key, element.Key->Storage[index].Amount);

		if (AmountLeft <= 0) {
			UpdateResourceUI(Faction, Resource);

			return true;
		}
	}

	UpdateResourceUI(Faction, Resource);

	return false;
}

bool UResourceManager::TakeLocalResource(TSubclassOf<AResource> Resource, ABuilding* Building, int32 Amount)
{
	FItemStruct itemStruct;
	itemStruct.Resource = Resource;

	int32 index = Building->Storage.Find(itemStruct);

	int32 target = Building->Storage[index].Amount - Amount;

	if (target < 0)
		return false;

	Building->Storage[index].Amount = target;

	if (!Building->Basket.IsEmpty())
		StoreBasket(Resource, Building);

	UpdateResourceUI(Cast<ACamera>(GetOwner())->ConquestManager->GetFaction(Building->FactionName), Resource);

	return true;
}

bool UResourceManager::TakeUniversalResource(FFactionStruct* Faction, TSubclassOf<AResource> Resource, int32 Amount, int32 Min)
{
	int32 stored = 0;

	FResourceStruct resourceStruct;
	resourceStruct.Type = Resource;

	int32 index = ResourceList.Find(resourceStruct);

	TArray<ABuilding*> foundBuildings;
	for (auto& element : ResourceList[index].Buildings)
		foundBuildings.Append(GetBuildingsOfClass(Faction, element.Key));

	for (ABuilding* building : foundBuildings) {
		FItemStruct itemStruct;
		itemStruct.Resource = Resource;

		index = building->Storage.Find(itemStruct);

		stored += building->Storage[index].Amount;
	}

	int32 target = stored - Amount - Min;

	if (target < 0)
		return false;

	int32 AmountLeft = Amount;
	for (ABuilding* building : foundBuildings) {
		FItemStruct itemStruct;
		itemStruct.Resource = Resource;

		index = building->Storage.Find(itemStruct);

		AmountLeft -= building->Storage[index].Amount - Min;

		building->Storage[index].Amount = FMath::Clamp(building->Storage[index].Amount - Amount, Min, 1000);

		if (!building->Basket.IsEmpty())
			StoreBasket(Resource, building);

		if (AmountLeft <= 0) {
			UpdateResourceUI(Faction, Resource);

			return true;
		}
	}

	UpdateResourceUI(Faction, Resource);

	return false;
}

int32 UResourceManager::GetResourceAmount(FString FactionName, TSubclassOf<AResource> Resource)
{
	FFactionStruct* faction = Cast<ACamera>(GetOwner())->ConquestManager->GetFaction(FactionName);

	int32 amount = -GetFactionResourceStruct(faction, Resource)->Committed;
	int32 committed = 0;

	FResourceStruct resourceStruct;
	resourceStruct.Type = Resource;

	int32 index = ResourceList.Find(resourceStruct);

	for (auto& element : ResourceList[index].Buildings) {
		TArray<ABuilding*> foundBuildings = GetBuildingsOfClass(faction, element.Key);

		for (ABuilding* building : foundBuildings) {
			FItemStruct itemStruct;
			itemStruct.Resource = Resource;

			index = building->Storage.Find(itemStruct);

			amount += building->Storage[index].Amount;
		}
	}

	return amount;
}

int32 UResourceManager::GetResourceCapacity(FString FactionName, TSubclassOf<class AResource> Resource)
{
	FFactionStruct* faction = Cast<ACamera>(GetOwner())->ConquestManager->GetFaction(FactionName);

	int32 capacity = 0;

	FResourceStruct resourceStruct;
	resourceStruct.Type = Resource;

	int32 index = ResourceList.Find(resourceStruct);

	for (auto &element : ResourceList[index].Buildings) {
		TArray<ABuilding*> foundBuildings = GetBuildingsOfClass(faction, element.Key);

		for (ABuilding* building : foundBuildings)
			capacity += *GetBuildingCapacities(building, Resource).Find(Resource);
	}

	return capacity;
}

float UResourceManager::GetBuildingCapacityPercentage(ABuilding* Building)
{
	int32 amount = 0;
	float capacity = 0.0f;

	for (FResourceStruct resourceStruct : ResourceList) {
		if (!resourceStruct.Buildings.Contains(Building->GetClass()))
			continue;

		FItemStruct item;
		item.Resource = resourceStruct.Type;

		int32 index = Building->Storage.Find(item);

		if (index != INDEX_NONE)
			amount += Building->Storage[index].Amount;

		capacity += *resourceStruct.Buildings.Find(Building->GetClass());
	}

	return amount / capacity;
}

TMap<TSubclassOf<class AResource>, int32> UResourceManager::GetBuildingCapacities(ABuilding* Building, TSubclassOf<class AResource> Resource)
{
	TMap<TSubclassOf<class AResource>, int32> capacities;

	for (FResourceStruct resource : ResourceList) {
		if ((Resource != nullptr && resource.Type != Resource) || !resource.Buildings.Contains(Building->GetClass()))
			continue;

		if (Building->IsA<AStockpile>()) {
			AStockpile* stockpile = Cast<AStockpile>(Building);
			int32 capacity = stockpile->StorageCap;

			for (FItemStruct item : stockpile->Storage) {
				if (item.Resource == resource.Type)
					continue;

				capacity -= item.Amount;
			}

			capacities.Add(resource.Type, capacity);
		}
		else
			capacities.Add(resource.Type, *resource.Buildings.Find(Building->GetClass()));
	}

	return capacities;
}

TArray<TSubclassOf<AResource>> UResourceManager::GetResources(ABuilding* Building)
{
	TArray<TSubclassOf<AResource>> resources;

	for (FResourceStruct resource : ResourceList)
		for (auto& element : resource.Buildings)
			if (element.Key == Building->GetClass())
				resources.Add(resource.Type);

	return resources;
}

TMap<TSubclassOf<class ABuilding>, int32> UResourceManager::GetBuildings(TSubclassOf<AResource> Resource)
{
	for (FResourceStruct resource : ResourceList)
		if (resource.Type == Resource)
			return resource.Buildings;

	TMap<TSubclassOf<class ABuilding>, int32> null;

	return null;
}

TArray<ABuilding*> UResourceManager::GetBuildingsOfClass(FFactionStruct* Faction, TSubclassOf<AActor> Class)
{
	TArray<ABuilding*> foundBuildings;

	ACamera* camera = Cast<ACamera>(GetOwner());

	for (ABuilding* building : Faction->Buildings) {
		if (building->GetClass() != Class || camera->BuildComponent->Buildings.Contains(building) || camera->ConstructionManager->IsBeingConstructed(building, nullptr) || building->HealthComponent->GetHealth() == 0)
			continue;

		foundBuildings.Add(building);
	}

	return foundBuildings;
}

void UResourceManager::GetNearestStockpile(TSubclassOf<AResource> Resource, ABuilding* Building, int32 Amount)
{
	FFactionStruct* faction = Cast<ACamera>(GetOwner())->ConquestManager->GetFaction(Building->FactionName);

	FResourceStruct resourceStruct;
	resourceStruct.Type = Resource;

	int32 index = ResourceList.Find(resourceStruct);

	if (ResourceList[index].Category == "Money" || ResourceList[index].Category == "Crystal")
		return;

	Async(EAsyncExecution::TaskGraph, [this, Resource, Building, Amount, faction]() {
		ACamera* camera = Cast<ACamera>(GetOwner());
		int32 workersNum = FMath::CeilToInt(Amount / 10.0f);
		AStockpile* nearestStockpile = nullptr;
		TArray<AStockpile*> stockpiles;

		for (ABuilding* building : faction->Buildings) {
			if (!building->IsA<AStockpile>())
				continue;

			AStockpile* stockpile = Cast<AStockpile>(building); 
			
			if (!stockpile->DoesStoreResource(Resource))
				continue;

			int32 index = 0;

			for (AStockpile* sp : stockpiles) {
				double stockpileDist = FVector::Dist(stockpile->GetActorLocation(), Building->GetActorLocation());
				double spDist = FVector::Dist(sp->GetActorLocation(), Building->GetActorLocation());

				if (stockpileDist > spDist)
					index++;
				else
					break;
			}

			stockpiles.Insert(stockpile, index);
		}

		TMap<AStockpile*, ACitizen*> workers;

		[&] {
			for (AStockpile* stockpile : stockpiles) {
				for (ACitizen* citizen : stockpile->GetCitizensAtBuilding()) {
					if (stockpile->Gathering.Contains(citizen))
						continue;

					workers.Add(stockpile, citizen);

					if (workers.Num() == workersNum)
						return;
				}
			}
		}();

		for (auto& element : workers)
			element.Key->SetItemToGather(Resource, element.Value, Building);
	});
}

void UResourceManager::UpdateResourceUI(FFactionStruct* Faction, TSubclassOf<AResource> Resource)
{
	if (Faction->Name != Cast<ACamera>(GetOwner())->ColonyName)
		return;

	Async(EAsyncExecution::TaskGraphMainTick, [this, Resource]() {
		FResourceStruct resourceStruct;
		resourceStruct.Type = Resource;

		int32 index = ResourceList.Find(resourceStruct);

		Cast<ACamera>(GetOwner())->UpdateResourceText(ResourceList[index].Category);
	});
}

void UResourceManager::UpdateResourceCapacityUI(ABuilding* Building)
{
	ACamera* camera = Cast<ACamera>(GetOwner());

	if (Building->FactionName != camera->ColonyName)
		return;

	TArray<TSubclassOf<AResource>> resources = GetResources(Building);

	for (TSubclassOf<AResource> resource : resources) {
		FResourceStruct resourceStruct;
		resourceStruct.Type = resource;

		int32 index = ResourceList.Find(resourceStruct);

		Cast<ACamera>(GetOwner())->UpdateResourceCapacityText(ResourceList[index].Category);
	}
}

TArray<TSubclassOf<AResource>> UResourceManager::GetResourcesFromCategory(FString Category)
{
	TArray<TSubclassOf<AResource>> resources;

	for (FResourceStruct resource : ResourceList) {
		if (resource.Category != Category)
			continue;

		resources.Add(resource.Type);
	}

	return resources;
}

//
// Trade
//
void UResourceManager::RandomiseMarket()
{
	for (FResourceStruct& resource : ResourceList) {
		int32 value = Cast<ACamera>(GetOwner())->Grid->Stream.RandRange(50, 5000);

		resource.Stored = value;
		resource.Value = 1 / FMath::Min(resource.Stored / 5000.0f, 1.0f);
	}
}

void UResourceManager::SetTradeValues()
{
	for (FResourceStruct& resource : ResourceList) {
		int32 gain = Cast<ACamera>(GetOwner())->Grid->Stream.RandRange(-resource.Stored * 0.1f, resource.Stored * 0.1f + 10);

		resource.Stored += gain;
		resource.Value = 1 / FMath::Min(resource.Stored / 5000.0f, 1.0f);
	}
}

int32 UResourceManager::GetStoredOnMarket(TSubclassOf<class AResource> Resource)
{
	for (FResourceStruct resource : ResourceList)
		if (resource.Type == Resource)
			return resource.Stored;

	return 0;
}

int32 UResourceManager::GetMarketValue(TSubclassOf<class AResource> Resource)
{
	for (FResourceStruct resource : ResourceList)
		if (resource.Type == Resource)
			return resource.Value;

	return 0;
}

//
// Trends
//
void UResourceManager::SetTrendOnHour(int32 Hour)
{
	for (FFactionStruct& faction : Cast<ACamera>(GetOwner())->ConquestManager->Factions) {
		for (FResourceStruct resource : ResourceList) {
			int32 amount = GetResourceAmount(faction.Name, resource.Type);

			FFactionResourceStruct* resourceStruct = GetFactionResourceStruct(&faction, resource.Type);

			int32 change = amount - resourceStruct->LastHourAmount;

			resourceStruct->HourlyTrend.Emplace(Hour, change);

			resourceStruct->LastHourAmount = amount;
		}

		if (faction.Name == Cast<ACamera>(GetOwner())->ColonyName)
			Cast<ACamera>(GetOwner())->UpdateTrends();
	}
}

int32 UResourceManager::GetResourceTrend(FString FactionName, TSubclassOf<class AResource> Resource)
{
	FFactionStruct* faction = Cast<ACamera>(GetOwner())->ConquestManager->GetFaction(FactionName);
	FFactionResourceStruct* resourceStruct = GetFactionResourceStruct(faction, Resource);

	int32 overallTrend = 0;

	for (auto& element : resourceStruct->HourlyTrend)
		overallTrend += element.Value;

	return overallTrend;
}

int32 UResourceManager::GetCategoryTrend(FString FactionName, FString Category)
{
	FFactionStruct* faction = Cast<ACamera>(GetOwner())->ConquestManager->GetFaction(FactionName);

	int32 overallTrend = 0;

	if (Category == "Money") {
		for (ABuilding* building : faction->Buildings) {
			if (!IsValid(building))
				continue;

			if (building->IsA<AWork>())
				for (ACitizen* citizen : Cast<AWork>(building)->GetOccupied())
					overallTrend -= Cast<AWork>(building)->GetWage(citizen);
			else if (building->IsA<AHouse>())
				overallTrend += Cast<AHouse>(building)->Rent * building->GetOccupied().Num();
		}
	}
	else if (Category == "Food") {
		overallTrend -= faction->Citizens.Num() * 8;

		for (ABuilding* building : faction->Buildings) {
			if (!IsValid(building) || !building->IsA<AFarm>() || building->GetOccupied().IsEmpty())
				continue;

			AFarm* farm = Cast<AFarm>(building);
			ACitizen* citizen = farm->GetOccupied()[0];

			int32 timeLength = farm->TimeLength / citizen->GetProductivity();

			int32 timePerHour = 360 / (24 * Cast<ACamera>(GetOwner())->Grid->AtmosphereComponent->Speed) / 24;

			int32 hours = farm->GetHoursInADay(citizen);

			int32 workdayTime = hours * timePerHour;
			int32 amount = workdayTime / timeLength;

			overallTrend += (amount * farm->GetYield());
		}
	}
	else {
		for (FResourceStruct &resource : ResourceList) {
			if (resource.Category != Category)
				continue;

			FFactionResourceStruct* resourceStruct = GetFactionResourceStruct(faction, resource.Type);

			for (auto& element : resourceStruct->HourlyTrend)
				overallTrend += element.Value;
		}
	}

	return overallTrend;
}