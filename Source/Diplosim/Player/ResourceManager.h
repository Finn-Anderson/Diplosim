#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ResourceManager.generated.h"

USTRUCT(BlueprintType)
struct FResourceStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TSubclassOf<class AResource> Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TArray<TSubclassOf<class ABuilding>> Buildings;

	FResourceStruct()
	{
		Type = nullptr;
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UResourceManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	UResourceManager();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TArray<FResourceStruct> ResourceList;

	bool AddLocalResource(class ABuilding* Building, int32 Amount);

	bool AddUniversalResource(TSubclassOf<class AResource> Resource, int32 Amount);

	bool TakeResource(TSubclassOf<class AResource> Resource, int32 Amount);

	void SetResourceStruct(TSubclassOf<AResource> Resource);

	int32 GetResourceAmount(TSubclassOf<class AResource> Resource);

	TSubclassOf<class AResource> GetResource(class ABuilding* Building);

public:
	// UI
	UFUNCTION(BlueprintCallable)
		FResourceStruct GetUpdatedResource();

	FResourceStruct ResourceStruct;
};
