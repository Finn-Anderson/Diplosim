#include "Buildings/Work/Service/Stockpile.h"

#include "Kismet/GameplayStatics.h"

#include "Player/Camera.h"
#include "PLayer/Managers/ResourceManager.h"
#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"

AStockpile::AStockpile()
{

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