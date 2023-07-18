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

	class UMaterialInstanceDynamic* OGMaterial;

	FRotator Rotation;

	UPROPERTY()
		class ABuilding* Building;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
		TSubclassOf<class ABuilding> BuildingClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
		TSubclassOf<class AResource> TreeClass;

	UPROPERTY()
		TArray<class AResource*> Trees;

	bool IsBlocked;

	void HideTree(class AResource* tile);

	void Build();

	void RotateBuilding();

	void Place();
};
