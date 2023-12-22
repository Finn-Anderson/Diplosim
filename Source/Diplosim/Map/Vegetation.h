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
	virtual void YieldStatus();

	void Grow();

	bool IsChoppable();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meshes")
		TArray<class UStaticMesh*> MeshList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
		int32 TimeLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale")
		bool bVaryScale;

	FVector IntialScale;

	FVector MaxScale;
};
