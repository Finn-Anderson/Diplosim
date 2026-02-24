#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Resource.generated.h"

USTRUCT(BlueprintType)
struct FWorkerStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workers")
		TArray<class ACitizen*> Citizens;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workers")
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

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UHierarchicalInstancedStaticMeshComponent* ResourceHISM;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		 TSubclassOf<class AResource> DroppedResource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		 TArray<TSubclassOf<class AResource>> ParentResource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		 TSubclassOf<class AResource> SpecialResource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 MinYield;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 MaxYield;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 MaxWorkers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workers")
		TArray<FWorkerStruct> WorkerStruct;

	UPROPERTY()
		class ACamera* Camera;

	int32 GenerateYield();

	int32 GetYield(class ACitizen* Citizen, int32 Instance);

	void AddWorker(class ACitizen* citizen, int32 Instance);

	TArray<ACitizen*> RemoveWorker(class ACitizen* citizen, int32 Instance);

	class AResource* GetHarvestedResource();

	TArray<TSubclassOf<class AResource>> GetParentResources();

	UFUNCTION()
		virtual void YieldStatus(int32 Instance, int32 Yield);
};
