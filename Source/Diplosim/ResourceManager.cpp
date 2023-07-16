#include "ResourceManager.h"

UResourceManager::UResourceManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	FString Arr[] = {TEXT("Wood"), TEXT("Stone"), TEXT("Money")};
	ResourceNames.Append(Arr, sizeof(Arr) / sizeof(int));

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