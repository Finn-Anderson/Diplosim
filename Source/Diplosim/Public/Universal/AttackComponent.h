#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttackComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UAttackComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAttackComponent();

	void SetProjectileClass(TSubclassOf<class AProjectile> OtherClass);

	void PickTarget();

	void Throw();

	void Melee();

	void ClearAttacks();

	UPROPERTY()
		TArray<AActor*> OverlappingEnemies;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		int32 Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		TSubclassOf<class AProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		float AttackTime;

	UPROPERTY()
		float AttackTimer;

	UPROPERTY()
		bool bShowMercy;

	UPROPERTY()
		AActor* CurrentTarget;

	UPROPERTY(BlueprintReadOnly, Category = "Attack")
		float DamageMultiplier;

	UPROPERTY()
		class ACamera* Camera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
		class USoundBase* OnHitSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
		class USoundBase* ZapSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Morale")
		bool bFactorMorale;

private:
	AActor* GetFavouredActor(AActor* CurrentFavoured, AActor* Actor);

	bool IsMoraleHigh();

	void Attack(AActor* Target);

	FVector GetThrowLocation(AActor* Actor);

	bool bClearAttacks;

	double LastUpdateTime;
};
