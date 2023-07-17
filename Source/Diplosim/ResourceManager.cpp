#include "ResourceManager.h"

UResourceManager::UResourceManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	ResourceNames.Add(TEXT("Wood"));
	ResourceNames.Add(TEXT("Stone"));
	ResourceNames.Add(TEXT("Money"));

	ResourceAmounts.Init(0, ResourceNames.Num());
}

void UResourceManager::ChangeResource(FString Name, int32 Change)
{
	for (int i = 0; i < ResourceNames.Num(); i++) {
		if (ResourceNames[i] == Name) {
			ResourceAmounts[i] += Change;

			return;
		}
	}
}

int32 UResourceManager::GetResource(FString Name)
{
	for (int i = 0; i < ResourceNames.Num(); i++) {
		if (ResourceNames[i] == Name) {
			return ResourceAmounts[i];
		}
	}

	return 0;
}