#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ConquestManager.generated.h"

USTRUCT(BlueprintType)
struct FFactionStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
		FString Owner;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
		UTexture2D* Texture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
		FLinearColor Colour;

	FFactionStruct()
	{
		Owner = "";
	}

	bool operator==(const FFactionStruct& other) const
	{
		return (other.Owner == Owner);
	}
};


USTRUCT(BlueprintType)
struct FWorldTileStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
		int32 X;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
		int32 Y;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
		bool bIsland;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
		bool bCapital;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
		FFactionStruct Occupier;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
		FString Name;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
		TSubclassOf<class AResource> Resource;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
		int32 Abundance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
		TArray<class ACitizen*> Citizens;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
		TMap<class ACitizen*, FString> Moving;

	FWorldTileStruct() {
		X = 0;
		Y = 0;
		bIsland = false;
		bCapital = false;
		Abundance = 1;
	}

	bool operator==(const FWorldTileStruct& other) const
	{
		return (other.X == X) && (other.Y == Y);
	}
};

USTRUCT(BlueprintType)
struct FIslandResourceStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TSubclassOf<class AResource> ResourceClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 Min;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 Max;

	FIslandResourceStruct()
	{
		ResourceClass = nullptr;
		Min = 0;
		Max = 0;
	}

	bool operator==(const FIslandResourceStruct& other) const
	{
		return (other.ResourceClass == ResourceClass);
	}
};

USTRUCT(BlueprintType)
struct FColonyEventStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colony")
		FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colony")
		float ResourceMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colony")
		float CitizenMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colony")
		float Citizens;

	FColonyEventStruct()
	{
		Name = "";
		ResourceMultiplier = 1.0f;
		CitizenMultiplier = 0.0f;
		Citizens = 0.0f;
	}

	bool operator==(const FColonyEventStruct& other) const
	{
		return (other.Name == Name);
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UConquestManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	UConquestManager();

protected:
	virtual void BeginPlay() override;

public:	
	void GenerateWorld();

	void StartConquest();

	void GiveResource();

	void SpawnCitizenAtColony(FWorldTileStruct* Tile);

	void MoveToColony(FWorldTileStruct* Tile, class ACitizen* Citizen);

	void StartTransmissionTimer(class ACitizen* Citizen);

	void AddCitizenToColony(FWorldTileStruct* OldTile, FWorldTileStruct* Tile, class ACitizen* Citizen);

	TArray<float> ProduceEvent();

	FWorldTileStruct* GetColonyContainingCitizen(class ACitizen* Citizen);

	void ModifyCitizensEvent(FWorldTileStruct* Tile, int32 Amount, bool bNegative);

	bool CanTravel(class ACitizen* Citizen);

	UFUNCTION(BlueprintCallable)
		FWorldTileStruct GetTileInformation(int32 Index);

	UFUNCTION(BlueprintCallable)
		TArray<FFactionStruct> GetFactions();

	UFUNCTION(BlueprintCallable)
		void SetFactionTexture(FFactionStruct Faction, UTexture2D* Texture, FLinearColor Colour);

	UFUNCTION(BlueprintCallable)
		void SetTerritoryName(FString OldEmpireName);

		void RemoveFromRecentlyMoved(class ACitizen* Citizen);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
		FString EmpireName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
		int32 WorldSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
		int32 PercentageIsland;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
		int32 EnemiesNum;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizens")
		TSubclassOf<class ACitizen> CitizenClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource List")
		TArray<FIslandResourceStruct> ResourceList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event List")
		TArray<FColonyEventStruct> EventList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occupier Texture List")
		TArray<FFactionStruct> DefaultOccupierTextureList;

	UPROPERTY()
		class ACamera* Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
		TArray<FWorldTileStruct> World;

	UPROPERTY()
		TArray<class ACitizen*> RecentlyMoved;

	UPROPERTY()
		int32 playerCapitalIndex;

	UPROPERTY()
		class APortal* Portal;
};
