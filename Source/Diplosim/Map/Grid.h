#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Grid.generated.h"

UCLASS()
class DIPLOSIM_API AGrid : public AActor
{
	GENERATED_BODY()
	
public:	
	AGrid();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UStaticMeshComponent* WaterMesh;

public:
	// Tile Classes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type")
		TSubclassOf<class ATile> Water;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type")
		TSubclassOf<class ATile> Ground;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type")
		TSubclassOf<class ATile> Hill;

public:
	// Resource Classes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
		TSubclassOf<class AMineral> Rock;

public:
	// Dimensions
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dimensions")
		int32 Size;

	UPROPERTY()
		class ACamera* Camera;

	AActor* Storage[200][200];

protected:
	virtual void BeginPlay() override;

public:
	void Render();

	void GenerateTile(TSubclassOf<class ATile> Choice, int32 Mean, int32 x, int32 y);

	void GenerateResource(class ATile* tile, int32 x, int32 y);

	void Clear();
};