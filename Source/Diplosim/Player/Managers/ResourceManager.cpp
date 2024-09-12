#include "ResourceManager.h"

#include "Kismet/GameplayStatics.h"

#include "Universal/Resource.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Buildings/Building.h"
#include "Universal/DiplosimGameModeBase.h"

UResourceManager::UResourceManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UResourceManager::StoreBasket(TSubclassOf<class AResource> Resource, class ABuilding* Building)
{
	for (FBasketStruct product : Building->Basket) {
		if (product.Item.Resource != Resource)
			continue;

		Building->Camera->CitizenManager->RemoveTimer(Building, product.TimerDelegate);

		int32 extra = AddLocalResource(Resource, Building, product.Item.Amount);

		if (extra > 0) {
			Building->AddToBasket(Resource, extra);

			return;
		}
	}
}

void UResourceManager::AddCommittedResource(TSubclassOf<class AResource> Resource, int32 Amount)
{
	for (int32 i = 0; i < ResourceList.Num(); i++) {
		if (ResourceList[i].Type == Resource) {
			ResourceList[i].Committed += Amount;

			break;
		}
	}
}

void UResourceManager::TakeCommittedResource(TSubclassOf<class AResource> Resource, int32 Amount)
{
	for (int32 i = 0; i < ResourceList.Num(); i++) {
		if (ResourceList[i].Type == Resource) {
			ResourceList[i].Committed -= Amount;

			break;
		}
	}
}

int32 UResourceManager::AddLocalResource(TSubclassOf<class AResource> Resource, ABuilding* Building, int32 Amount)
{
	GameMode->TallyEnemyData(Resource, Amount);

	FItemStruct itemStruct;
	itemStruct.Resource = Resource;

	int32 index = Building->Storage.Find(itemStruct);

	int32 currentAmount = 0;

	for (FItemStruct item : Building->Storage)
		currentAmount += item.Amount;

	int32 target = currentAmount + Amount;

	if (target > Building->StorageCap) {
		Building->Storage[index].Amount = Building->StorageCap;

		Amount = target - Building->StorageCap;
	}
	else {
		Building->Storage[index].Amount = target;

		Amount = 0;
	}

	UpdateResourceUI(Resource);

	return Amount;
}

bool UResourceManager::AddUniversalResource(TSubclassOf<AResource> Resource, int32 Amount)
{
	GameMode->TallyEnemyData(Resource, Amount);

	int32 stored = 0;
	int32 capacity = 0;

	for (int32 i = 0; i < ResourceList.Num(); i++) {
		if (ResourceList[i].Type == Resource) {
			for (int32 j = 0; j < ResourceList[i].Buildings.Num(); j++) {
				TArray<AActor*> foundBuildings;
				UGameplayStatics::GetAllActorsOfClass(GetWorld(), ResourceList[i].Buildings[j], foundBuildings);

				for (int32 k = 0; k < foundBuildings.Num(); k++) {
					ABuilding* b = Cast<ABuilding>(foundBuildings[k]);

					int32 currentAmount = 0;

					for (FItemStruct item : b->Storage)
						currentAmount += item.Amount;

					stored += currentAmount;
					capacity += b->StorageCap;
				}
			}

			break;
		}
	}

	int32 target = stored + Amount;

	if (target > capacity)
		return false;

	int32 AmountLeft = Amount;
	for (int32 i = 0; i < ResourceList.Num(); i++) {
		if (ResourceList[i].Type == Resource) {
			for (int32 j = 0; j < ResourceList[i].Buildings.Num(); j++) {
				TArray<AActor*> foundBuildings;
				UGameplayStatics::GetAllActorsOfClass(GetWorld(), ResourceList[i].Buildings[j], foundBuildings);

				for (int32 k = 0; k < foundBuildings.Num(); k++) {
					ABuilding* b = Cast<ABuilding>(foundBuildings[k]);

					FItemStruct itemStruct;
					itemStruct.Resource = Resource;

					int32 index = b->Storage.Find(itemStruct);

					AmountLeft -= (b->StorageCap - b->Storage[index].Amount);

					b->Storage[index].Amount = FMath::Clamp(b->Storage[index].Amount + Amount, 0, 1000);

					if (AmountLeft <= 0) {
						UpdateResourceUI(Resource);

						return true;
					}
				}
			}
		}
	}

	UpdateResourceUI(Resource);

	return false;
}

bool UResourceManager::TakeLocalResource(TSubclassOf<class AResource> Resource, ABuilding* Building, int32 Amount)
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

	UpdateResourceUI(Resource);

	return true;
}

