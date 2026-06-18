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
	Capacity = 0;
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

	ShowBoxesInStockpile();
}

void AStockpile::ShowBoxesInStockpile()
{
	float instPerStorage = StorageCap / 32.0f;
	
	int32 stored = 0;

	for (const FItemStruct& item : Storage)
		stored += item.Amount;

	int32 instances =  FMath::CeilToInt(stored / instPerStorage);

	if (instances < HISMBox->GetInstanceCount()) {
		for (int32 i = HISMBox->GetInstanceCount() - 1; i > instances - 1; i--)
			HISMBox->RemoveInstance(i);
	}
	else {
		for (int32 i = HISMBox->GetInstanceCount(); i < instances; i++) {
			FTransform transform;
			transform.SetLocation(BuildingMesh->GetSocketLocation(*FString::FromInt(i)));

			HISMBox->AddInstance(transform);
		}
	}
}

void AStockpile::SetStoreResoruce(TSubclassOf<class AResource> Resource, bool bStore)
{
	Store.Add(Resource, bStore);

	Camera->ResourceManager->UpdateResourceCapacityUI(this);
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

bool AStockpile::IsCapacityFull()
{
	return GetFreeStorage() == 0;
}