#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NaturalDisasterComponent.generated.h"


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

	void CancelEarthquake();

	void GenerateMeteor(float Magnitude);

	void GenerateRedSun(float Magnitude);

	void AlterSunGradually(float Target, float Increment);

	void CancelRedSun();

	UPROPERTY()
		class AGrid* Grid;
		
	UPROPERTY()
		float bDisasterChance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Natural Disaster")
		float Intensity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Natural Disaster")
		float Frequency;
};
