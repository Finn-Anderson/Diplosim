#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

UCLASS()
class DIPLOSIM_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectile();

protected:
	virtual void BeginPlay() override;

public:	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UStaticMeshComponent* ProjectileMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
		class UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
		class UNiagaraSystem* TrailSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
		class UNiagaraSystem* ExplosionSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake")
		TSubclassOf<class UCameraShakeBase> Shake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
		int32 Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
		int32 Radius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
		bool bExplode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
		bool bDamageFallOff;

	UFUNCTION()
		void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	void SpawnNiagaraSystems(AActor* Launcher);
};
