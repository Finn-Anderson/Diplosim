#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Components/ActorComponent.h"
#include "AIBuildComponent.generated.h"

USTRUCT(BlueprintType)
struct FAIBuildStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		TSubclassOf<class ABuilding> Building;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		int32 NumCitizens;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		int32 NumCitizensIncrement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Build")
		int32 CurrentAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		int32 Limit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		TSubclassOf<class AResource> Resource;

	FAIBuildStruct()
	{
		Building = nullptr;
		NumCitizens = 0;
		NumCitizensIncrement = 50;
		CurrentAmount = 0;
		Limit = 0;
		Resource = nullptr;
	}

	bool operator==(const FAIBuildStruct& other) const
	{
		return (other.Building == Building);
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UAIBuildComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAIBuildComponent();

	// Tile Distance Caluclation
	void InitialiseTileLocationDistances(FFactionStruct* Faction);

	double CalculateTileDistance(FVector EggTimerLocation, FVector TileLocation);

	void SortTileDistances(TMap<FVector, double>& Locations);

	// AI Building
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		TArray<FAIBuildStruct> AIBuilds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		TArray<TSubclassOf<class ABuilding>> Houses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		TArray<TSubclassOf<class ABuilding>> MiscBuilds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		TSubclassOf<class ABuilding> RoadClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		TSubclassOf<class ABuilding> RampClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		TSubclassOf<class ABuilding> NDProtectorClass;

	UPROPERTY()
		class ACamera* Camera;

	void EvaluateAIBuild(FFactionStruct* Faction);

private:
	void RecalculateTileLocationDistances(FFactionStruct* Faction);

	void RemoveTileLocations(FFactionStruct* Faction, class ABuilding* Building);

	TArray<FVector> ConnectRoadTiles(FFactionStruct* Faction, struct FTileStruct* Tile, FVector Location);

	void AITrade(FFactionStruct* Faction);

	void BuildFirstBuilder(FFactionStruct* Faction);

	void BuildAIBuild(FFactionStruct* Faction);
	void BuildAIFarms(FFactionStruct* Faction, TArray<TSubclassOf<ABuilding>>& BuildingsClassList);
	void BuildAIBuildings(FFactionStruct* Faction, TArray<TSubclassOf<ABuilding>>& BuildingsClassList);
	void BuildAINDProtectors(FFactionStruct* Faction, TArray<TSubclassOf<ABuilding>>& BuildingsClassList);

	void BuildAIHouse(FFactionStruct* Faction);

	void BuildAIRoads(FFactionStruct* Faction);

	void BuildMiscBuild(FFactionStruct* Faction);

	void BuildAIAccessibility(FFactionStruct* Faction);

	void ChooseBuilding(FFactionStruct* Faction, TArray<TSubclassOf<class ABuilding>> BuildingsClasses);

	bool AIValidBuildingLocation(FFactionStruct* Faction, class ABuilding* Building, float Extent, FVector Location);

	bool AICanAfford(FFactionStruct* Faction, TSubclassOf<class ABuilding> BuildingClass, int32 Amount = 1);

	void AIBuild(FFactionStruct* Faction, TSubclassOf<class ABuilding> BuildingClass, TSubclassOf<class AResource> Resource, bool bAccessibility = false, FTileStruct* Tile = nullptr);
};