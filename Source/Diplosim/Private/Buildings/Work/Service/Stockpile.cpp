#include "Buildings/Work/Service/Stockpile.h"

#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

#include "AI/Citizen/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Universal/HealthComponent.h"

AStockpile::AStockpile()
{
	HealthComponent->MaxHealth = 200;
	HealthComponent->Health = HealthComponent->MaxHealth;
	
	HISMBox = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMBox"));
	HISMBox->SetupAttachment(GetRootComponent());

	StorageCap = 2000;
}

void AStockpile::BeginPlay()
{
	Super::BeginPlay();

	TArray<TSubclassOf<AResource>> resources = Camera->ResourceManager->GetResources(this);

	for (TSubclassOf<AResource> resource : resources)
		Store.Add(resource, true);
}

void AStockpile::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (!GetOccupied().Contains(Citizen))
		return;

	ShowBoxesInStockpile();
}

bool AStockpile::IsAtWork(ACitizen* Citizen)
{
	bool bWorking = Super::IsAtWork(Citizen);

	if (!bWorking) {
		AActor* goal = Citizen->AIController->MoveRequest.GetGoalActor();

		if (IsValid(goal)) {
			TArray<TSubclassOf<AResource>> resources = Camera->ResourceManager->GetResources(this);

			for (TSubclassOf<AResource> resource : resources) {
				TMap<TSubclassOf<ABuilding>, int32> buildingTypes = Camera->ResourceManager->GetBuildings(resource);

				for (auto& element : buildingTypes) {
					if (!goal->IsA(element.Key))
						continue;

					bWorking = true;

					break;
				}

				if (bWorking)
					break;
			}
		}
	}

	return bWorking;
}

void AStockpile::ShowBoxesInStockpile()
{
	float instPerStorage = StorageCap / 32.0f;
	
	int32 stored = 0;

	for (FItemStruct item : Storage)
		stored += item.Amount;

	int32 instances =  FMath::CeilToInt(stored / instPerStorage);

	if (instances < HISMBox->GetInstanceCount()) {
		for (int32 i = HISMBox->GetInstanceCount() - 1; i > instances - 1; i--)
			HISMBox->RemoveInstance(i);
	}
	else {
		for (int32 i = 0; i < instances - HISMBox->GetInstanceCount(); i++) {
			FTransform transform;
			transform.SetLocation(BuildingMesh->GetSocketLocation(*FString::FromInt(i)));

			HISMBox->AddInstance(transform);
		}
	}
}

bool AStockpile::DoesStoreResource(TSubclassOf<class AResource> Resource)
{
	bool* bCan = Store.Find(Resource);

	if (bCan == nullptr)
		return false;

	return *bCan;
}

int32 AStockpile::GetStoredResourceAmount(TSubclassOf<AResource> Resource)
{
	FItemStruct item;
	item.Resource = Resource;

	int32 index = Storage.Find(item);

	if (index == INDEX_NONE)
		return 0;

	return Storage[index].Amount;
}

int32 AStockpile::GetFreeStorage()
{
	int32 amount = StorageCap;

	for (auto element : Store)
		amount -= GetStoredResourceAmount(element.Key);

	return amount;
}
