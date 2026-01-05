#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Grid.generated.h"

struct FTileStruct
{
	int32 Level;

	int32 Instance;

	int32 Fertility;

	int32 X;

	int32 Y;
		
	FQuat Rotation;

	TMap<FString, FTileStruct*> AdjacentTiles;

	bool bRamp;

	bool bRiver;

	bool bEdge;

	bool bMineral;

	bool bUnique;

	FTileStruct() {
		Level = -1;

		Instance = 0;

		Fertility = -1;

		X = 0;

		Y = 0;

		Rotation = FRotator(0.0f, 0.0f, 0.0f).Quaternion();

		bRamp = false;

		bRiver = false;

		bEdge = false;

		bMineral = false;

		bUnique = false;
	}

	bool operator==(const FTileStruct& other) const
	{
		return (other.X == X) && (other.Y == Y);
	}
};

USTRUCT(BlueprintType)
struct FResourceHISMStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TSubclassOf<class AResource> ResourceClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 Multiplier;

	UPROPERTY()
		class AResource* Resource;

	FResourceHISMStruct()
	{
		ResourceClass = nullptr;
		Multiplier = 1;
		Resource = nullptr;
	}
};

UENUM()
enum class EType : uint8
{
	Round,
	Mountain
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class USceneComponent* WorldContainer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
		class UAtmosphereComponent* AtmosphereComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
		class UAIVisualiser* AIVisualiser;

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

	// Instance Meshes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMLava;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMGround;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMFlatGround;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMRampGround;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMRiver;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMSea;

	// Dimensions
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dimensions")
		int32 Size;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dimensions")
		int32 Chunks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dimensions")
		int32 Peaks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dimensions")
		int32 Rivers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dimensions")
		EType Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dimensions")
		int32 MaxLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dimensions")
		bool bLava;

	UPROPERTY()
		class ACamera* Camera;

	// Lava
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lava")
		class UNiagaraComponent* LavaComponent;

	UPROPERTY()
		TArray<FVector> LavaSpawnLocations;

	// Map
	int32 GetMapBounds();

	void Load();

	void InitialiseStorage();

	void SetupMap();

	void CleanupHoles();

	void RemovePocketSeas();

	void SetupTileInformation();

	void PaveRivers();

	void SpawnTiles();

	void SpawnMinerals();

	void SpawnVegetation();

	void SetupEnvironment(bool bLoad = false);

	UFUNCTION()
		void OnNavMeshGenerated();

	TArray<FTileStruct*> CalculatePath(FTileStruct* Tile, FTileStruct* Target);

	void FillHoles(FTileStruct* Tile);

	void GetSeaTiles(FTileStruct* Tile);

	void SetTileDetails(FTileStruct* Tile);

	TArray<FTileStruct*> GenerateRiver(FTileStruct* Tile, FTileStruct* Peak);

	void CalculateTile(FTileStruct* Tile);

	void AddCalculatedTile(UHierarchicalInstancedStaticMeshComponent* HISM, FTransform Transform);

	void GenerateTiles();

	void CreateWaterfall(FVector Location, int32 Num, int32 Sign, bool bOnYAxis);

	void Clear();

	FTileStruct* GetTileFromLocation(FVector WorldLocation);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crystal")
		class UStaticMeshComponent* CrystalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Seed")
		FString Seed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colours")
		TArray<FLinearColor> TreeColours;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colours")
		TArray<FLinearColor> GroundColours;

	TMap<UHierarchicalInstancedStaticMeshComponent*, TArray<FTransform>> CalculatedTiles;

	TArray<FTileStruct*> PeaksList;

	TArray<TArray<FTileStruct*>> ValidMineralTiles;

	TArray<FTileStruct*> SeaTiles;

	TArray<TArray<FTileStruct>> Storage;

	// Resources
	UFUNCTION(BlueprintCallable)
		void SetMineralMultiplier(TSubclassOf<class AMineral> MineralClass, int32 Multiplier);

	void GetValidSpawnLocations(FTileStruct* Tile, FTileStruct* CheckTile, int32 Range, bool& Valid, TArray<FTileStruct*>& Tiles);

	void GenerateMinerals(FTileStruct* Tile, class AResource* Resource);

	void GenerateVegetation(TArray<FResourceHISMStruct> Vegetation, FTileStruct* StartingTile, FTileStruct* Tile, int32 Amount, float Scale, bool bTree);

	void RemoveTree(class AResource* Resource, int32 Instance);

	FTransform GetTransform(FTileStruct* Tile);

	TArray<FTileStruct*> ResourceTiles;

	TArray<FTileStruct*> VegetationTiles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TArray<FResourceHISMStruct> TreeStruct;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TArray<FResourceHISMStruct> FlowerStruct;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TArray<FResourceHISMStruct> MineralStruct;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 VegetationMinDensity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 VegetationMaxDensity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 VegetationMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 VegetationSizeMultiplier;

	// Egg Basket
	UFUNCTION()
		void SpawnEggBasket();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Egg Basket")
		TSubclassOf<class AEggBasket> EggBasketClass;

	// Unique Buildings
	void SetSpecialBuildings(TArray<TArray<FTileStruct*>> ValidTiles);

	UFUNCTION(BlueprintCallable)
		void SetSpecialBuildingStatus(class ASpecial* Building, bool bShow);

	void BuildSpecialBuildings();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unique")
		TArray<class ASpecial*> SpecialBuildings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unique")
		bool bRandSpecialBuildings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unique")
		TArray<TSubclassOf<class ASpecial>> SpecialBuildingClasses;

	// AISpawners
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Spawner")
		TSubclassOf<class AAISpawner> AISpawnerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Spawner")
		int32 NumOfNests;

	void SpawnAISpawners();
};
