#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttackComponent.generated.h"

USTRUCT()
struct FFavourabilityStruct
{
	GENERATED_USTRUCT_BODY()

	int32 Hp;

	int32 Dmg;

	double Dist;

	FFavourabilityStruct()
	{
		Hp = 10000000;
		Dmg = 1;
		Dist = 10000000.0f;
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UAttackComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAttackComponent();

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetProjectileClass(TSubclassOf<class AProjectile> OtherClass);

	void ClearAttacks();

	void Throw();

	void Melee();

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
		int32 DamageMultiplier;

	UPROPERTY()
		class ACamera* Camera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
		class USoundBase* OnHitSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
		class USoundBase* ZapSound;

	UPROPERTY(BlueprintReadOnly, Category = "Attack")
		bool bFactorMorale;

private:
	void PickTarget();

	FFavourabilityStruct GetActorFavourability(AActor* Actor);

	bool IsMoraleHigh();

	void Attack();
};
