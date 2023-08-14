#pragma once

#include "CoreMinimal.h"
#include "Resource.h"
#include "Vegetation.generated.h"

UCLASS()
class DIPLOSIM_API AVegetation : public AResource
{
	GENERATED_BODY()

public:
	AVegetation();

protected:
	virtual void BeginPlay();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree Meshes")
		TArray<class UStaticMesh*> MeshList;

	virtual void YieldStatus();

	void Grow();

	bool bIsGettingChopped;
};
