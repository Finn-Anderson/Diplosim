#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttackComponent.generated.h"

USTRUCT()
struct FAttackStruct
{
	GENERATED_USTRUCT_BODY()

	class AActor* Actor;

	int32 Dmg;

	int32 Hp;

	FVector::FReal Length;

	FAttackStruct()
	{
		Actor = nullptr;
		Dmg = 0;
		Hp = 0;
		Length = 0;
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
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION()
		void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void PickTarget(TArray<AActor*> Targets);

	bool CanThrow(AActor* Target, TArray<FVector> Locations);

	bool CanHit(AActor* Target, FVector::FReal Length);

	void Attack(AActor* Target);

	void Throw(AActor* Target);

	bool CanAttack();

	TArray<AActor*> OverlappingEnemies;

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

	FVector ThrowLocation;

	FTimerHandle AttackTimer;

	float Theta;

	AActor* Owner;
};