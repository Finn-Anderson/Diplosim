#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Map/Grid.h"
#include "Universal/Resource.h"
#include "DiplosimSaveGame.generated.h"

USTRUCT()
struct FHISMData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY();
		FString Name;

	UPROPERTY();
		TArray<FTransform> Transforms;

	UPROPERTY();
		TArray<float> CustomDataValues;

	FHISMData()
	{
		Name = "";
	}

	bool operator==(const FHISMData& other) const
	{
		return (other.Name == Name);
	}
};

USTRUCT()
struct FTileData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		int32 Level;

	UPROPERTY()
		int32 Fertility;

	UPROPERTY()
		int32 X;

	UPROPERTY()
		int32 Y;

	UPROPERTY()
		FQuat Rotation;

	UPROPERTY()
		bool bRamp;

	UPROPERTY()
		bool bRiver;

	UPROPERTY()
		bool bEdge;

	UPROPERTY()
		bool bMineral;

	UPROPERTY()
		bool bUnique;

	FTileData() 
	{
		Level = -1;
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
};

USTRUCT()
struct FResourceData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FHISMData HISMData;

	UPROPERTY()
		TArray<FWorkerStruct> Workers;

	FResourceData()
	{

	}
};

USTRUCT()
struct FWorldSaveData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		TArray<FTileData> Tiles;

	UPROPERTY()
		TArray<FHISMData> HISMData;

	UPROPERTY()
		int32 Size;

	UPROPERTY()
		int32 Chunks;

	UPROPERTY()
		FRandomStream Stream;
		
	UPROPERTY()
		TArray<FVector> LavaSpawnLocations;

	FWorldSaveData()
	{
		Stream = FRandomStream();
		Size = 0;
		Chunks = 0;
	}
};

USTRUCT()
struct FActorSaveData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FString Name;

	UPROPERTY()
		UClass* Class;

	UPROPERTY()
		FTransform Transform;

	UPROPERTY()
		FWorldSaveData WorldSaveData;

	UPROPERTY()
		FResourceData ResourceData;

	FActorSaveData()
	{
		Class = nullptr;
		Transform = FTransform(FQuat::Identity, FVector(0.0f));
	}
};

USTRUCT(BlueprintType)
struct FSave
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
		FString SaveName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
		FString Period;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
		int32 Day;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
		int32 Hour;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
		int32 CitizenNum;

	UPROPERTY()
		bool bAutosave;

	UPROPERTY()
		TArray<FActorSaveData> SavedActors;

	UPROPERTY()
		TArray<FTimerStruct> SavedTimers;

	FSave()
	{
		SaveName = "";
		Period = "";
		Day = 0;
		Hour = 0;
		CitizenNum = 0;
		bAutosave = false;
	}
	
	bool operator==(const FSave& other) const
	{
		return (other.SaveName == SaveName);
	}
};

UCLASS()
class DIPLOSIM_API UDiplosimSaveGame : public USaveGame
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
		TArray<FSave> Saves;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
		FString ColonyName;

	UPROPERTY()
		FDateTime LastTimeUpdated;
};
