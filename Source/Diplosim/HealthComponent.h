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

	void AddHealth(int32 Amount);

	void TakeHealth(int32 Amount, FVector DeathInstigatorLocation = FVector(0, 0, 0));

	int32 GetHealth();

public:
	// Death
	void Death(FVector DeathInstigatorLocation);

	void ClearRagdoll();
};
