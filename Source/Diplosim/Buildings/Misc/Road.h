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

	UFUNCTION()
		void OnRoadOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void OnRoadOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
		class UBoxComponent* BoxCollision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
		class UBoxComponent* BoxAreaAffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roads")
		class UHierarchicalInstancedStaticMeshComponent* HISMRoad;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roads")
		TArray<UStaticMesh*> RoadMeshes;

	void RegenerateMesh();

	void SetTier(int32 Value) override;
};
