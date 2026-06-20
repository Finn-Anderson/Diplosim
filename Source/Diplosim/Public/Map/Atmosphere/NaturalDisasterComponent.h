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

	void SetEarthquakeSounds(float Dilation);

	UFUNCTION()
		void ClearEarthquakeShakesAndSounds();

	void AlterSunGradually(float Target, float Increment);

	UFUNCTION()
		void CancelRedSun();

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
		TSubclassOf<class UCameraShakeBase> Shake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Earthquake")
		class USoundBase* Sound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Purifier")
		TSubclassOf<class AProjectile> PurifierClass;

private:
	void GenerateEarthquake(float Magnitude);

	TArray<FVector> GetEarthquakePoints(float Magnitude);

	void GeneratePurifier(float Magnitude);

	void GenerateRedSun(float Magnitude);

	bool bVisualiseEarthquakes;
};
