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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 Committed;

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

	void AddCommittedResource(TSubclassOf<class AResource> Resource, int32 Amount);

	void TakeCommittedResource(TSubclassOf<class AResource> Resource, int32 Amount);

	bool AddLocalResource(class ABuilding* Building, int32 Amount);

	bool AddUniversalResource(TSubclassOf<class AResource> Resource, int32 Amount);

	bool TakeLocalResource(class ABuilding* Building, int32 Amount);

	bool TakeUniversalResource(TSubclassOf<class AResource> Resource, int32 Amount, int32 Min);

	void SetResourceStruct(TSubclassOf<AResource> Resource);

	UFUNCTION(BlueprintCallable, Category = "Resource")
		int32 GetResourceAmount(TSubclassOf<class AResource> Resource);

	TArray<TSubclassOf<AResource>> GetResource(class ABuilding* Building);

	TArray<TSubclassOf<class ABuilding>> GetBuildings(TSubclassOf<class AResource> Resource);

	void DisplayStorage(ABuilding* Building);

public:
	// UI
	UFUNCTION(BlueprintCallable)
		FResourceStruct GetUpdatedResource();

	FResourceStruct ResourceStruct;
};
