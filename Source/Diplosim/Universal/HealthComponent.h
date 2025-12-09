#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UHealthComponent();

private:
	virtual void BeginPlay() override;

public:
	UPROPERTY()
		class ACamera* Camera;

	// Health
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		int32 MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		int32 Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		float HealthMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		class UMaterial* OnHitEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UAudioComponent* HitAudioComponent;

	void AddHealth(int32 Amount);

	UFUNCTION()
		void TakeHealth(int32 Amount, AActor* Attacker, USoundBase* Sound = nullptr);

	bool IsMaxHealth();

	int32 GetHealth();

	UFUNCTION()
		void ApplyDamageOverlay(bool bLoad = false);

	UFUNCTION()
		void RemoveDamageOverlay();

	// Death
	void Death(AActor* Attacker);

	UFUNCTION()
		void Clear(FFactionStruct Faction, AActor* Attacker);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake")
		TSubclassOf<class UCameraShakeBase> Shake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
		class UNiagaraSystem* DeathSystem;

	// Fire
	UFUNCTION()
		void OnFire(int32 Counter = 0);
};
