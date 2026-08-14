#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AudioManager.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UAudioManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAudioManager();

	void SetupAttachment(class USceneComponent* SceneComponent);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UAudioComponent* InteractAudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UAudioComponent* AmbientWindAudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UAudioComponent* AmbientTreesAudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UAudioComponent* AmbientSeaAudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UAudioComponent* MusicAudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class USoundBase* InteractSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class USoundBase* EventSound;

	UPROPERTY()
		class ACamera* Camera;

	void PlayAmbientSound(class UAudioComponent* AudioComponent, class USoundBase* Sound, float Pitch = -1.0f);

	void PlayInteractSound(class USoundBase* Sound, float Pitch = 1.0f);

	void CalculateAmbientEnvironmentSound(bool bForce = false);

	void AlterWindPitch(float WindSpeedPerc);

	void ClearAmbientSound();

private:
	FCriticalSection AmbientLock;
	int32 Counter;

	FTransform LastTransform;

	float WindSpeedPercentage;

	float GetWindSpeedVolume();
};
