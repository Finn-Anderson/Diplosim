#include "ResourceManager.h"

#include "Kismet/GameplayStatics.h"

#include "Universal/Resource.h"
#include "Player/Camera.h"
#include "Buildings/Building.h"
#include "Universal/DiplosimGameModeBase.h"

UResourceManager::UResourceManager()
{
	PrimaryComponentTick.bCanEverTick = false;
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

bool UResourceManager::AddLocalResource(ABuilding* Building, int32 Amount)
{
	GameMode->TallyEnemyData(GetResource(Building), Amount);

	int32 target = Building->Storage + Amount;

	bool space = true;

	if (target > Building->StorageCap) {
		Building->Storage = Building->StorageCap;

		space = false;
	}
	else {
		Building->Storage = target;
	}

	return space;
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

					stored += b->Storage;
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

					AmountLeft -= (b->StorageCap - b->Storage);

					b->Storage = FMath::Clamp(b->Storage + Amount, 0, 1000);

					if (AmountLeft <= 0)
						return true;
				}
			}
		}
	}

	return false;
}

bool UResourceManager::TakeLocalResource(ABuilding* Building, int32 Amount)
{
	int32 target = Building->Storage - Amount;

	if (target < 0)
		return false;

	Building->Storage = target;

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

					stored += b->Storage;
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

					AmountLeft -= b->Storage - Min;

					b->Storage = FMath::Clamp(b->Storage - Amount, Min, 1000);

					if (AmountLeft <= 0)
						return true;
				}
			}

			break;
		}
	}

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

					amount += b->Storage;
				}
			}

			break;
		}
	}

	return amount;
}

TSubclassOf<AResource> UResourceManager::GetResource(ABuilding* Building)
{
	TSubclassOf<AResource> resource = nullptr;

	for (int32 i = 0; i < ResourceList.Num(); i++) {
		for (int32 j = 0; j < ResourceList[i].Buildings.Num(); j++) {
			if (ResourceList[i].Buildings[j] == Building->GetClass()) {
				resource = ResourceList[i].Type;

				break;
			}
		}
	}

	return resource;
}

TArray<TSubclassOf<class ABuilding>> UResourceManager::GetBuildings(TSubclassOf<class AResource> Resource) 
{
	for (int32 i = 0; i < ResourceList.Num(); i++)
		if (ResourceList[i].Type == Resource)
			return ResourceList[i].Buildings;

	TArray<TSubclassOf<class ABuilding>> null;

	return null;
}

void UResourceManager::Interest()
{
	int32 amount = GetResourceAmount(Money);

	amount = FMath::CeilToInt32(amount * 0.02f);

	AddUniversalResource(Money, amount);
}