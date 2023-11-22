#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Grid.generated.h"

USTRUCT()
struct FTileStruct
{
	GENERATED_USTRUCT_BODY()

	class UHierarchicalInstancedStaticMeshComponent* Choice;

	TArray<class UHierarchicalInstancedStaticMeshComponent*> ChoiceList;

	int32 Instance;

	int32 Fertility;

	int32 X;

	int32 Y;

	TArray<class AActor*> Resource;

	FTileStruct() {
		Choice = nullptr;

		Fertility = 0;
	}
};

UCLASS()
class DIPLOSIM_API AGrid : public AActor
{
	GENERATED_BODY()
	
public:	
	AGrid();

protected:
	virtual void BeginPlay() override;

public:
	// Resource Classes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
		TSubclassOf<class AMineral> Rock;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
		TSubclassOf<class AVegetation> Tree;

	// Instance Meshes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMWater;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMGround;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMMound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMHill;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMMountain;

	// Dimensions
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dimensions")
		int32 Size;

	UPROPERTY()
		class ACamera* Camera;

	// Map
	TArray<FTileStruct> Storage;
	
	void Render();

	int32 GetFertility(FTileStruct Tile);

	void SetTileChoices(TArray<FTileStruct> Tiles);

	UHierarchicalInstancedStaticMeshComponent* GetTargetChoice(TArray<int32> Pos, TArray<UHierarchicalInstancedStaticMeshComponent*> TargetBanList);

	TArray<FTileStruct> GetChosenList(TArray<int32> Pos, class UHierarchicalInstancedStaticMeshComponent* TargetChoice);

	void GenerateTile(UHierarchicalInstancedStaticMeshComponent* Choice, int32 Fertility, int32 x, int32 y);

	void Clear();

	// Resources
	bool Forest;

	void GenerateResource(int32 Pos);

	void SpawnResource(int32 Pos, FVector Location, TSubclassOf<class AResource> ResourceClass);
};
