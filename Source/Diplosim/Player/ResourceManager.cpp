#include "ResourceManager.h"

#include "Resource.h"
#include "Camera.h"

UResourceManager::UResourceManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UResourceManager::ChangeResource(TSubclassOf<AResource> Resource, int32 Change)
{
	for (int i = 0; i < ResourceList.Num(); i++) {
		if (ResourceList[i].Type == Resource) {
			ResourceList[i].Amount += Change;

			ResourceStruct = ResourceList[i];

			ACamera* player = Cast<ACamera>(GetOwner());
			player->UpdateDisplay();

			return;
		}
	}
}

int32 UResourceManager::GetResourceAmount(TSubclassOf<AResource> Resource)
{
	for (int i = 0; i < ResourceList.Num(); i++) {
		if (ResourceList[i].Type == Resource) {
			return ResourceList[i].Amount;
		}
	}

	return 0;
}

FResourceStruct UResourceManager::GetUpdatedResource()
{
	return ResourceStruct;
}