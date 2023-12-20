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

	FQuat Rotation;

	TArray<class AActor*> Resource;

	bool bIsSea;

	FTileStruct() {
		Choice = nullptr;

		Instance = 0;

		Fertility = 0;

		X = 0;

		Y = 0;

		bIsSea = false;

		Rotation = FRotator(0.0f, 0.0f, 0.0f).Quaternion();
	}

	bool operator==(const FTileStruct& other) const
	{
		return (other.X == X) && (other.Y == Y);
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds")
	TSubclassOf<class AClouds> CloudsClass;

	// Resource Classes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
		TSubclassOf<class AMineral> Rock;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
		TSubclassOf<class AVegetation> Tree;

	// Instance Meshes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMWater;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMBeach;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMGround;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMMound;

	// Dimensions
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dimensions")
		int32 Size;

	UPROPERTY()
		class ACamera* Camera;

	// Map
	TArray<FTileStruct> Storage;
	
	void Render();

	TArray<FTileStruct> SetTileChoices(TArray<FTileStruct> Tiles);

	UHierarchicalInstancedStaticMeshComponent* GetTargetChoice(TArray<FTileStruct> Tiles, TArray<UHierarchicalInstancedStaticMeshComponent*> TargetBanList);

	TArray<FTileStruct> GetChosenList(TArray<FTileStruct> Tiles, class UHierarchicalInstancedStaticMeshComponent* TargetChoice, int32 MaxCount);

	void SetBeaches();

	void GenerateTile(UHierarchicalInstancedStaticMeshComponent* Choice, int32 Fertility, int32 x, int32 y, FQuat Rotation);

	void Clear();

	// Resources
	bool Forest;

	void GenerateResource(int32 Pos);

	void SpawnResource(int32 Pos, FVector Location, TSubclassOf<class AResource> ResourceClass);
};
