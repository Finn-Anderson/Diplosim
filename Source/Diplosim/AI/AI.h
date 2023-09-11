#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AI.generated.h"

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
class DIPLOSIM_API AAI : public ACharacter
{
	GENERATED_BODY()

public:
	AAI();

protected:
	virtual void BeginPlay() override;

public:	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UStaticMeshComponent* AIMesh;

	UPROPERTY()
		class AAIController* AIController;

	UFUNCTION()
		virtual void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		virtual void OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Fighting
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		class UHealthComponent* HealthComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		class USphereComponent* AttackComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		int32 Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		float Range;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		float TimeToAttack;
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		TSubclassOf<class AProjectile> ProjectileClass;

	TArray<AActor*> OverlappingEnemies;

	FTimerHandle DamageTimer;

	UFUNCTION()
		virtual void OnEnemyOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		virtual void OnEnemyOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void Attack();

	bool CanThrow(AActor* Target);

	void Throw(AActor* Target);

	float Theta;


	// Movement
	void MoveTo(AActor* Location);

	AActor* Goal;

};
