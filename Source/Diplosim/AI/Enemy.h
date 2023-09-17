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
	void BeginPlay();

public:
	virtual void Tick(float DeltaTime) override;

	virtual void OnEnemyOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

	virtual void OnEnemyOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) override;
};
