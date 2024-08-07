#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Resource.generated.h"

USTRUCT()
struct FWorkerStruct
{
	GENERATED_USTRUCT_BODY()

	TArray<class ACitizen*> Citizens;

	int32 Instance;

	FWorkerStruct() {
		Citizens = {};
		Instance = -1;
	}

	bool operator==(const FWorkerStruct& other) const
	{
		return (other.Instance == Instance);
	}
};

UCLASS()
class DIPLOSIM_API AResource : public AActor
{
	GENERATED_BODY()
	
public:	
	AResource();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UHierarchicalInstancedStaticMeshComponent* ResourceHISM;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		 TSubclassOf<class AResource> DroppedResource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		 TArray<TSubclassOf<class AResource>> ParentResource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 MinYield;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 MaxYield;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 MaxWorkers;

	TArray<FWorkerStruct> WorkerStruct;

	int32 GenerateYield();

	int32 GetYield(class ACitizen* Citizen, int32 Instance);

	void SetQuantity(int32 Instance, int32 Value);

	void AddWorker(class ACitizen* citizen, int32 Instance);

	void RemoveWorker(class ACitizen* citizen, int32 Instance);

	class AResource* GetHarvestedResource();

	TArray<TSubclassOf<class AResource>> GetParentResources();

	virtual void YieldStatus(int32 Instance, int32 Yield);
};