bool UResourceManager::TakeUniversalResource(TSubclassOf<AResource> Resource, int32 Amount, int32 Min)
{
	int32 stored = 0;

	for (int32 i = 0; i < ResourceList.Num(); i++) {
		if (ResourceList[i].Type == Resource) {
			for (int32 j = 0; j < ResourceList[i].Buildings.Num(); j++) {
				TArray<AActor*> foundBuildings;
				UGameplayStatics::GetAllActorsOfClass(GetWorld(), ResourceList[i].Buildings[j], foundBuildings);

				for (int32 k = 0; k < foundBuildings.Num(); k++) {
					ABuilding* b = Cast<ABuilding>(foundBuildings[k]);

					FItemStruct itemStruct;
					itemStruct.Resource = Resource;

					int32 index = b->Storage.Find(itemStruct);

					stored += b->Storage[index].Amount;
				}
			}

			break;
		}
	}

	int32 target = stored - Amount - Min;

	if (target < 0)
		return false;

	int32 AmountLeft = Amount;
	for (int32 i = 0; i < ResourceList.Num(); i++) {
		if (ResourceList[i].Type == Resource) {
			for (int32 j = 0; j < ResourceList[i].Buildings.Num(); j++) {
				TArray<AActor*> foundBuildings;
				UGameplayStatics::GetAllActorsOfClass(GetWorld(), ResourceList[i].Buildings[j], foundBuildings);

				for (int32 k = 0; k < foundBuildings.Num(); k++) {
					ABuilding* b = Cast<ABuilding>(foundBuildings[k]);

					FItemStruct itemStruct;
					itemStruct.Resource = Resource;

					int32 index = b->Storage.Find(itemStruct);

					AmountLeft -= b->Storage[index].Amount - Min;

					b->Storage[index].Amount = FMath::Clamp(b->Storage[index].Amount - Amount, Min, 1000);

					if (!b->Basket.IsEmpty())
						StoreBasket(Resource, b);

					if (AmountLeft <= 0) {
						UpdateResourceUI(Resource);

						return true;
					}
				}
			}

			break;
		}
	}

	UpdateResourceUI(Resource);

	return false;
}

int32 UResourceManager::GetResourceAmount(TSubclassOf<AResource> Resource)
{
	int32 amount = 0;
	int32 committed = 0;

	for (int32 i = 0; i < ResourceList.Num(); i++) {
		if (ResourceList[i].Type == Resource) {
			amount -= ResourceList[i].Committed;

			for (int32 j = 0; j < ResourceList[i].Buildings.Num(); j++) {
				TArray<AActor*> foundBuildings;
				UGameplayStatics::GetAllActorsOfClass(GetWorld(), ResourceList[i].Buildings[j], foundBuildings);

				for (int32 k = 0; k < foundBuildings.Num(); k++) {
					ABuilding* b = Cast<ABuilding>(foundBuildings[k]);

					FItemStruct itemStruct;
					itemStruct.Resource = Resource;

					int32 index = b->Storage.Find(itemStruct);

					amount += b->Storage[index].Amount;
				}
			}

			break;
		}
	}

	return amount;
}

TArray<TSubclassOf<AResource>> UResourceManager::GetResources(ABuilding* Building)
{
	TArray<TSubclassOf<AResource>> resources;

	for (int32 i = 0; i < ResourceList.Num(); i++)
		for (int32 j = 0; j < ResourceList[i].Buildings.Num(); j++)
			if (ResourceList[i].Buildings[j] == Building->GetClass())
				resources.Add(ResourceList[i].Type);

	return resources;
}

TArray<TSubclassOf<class ABuilding>> UResourceManager::GetBuildings(TSubclassOf<class AResource> Resource) 
{
	for (int32 i = 0; i < ResourceList.Num(); i++)
		if (ResourceList[i].Type == Resource)
			return ResourceList[i].Buildings;

	TArray<TSubclassOf<class ABuilding>> null;

	return null;
}

void UResourceManager::UpdateResourceUI(TSubclassOf<class AResource> Resource)
{
	FResourceStruct resourceStruct;
	resourceStruct.Type = Resource;

	int32 index = ResourceList.Find(resourceStruct);

	Cast<ACamera>(GetOwner())->UpdateResourceText(ResourceList[index].Category);
}

//
// Interest
//
void UResourceManager::Interest()
{
	int32 amount = GetResourceAmount(Money);

	amount = FMath::CeilToInt32(amount * 0.02f);

	AddUniversalResource(Money, amount);
}

//
// Trade
//
void UResourceManager::RandomiseMarket()
{
	for (FResourceStruct& resource : ResourceList) {
		int32 value = FMath::RandRange(50, 5000);

		resource.Stored = value;
		resource.Value = 1 / FMath::Min(resource.Stored / 5000.0f, 1.0f);
	}
}

void UResourceManager::SetTradeValues()
{
	for (FResourceStruct& resource : ResourceList) {
		int32 gain = FMath::RandRange(-resource.Stored * 0.1f, resource.Stored * 0.1f + 10);

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