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

private:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

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

	void AddHealth(int32 Amount);

	void TakeHealth(int32 Amount, AActor* Attacker);

	bool IsMaxHealth();

	void RemoveDamageOverlay();

	int32 GetHealth();

	// Death
	void Death(AActor* Attacker, int32 Force);

	void Clear(AActor* Attacker);

	UPROPERTY()
		FVector RebuildLocation;

	UPROPERTY()
		FVector CrumbleLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake")
		TSubclassOf<class UCameraShakeBase> Shake;

	// Fire
	void OnFire(int32 Counter = 0);
};
