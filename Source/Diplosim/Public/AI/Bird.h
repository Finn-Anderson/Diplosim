#pragma once

#include "CoreMinimal.h"
#include "AI/AI.h"
#include "Bird.generated.h"

UCLASS()
class DIPLOSIM_API ABird : public AAI
{
	GENERATED_BODY()
	
public:
	ABird();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
		TArray<class USoundBase*> Chirps;

	void SetupBird();

	UFUNCTION()
		void Poo();

	UFUNCTION()
		void Chirp();

private:
	int32 GetPooTimer();

	int32 GetChirpTimer();
};
