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
	// Health
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		int32 MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		int32 Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		class UMaterial* OnHitEffect;

	void AddHealth(int32 Amount);

	void TakeHealth(int32 Amount, AActor* Attacker);

	bool IsMaxHealth();

	void RemoveDamageOverlay();

	int32 GetHealth();


	// Death
	void Death(AActor* Attacker);

	void Clear(AActor* Attacker);
};
