#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Map/Grid.h"
#include "Universal/Resource.h"
#include "Player/Managers/CitizenManager.h"
#include "Buildings/Work/Work.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
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
struct FAtmosphereData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FCalendarStruct Calendar;

	UPROPERTY()
		FRotator WindRotation;

	UPROPERTY()
		bool bRedSun;

	FAtmosphereData()
	{
		WindRotation = FRotator::ZeroRotator;
		bRedSun = false;
	}
};

USTRUCT()
struct FWetnessData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FVector Location;

	UPROPERTY()
		float Value;

	UPROPERTY()
		float Increment;

	FWetnessData()
	{
		Location = FVector::Zero();
		Value = -1.0f;
		Increment = 0.0f;
	}
};

USTRUCT()
struct FCloudData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FTransform Transform;

	UPROPERTY()
		double Distance;

	UPROPERTY()
		bool bPrecipitation;

	UPROPERTY()
		bool bHide;

	UPROPERTY()
		float lightningTimer;

	UPROPERTY()
		float lightningFrequency;

	FCloudData()
	{
		Transform = FTransform();
		Distance = 0.0f;
		bPrecipitation = false;
		bHide = false;
		lightningTimer = 0.0f;
		lightningFrequency = 0.0f;
	}
};

USTRUCT()
struct FCloudsData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		TArray<FWetnessData> WetnessData;

	UPROPERTY()
		TArray<FCloudData> CloudData;

	UPROPERTY()
		bool bSnow;

	FCloudsData()
	{
		bSnow = false;
	}
};

USTRUCT()
struct FNaturalDisasterData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		float bDisasterChance;

	UPROPERTY()
		float Intensity;

	UPROPERTY()
		float Frequency;

	FNaturalDisasterData()
	{
		bDisasterChance = 0.0f;
		Intensity = 1.0f;
		Frequency = 1.0f;
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

	UPROPERTY()
		FAtmosphereData AtmosphereData;

	UPROPERTY()
		FCloudsData	CloudsData;

	UPROPERTY()
		FNaturalDisasterData NaturalDisasterData;

	FWorldSaveData()
	{
		Stream = FRandomStream();
		Size = 0;
		Chunks = 0;
	}
};

USTRUCT()
struct FOccupantData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FString OccupantName;

	UPROPERTY()
		TArray<FString> VisitorNames;

	FOccupantData()
	{
		OccupantName = "";
	}
};

USTRUCT()
struct FBuildingData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FString FactionName;

	UPROPERTY()
		int32 Capacity;

	UPROPERTY()
		int32 Seed;

	UPROPERTY()
		FLinearColor ChosenColour;

	UPROPERTY()
		int32 Tier;

	UPROPERTY()
		UStaticMesh* ActualMesh;

	UPROPERTY()
		TArray<FSocketStruct> SocketList;

	UPROPERTY()
		TArray<FItemStruct> Storage;

	UPROPERTY()
		TArray<FBasketStruct> Basket;

	UPROPERTY()
		double DeathTime;

	UPROPERTY()
		bool bOperate;

	UPROPERTY()
		TArray<FWorkHoursStruct> WorkHours;

	UPROPERTY()
		TArray<FOccupantData> OccupantsData;

	UPROPERTY()
		int32 BuildPercentage;

	FBuildingData()
	{
		FactionName = "";
		Capacity = 0;
		Seed = 0;
		ChosenColour = FLinearColor();
		Tier = 1;
		ActualMesh = nullptr;
		DeathTime = 0.0f;
		bOperate = true;
		BuildPercentage = 0;
	}
};

USTRUCT()
struct FAIData
{
	GENERATED_USTRUCT_BODY()

	ACitizen* Citizen;

	UPROPERTY()
		FString BuildingAtName;

	FAIData()
	{
		Citizen = nullptr;
		BuildingAtName = "";
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

	UPROPERTY()
		FBuildingData BuildingData;

	UPROPERTY()
		FAIData AIData;

	UPROPERTY()
		TArray<FTimerStruct> SavedTimers;

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
		TArray<FFactionStruct> Factions;

	UPROPERTY()
		TArray<FActorSaveData> SavedActors;

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
