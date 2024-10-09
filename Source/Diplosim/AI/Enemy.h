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

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
		class UNiagaraComponent* SpawnComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
		class UNiagaraComponent* EffectComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
		class UNiagaraComponent* ZapComponent;

	UPROPERTY()
		FLinearColor Colour;

	void Zap(FVector Location);
};
