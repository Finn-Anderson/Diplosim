#pragma once

#include "CoreMinimal.h"
#include "Map/Grid.h"
#include "Components/ActorComponent.h"
#include "NaturalDisasterComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UNaturalDisasterComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UNaturalDisasterComponent();

	bool ShouldCreateDisaster();

	void IncrementDisasterChance();

	void ResetDisasterChance();

	void GenerateNDShakesAndSounds(FString Type, TArray<FVector> Points, float Range);

	void SetNDSounds(float Dilation);

	UFUNCTION()
		void ClearNDShakesAndSounds(FString TypeAndTally);

	UFUNCTION()
		void AlterSunGradually(float Target, float Increment);

	UFUNCTION()
		void CancelRedSun();

	void GenerateVolcano(float Magnitude);

	UFUNCTION()
		void GenerateVolcanicRock(float Magnitude, FString TypeAndTally);

	bool IsProtected(FVector Location);

	UPROPERTY()
		class AGrid* Grid;
		
	UPROPERTY()
		float DisasterChance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Natural Disaster")
		float Intensity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Natural Disaster")
		float Frequency;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Earthquake")
		TSubclassOf<class UCameraShakeBase> EarthquakeShake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Earthquake")
		TSubclassOf<class UCameraShakeBase> VolcanoShake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Earthquake")
		class USoundBase* Sound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Purifier")
		TSubclassOf<class AProjectile> PurifierClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Volcano")
		TSubclassOf<class AProjectile> VolcanicRockClass;

	UPROPERTY()
		bool bRedSun;

	UPROPERTY()
		float VolcanoMagnitude;

	UPROPERTY()
		int32 DisasterTally;

private:
	void GenerateEarthquake(float Magnitude);

	TArray<FVector> GetEarthquakePoints(float Magnitude);

	void GeneratePurifier(float Magnitude);

	void GenerateRedSun(float Magnitude);

	float VolcanoTimer;
};
