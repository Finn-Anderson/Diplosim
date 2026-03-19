#pragma once

#include "CoreMinimal.h"
#include "Buildings/Building.h"
#include "Road.generated.h"

UCLASS()
class DIPLOSIM_API ARoad : public ABuilding
{
	GENERATED_BODY()
	
public:
	ARoad();

	virtual void Build(bool bRebuild = false, bool bUpgrade = false, int32 Grade = 0) override;

	virtual void SetBuildingColour(float R, float G, float B) override;

	virtual void DestroyBuilding(bool bCheckAbove = true, bool bMove = true) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
		class UBoxComponent* BoxAreaAffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roads")
		class UInstancedStaticMeshComponent* HISMRoad;

	void RegenerateMesh(bool bRegenerateHits);

	void SetTier(int32 Value) override;

protected:
	virtual void BeginPlay() override;
};
