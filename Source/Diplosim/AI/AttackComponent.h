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
	void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION()
		void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

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

	FTimerHandle AttackTimer;

	bool bCanAttack;

	UPROPERTY()
		AAI* Owner;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		UAnimSequence* MeleeAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		UAnimSequence* RangeAnim;

	UPROPERTY()
		AActor* CurrentTarget;
};
