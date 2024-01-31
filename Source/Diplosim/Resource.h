#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Resource.generated.h"

UCLASS()
class DIPLOSIM_API AResource : public AActor
{
	GENERATED_BODY()
	
public:	
	AResource();

public:	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UStaticMeshComponent* ResourceMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 MinYield;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 MaxYield;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 MaxWorkers;

	int32 WorkerCount;

	int32 Yield;

	int32 Quantity;

	int32 MaxQuantity;

	int32 GenerateYield();

	int32 GetYield();

	void SetQuantity(int32 Value);

	virtual void YieldStatus();
};
