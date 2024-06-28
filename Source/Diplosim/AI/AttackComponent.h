#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttackComponent.generated.h"

UENUM()
enum class ECanAttack : uint8
{
	Invalid,
	Timer,
	Valid
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

	void Melee(AActor* Target);

	void ClearAttacks();

	TArray<AActor*> OverlappingEnemies;

	TArray<AActor*> MeleeableEnemies;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		class USphereComponent* RangeComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		int32 Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		TSubclassOf<class AProjectile> ProjectileClass;

	FTimerHandle AttackTimer;

	ECanAttack CanAttack;

	AAI* Owner;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		UAnimSequence* MeleeAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		UAnimSequence* RangeAnim;

	AActor* CurrentTarget;
};
