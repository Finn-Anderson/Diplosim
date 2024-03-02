#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Grid.generated.h"

USTRUCT()
struct FTileStruct
{
	GENERATED_USTRUCT_BODY()

	int32 Level;

	int32 Instance;

	int32 Fertility;

	int32 X;

	int32 Y;

	FQuat Rotation;

	int32 ResourceNum;

	TMap<FString, FTileStruct*> AdjacentTiles;

	FTileStruct() {
		Level = -100;

		Instance = 0;

		Fertility = 0;

		X = 0;

		Y = 0;

		ResourceNum = 0;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
		int32 PercentageGround;

	//UI
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> LoadUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* LoadUIInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> MapUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* MapUIInstance;

	// Resource Classes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
		TArray<TSubclassOf<class AResource>> ResourceClasses;

	// Instance Meshes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMLava;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMWater;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMGround;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMFlatGround;

	TMap<FString, TArray<class AResource*>> Resources;

	// Dimensions
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dimensions")
		int32 Size;

	UPROPERTY()
		class ACamera* Camera;

	// Lava Particle System
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particles")
		class UNiagaraSystem* LavaSystem;

	TArray<class UNiagaraComponent*> LavaComponents;

	// Map
	void Load();

	void Render();

	void FillHoles(FTileStruct* Tile);

	void SetTileDetails(FTileStruct* Tile);

	void GenerateTile(FTileStruct* Tile);

	void Clear();

	TArray<TArray<FTileStruct>> Storage;

	AClouds* Clouds;

	// Resources
	void GenerateResource(FTileStruct* Tile);

	bool Forest;
};
