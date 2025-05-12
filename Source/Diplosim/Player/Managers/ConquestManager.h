#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ConquestManager.generated.h"

struct FWorldTileStruct
{
	int32 X;

	int32 Y;
	
	bool bIsland;

	bool bCapital;

	FString Owner;

	FString Name;

	TSubclassOf<class AResource> Resource;

	int32 Abundance;

	int32 Males;

	int32 Females;

	FWorldTileStruct() {
		X = 0;
		Y = 0;
		bIsland = false;
		bCapital = false;
		Owner = "";
		Abundance = 1;
		Males = 0;
		Females = 0;
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
		int32 ResourceMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colony")
		int32 CitizenMultiplier;

	FColonyEventStruct()
	{
		Name = "";
		ResourceMultiplier = 1;
		CitizenMultiplier = 1;
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

	void GiveResource();

	TArray<int32> ProduceEvent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource List")
		TArray<FIslandResourceStruct> ResourceList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event List")
		TArray<FColonyEventStruct> EventList;

	UPROPERTY()
		class ACamera* Camera;

	TArray<TArray<FWorldTileStruct>> World;
};
