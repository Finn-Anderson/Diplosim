#pragma once

#include "CoreMinimal.h"
#include "Map/Grid.h"
#include "Components/ActorComponent.h"
#include "NaturalDisasterComponent.generated.h"

USTRUCT()
struct FEarthquakeStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FVector Point;

	UPROPERTY()
		TMap<class ABuilding*, float> BuildingsInRange;

	FEarthquakeStruct()
	{
		Point = FVector::Zero();
	}

	bool operator==(const FEarthquakeStruct& other) const
	{
		return (other.Point == Point);
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UNaturalDisasterComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UNaturalDisasterComponent();

public:	
	bool ShouldCreateDisaster();

	void IncrementDisasterChance();

	void ResetDisasterChance();

	void GenerateEarthquake(float Magnitude);

	TArray<FTileStruct> GetEarthquakePoints(float Magnitude);

	UFUNCTION()
		void CalculateEarthquakeDamage(TArray<FEarthquakeStruct> EarthquakeStruct, float Range, float Magnitude);

	void CancelEarthquake();

	void GeneratePurifier(float Magnitude);

	void GenerateRedSun(float Magnitude);

	void AlterSunGradually(float Target, float Increment);

	UFUNCTION()
		void CancelRedSun();

	bool IsProtected(FVector Location);

	UPROPERTY()
		class AGrid* Grid;
		
	UPROPERTY()
		float bDisasterChance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Natural Disaster")
		float Intensity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Natural Disaster")
		float Frequency;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Earthquake Shake")
		TSubclassOf<class UCameraShakeBase> Shake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Purifier")
		TSubclassOf<class AProjectile> PurifierClass;
};
