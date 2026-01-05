#pragma once

#include "CoreMinimal.h"
#include "AI/AI.h"
#include "Enemy.generated.h"

UCLASS()
class DIPLOSIM_API AEnemy : public AAI
{
	GENERATED_BODY()
	
public:
	AEnemy();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
		class UNiagaraComponent* ZapComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
		FVector SpawnLocation;

	void Zap(FVector Location);
};
