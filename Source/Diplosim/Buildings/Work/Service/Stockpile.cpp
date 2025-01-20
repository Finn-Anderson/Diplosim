#include "Buildings/Work/Service/Stockpile.h"

#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

#include "Player/Camera.h"
#include "PLayer/Managers/ResourceManager.h"
#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"

AStockpile::AStockpile()
{
	HISMBox = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMBox"));
	HISMBox->SetupAttachment(GetRootComponent());

	StorageCap = 1000;
}

void AStockpile::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	ACamera* camera = PController->GetPawn<ACamera>();

	TArray<TSubclassOf<AResource>> resources = camera->ResourceManager->GetResources(this);

	for (TSubclassOf<AResource> resource : resources)
		Store.Add(resource, true);
}

void AStockpile::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (!GetOccupied().Contains(Citizen))
		return;

	Gathering.Remove(Citizen);

	ShowBoxesInStockpile();
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

void AStockpile::SetItemToGather(TSubclassOf<class AResource> Resource, ACitizen* Citizen, ABuilding* Building)
{
	FItemStruct gather;
	gather.Resource = Resource;
	gather.Amount = 10;

	Gathering.Add(Citizen, gather);

	Citizen->AIController->AIMoveTo(Building);
}

FItemStruct AStockpile::GetItemToGather(class ACitizen* Citizen)
{
	FItemStruct gather;

	FItemStruct* item = Gathering.Find(Citizen);

	if (item == nullptr)
		return gather;

	bool* bCanGather = Store.Find(item->Resource);

	if (!bCanGather)
		return gather;

	return *item;
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