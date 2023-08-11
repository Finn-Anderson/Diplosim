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

	UPROPERTY()
		class AGround* PrevTile;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetGridStatus();

	bool GridStatus;

public:
	// Building
	UPROPERTY(EditAnywhere)
		class UMaterial* BlueprintMaterial;

	UPROPERTY(EditAnywhere)
		class UMaterial* BlockedMaterial;

	FRotator Rotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
		class ABuilding* Building;

	UPROPERTY()
		TArray<class AVegetation*> Vegetation;

	void HideTree(class AVegetation* vegetation, bool Hide);

	UFUNCTION(BlueprintCallable)
		void Build();

	void RotateBuilding();

	void Place();

	UFUNCTION(BlueprintCallable)
		void SetBuildingClass(TSubclassOf<class ABuilding> BuildingClass);
};
