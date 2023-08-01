#include "ResourceManager.h"

#include "Resource.h"

UResourceManager::UResourceManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UResourceManager::ChangeResource(TSubclassOf<AResource> Resource, int32 Change)
{
	for (int i = 0; i < ResourceTypes.Num(); i++) {
		if (ResourceTypes[i] == Resource) {
			ResourceAmounts[i] += Change;

			return;
		}
	}
}

int32 UResourceManager::GetResource(TSubclassOf<AResource> Resource)
{
	for (int i = 0; i < ResourceTypes.Num(); i++) {
		if (ResourceTypes[i] == Resource) {
			return ResourceAmounts[i];
		}
	}

	return 0;
}