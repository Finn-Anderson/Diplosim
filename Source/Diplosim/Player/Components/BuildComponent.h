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
		bool bCanRotate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
		TSubclassOf<ABuilding> FoundationClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
		TSubclassOf<ABuilding> RampClass;

	// Building
	UPROPERTY(EditAnywhere)
		class UMaterial* BlueprintMaterial;

	UPROPERTY(EditAnywhere)
		class UMaterial* BlockedMaterial;

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

	void SetTileOpacity(ABuilding* Building, float Opacity);

	void SetBuildingsOnPath();

	TArray<FVector> CalculatePath(struct FTileStruct StartTile, struct FTileStruct EndTile);

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
		void SpawnBuilding(TSubclassOf<class ABuilding> BuildingClass, FVector location = FVector(0.0f, 0.0f, -1000.0f));

	void ResetBuilding(ABuilding* Building);

	UFUNCTION(BlueprintCallable)
		void DetachBuilding();

	UFUNCTION(BlueprintCallable)
		void RemoveBuilding();

	void RemoveWalls(ABuilding* Building);
};
