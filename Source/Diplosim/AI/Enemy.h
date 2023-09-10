#pragma once

#include "CoreMinimal.h"
#include "AI/AI.h"
#include "Enemy.generated.h"

USTRUCT()
struct FAttackStruct
{
	GENERATED_USTRUCT_BODY()

	class AActor* Actor;

	int32 Dmg;

	int32 Hp;

	FAttackStruct()
	{
		Actor = nullptr;
		Dmg = 0;
		Hp = 0;
	}
};

UCLASS()
class DIPLOSIM_API AEnemy : public AAI
{
	GENERATED_BODY()
	
public:
	AEnemy();

protected:
	void BeginPlay();

public:
	virtual void OnEnemyOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

	virtual void OnEnemyOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) override;

	void Attack();

	TArray<AActor*> OverlappingEnemies;

	FTimerHandle DamageTimer;
};
