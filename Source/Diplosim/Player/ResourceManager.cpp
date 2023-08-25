#include "ResourceManager.h"

#include "Kismet/GameplayStatics.h"

#include "Resource.h"
#include "Camera.h"
#include "Buildings/Building.h"

UResourceManager::UResourceManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UResourceManager::AddLocalResource(ABuilding* Building, int32 Amount)
{
	// No worky work.
	int32 target = Building->Storage + Amount;

	if (target > Building->Capacity)
		return false;

	Building->Storage = target;

	for (int32 i = 0; i < ResourceList.Num(); i++) {
		if (ResourceList[i].Buildings.Contains(Building->GetClass())) {
			SetResourceStruct(ResourceList[i].Type);

			break;
		}
	}

	return true;
}

bool UResourceManager::AddUniversalResource(TSubclassOf<AResource> Resource, int32 Amount)
{
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
					capacity += b->Capacity;
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

					AmountLeft -= (b->Capacity - b->Storage);

					b->Storage = FMath::Clamp(b->Storage + Amount, 0, 1000);

					if (AmountLeft <= 0)
						SetResourceStruct(Resource);
						return true;
				}
			}
		}
	}

	return false;
}

bool UResourceManager::TakeResource(TSubclassOf<AResource> Resource, int32 Amount)
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

	int32 target = stored - Amount;

	if (target < 0)
		return false;

	SetResourceStruct(Resource);

	int32 AmountLeft = Amount;
	for (int32 i = 0; i < ResourceList.Num(); i++) {
		if (ResourceList[i].Type == Resource) {
			for (int32 j = 0; j < ResourceList[i].Buildings.Num(); j++) {
				TArray<AActor*> foundBuildings;
				UGameplayStatics::GetAllActorsOfClass(GetWorld(), ResourceList[i].Buildings[j], foundBuildings);

				for (int32 k = 0; k < foundBuildings.Num(); k++) {
					ABuilding* b = Cast<ABuilding>(foundBuildings[k]);

					AmountLeft -= b->Storage;

					b->Storage = FMath::Clamp(b->Storage - Amount, 0, 1000);

					if (AmountLeft <= 0)
						SetResourceStruct(Resource);
						return true;
				}
			}
		}
	}

	return false;
}

void UResourceManager::SetResourceStruct(TSubclassOf<AResource> Resource)
{
	for (int32 i = 0; i < ResourceList.Num(); i++) {
		if (ResourceList[i].Type == Resource) {
			ResourceStruct = ResourceList[i];

			ACamera* player = Cast<ACamera>(GetOwner());
			player->UpdateDisplay();

			return;
		}
	}
}

int32 UResourceManager::GetResourceAmount(TSubclassOf<AResource> Resource)
{
	int32 amount = 0;

	for (int32 i = 0; i < ResourceList.Num(); i++) {
		if (ResourceList[i].Type == Resource) {
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

TSubclassOf<class AResource> UResourceManager::GetResource(ABuilding* Building) {
	for (int32 i = 0; i < ResourceList.Num(); i++) {
		for (int32 j = 0; j < ResourceList[i].Buildings.Num(); j++) {
			if (ResourceList[i].Buildings[j] == Building->GetClass()) {
				return ResourceList[i].Type;
			}
		}
	}

	return 0;
}

FResourceStruct UResourceManager::GetUpdatedResource()
{
	return ResourceStruct;
}