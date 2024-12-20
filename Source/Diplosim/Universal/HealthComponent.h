#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UHealthComponent();

public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Health
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		int32 MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		int32 Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		float HealthMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		class UMaterial* OnHitEffect;

	void AddHealth(int32 Amount);

	void TakeHealth(int32 Amount, AActor* Attacker);

	bool IsMaxHealth();

	void RemoveDamageOverlay();

	int32 GetHealth();

	void SetHealthMultiplier(float Multiplier);


	// Death
	void Death(AActor* Attacker);

	void Clear(AActor* Attacker);

	UPROPERTY()
		FVector RebuildLocation;

	UPROPERTY()
		FVector CrumbleLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake")
		TSubclassOf<class UCameraShakeBase> Shake;
};
