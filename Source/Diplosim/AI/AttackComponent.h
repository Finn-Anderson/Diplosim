#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttackComponent.generated.h"

USTRUCT()
struct FAttackStruct
{
	GENERATED_USTRUCT_BODY()

	AActor* Actor;

	int32 Dmg;

	int32 Hp;

	FAttackStruct()
	{
		Actor = nullptr;
		Dmg = 1;
		Hp = 1000000;
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UAttackComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAttackComponent();

protected:
	virtual void BeginPlay() override;

public:	
	UFUNCTION()
		void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void SetProjectileClass(TSubclassOf<class AProjectile> OtherClass);

	void PickTarget();

	void CanHit(AActor* Target);

	void Attack(AActor* Target);

	void Throw(AActor* Target);

	bool CanAttack();

	TArray<TWeakObjectPtr<AActor>> OverlappingEnemies;

	TArray<TWeakObjectPtr<AActor>> MeleeableEnemies;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		class USphereComponent* RangeComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		int32 Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		float TimeToAttack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		TSubclassOf<class AProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		TArray<TSubclassOf<class AActor>> EnemyClasses;

	FTimerHandle AttackTimer;

	AAI* Owner;
};
