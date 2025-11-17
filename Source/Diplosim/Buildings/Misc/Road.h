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

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION()
		void OnCitizenOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void OnCitizenOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	virtual void Build(bool bRebuild = false, bool bUpgrade = false, int32 Grade = 0) override;

	virtual void SetBuildingColour(float R, float G, float B) override;

	virtual void DestroyBuilding(bool bCheckAbove = true, bool bMove = true) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
		class UBoxComponent* BoxAreaAffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roads")
		class UHierarchicalInstancedStaticMeshComponent* HISMRoad;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roads")
		TArray<UStaticMesh*> RoadMeshes;

	void RegenerateMesh();

	void SetTier(int32 Value) override;
};
