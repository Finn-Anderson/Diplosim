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
	class ACamera* Camera;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	bool bCanRotate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
		TSubclassOf<ABuilding> FoundationClass;

	// Building
	UPROPERTY(EditAnywhere)
		class UMaterial* BlueprintMaterial;

	UPROPERTY(EditAnywhere)
		class UMaterial* BlockedMaterial;

	FRotator Rotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
		TArray<class ABuilding*> Buildings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
		class ABuilding* BuildingToMove;

	FVector StartLocation;

	FVector EndLocation;

	void SetBuildingsOnPath();

	TArray<FVector> CalculatePath(struct FTileStruct Tile);

	bool IsValidLocation(ABuilding* building);

	UFUNCTION(BlueprintCallable)
		struct FQueueStruct GetBuildCosts();

	bool CheckBuildCosts();

	void RotateBuilding(bool Rotate);

	void Place();

	void QuickPlace();

	UFUNCTION(BlueprintCallable)
		void SpawnBuilding(TSubclassOf<class ABuilding> BuildingClass, FVector location = FVector(0.0f, 0.0f, 50.0f));

	UFUNCTION(BlueprintCallable)
		void DetachBuilding();

	UFUNCTION(BlueprintCallable)
		void RemoveBuilding();
};