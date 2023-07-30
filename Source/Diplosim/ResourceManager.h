#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ResourceManager.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UResourceManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	UResourceManager();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TArray<TSubclassOf<class AResource>> ResourceTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TArray<int32> ResourceAmounts;

	void ChangeResource(TSubclassOf<AResource> Resource, int32 Change);

	int32 GetResource(TSubclassOf<AResource> Resource);
};
