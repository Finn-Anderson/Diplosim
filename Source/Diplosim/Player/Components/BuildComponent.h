#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuildComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UBuildComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UBuildComponent();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY()
		class ACamera* Camera;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY()
		double lastUpdatedTime;

	UPROPERTY()
		bool bCanRotate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
		TSubclassOf<ABuilding> FoundationClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
		TSubclassOf<ABuilding> RampClass;

	FCriticalSection BuildLock;

	// Building
	UPROPERTY(EditAnywhere)
		class UMaterial* BlueprintMaterial;

	UPROPERTY(EditAnywhere)
		class UMaterial* BlockedMaterial;

	UPROPERTY(EditAnywhere)
		class UMaterial* InfluencedMaterial;

	UPROPERTY()
		FRotator Rotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
		TArray<class ABuilding*> Buildings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
		class ABuilding* BuildingToMove;

	UPROPERTY()
		FVector StartLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
		class USoundBase* PlaceSound;

	TArray<FHitResult> GetBuildingOverlaps(class ABuilding* Building, float Extent = 1.0f, FVector Location = FVector::Zero());

	void SetTreeStatus(ABuilding* Building, bool bDestroy, bool bRemoveBuilding = false, FVector PrevLocation = FVector::Zero());

	void DisplayInfluencedBuildings(class ABuilding* Building, bool bShow);

	void SetBuildingsOnPath();

	TArray<FVector> CalculatePath(struct FTileStruct* StartTile, struct FTileStruct* EndTile);

	bool IsValidLocation(ABuilding* building);

	UFUNCTION(BlueprintCallable)
		TArray<struct FItemStruct> GetBuildCosts();

	bool CheckBuildCosts();

	void RotateBuilding(bool Rotate);

	void StartPathPlace();

	void EndPathPlace();

	void Place(bool bQuick = false);

	void QuickPlace();

	UFUNCTION(BlueprintCallable)
		void SpawnBuilding(TSubclassOf<class ABuilding> BuildingClass, FString FactionName, FVector location = FVector(0.0f, 0.0f, -1000.0f));

	void ResetBuilding(ABuilding* Building);

	UFUNCTION(BlueprintCallable)
		void DetachBuilding();

	UFUNCTION(BlueprintCallable)
		void RemoveBuilding();

	void RemoveWalls(ABuilding* Building);
};
