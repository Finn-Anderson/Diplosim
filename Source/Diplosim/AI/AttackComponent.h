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

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetProjectileClass(TSubclassOf<class AProjectile> OtherClass);

	void PickTarget();

	FFavourabilityStruct GetActorFavourability(AActor* Actor);

	void CanHit(AActor* Target);

	void Attack();

	void Throw();

	void Melee();

	void ClearAttacks();

	UPROPERTY()
		TArray<AActor*> OverlappingEnemies;

	UPROPERTY()
		TArray<AActor*> MeleeableEnemies;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		class USphereComponent* RangeComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		int32 Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		TSubclassOf<class AProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		float AttackTime;

	UPROPERTY()
		FTimerHandle AttackTimer;

	UPROPERTY()
		bool bCanAttack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		UAnimSequence* MeleeAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		UAnimSequence* RangeAnim;

	UPROPERTY()
		AActor* CurrentTarget;

	UPROPERTY(BlueprintReadOnly, Category = "Attack")
		int32 DamageMultiplier;
};
