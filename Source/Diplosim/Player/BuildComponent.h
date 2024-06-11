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

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	bool bCanRotate;

public:
	// Building
	UPROPERTY(EditAnywhere)
		class UMaterial* BlueprintMaterial;

	UPROPERTY(EditAnywhere)
		class UMaterial* BlockedMaterial;

	FRotator Rotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
		class ABuilding* Building;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
		class ABuilding* BuildingToMove;

	bool IsValidLocation();

	void RotateBuilding(bool Rotate);

	void Place();

	void QuickPlace();

	UFUNCTION(BlueprintCallable)
		void SpawnBuilding(TSubclassOf<class ABuilding> BuildingClass);

	UFUNCTION(BlueprintCallable)
		void DetachBuilding();

	UFUNCTION(BlueprintCallable)
		void RemoveBuilding();
};
