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

	void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UStaticMeshComponent* ProjectileMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
		class UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UAudioComponent* AudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
		class UNiagaraSystem* TrailSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
		class UNiagaraSystem* ExplosionSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake")
		TSubclassOf<class UCameraShakeBase> Shake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake")
		TSubclassOf<class UCameraShakeBase> MovementShake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
		int32 Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
		int32 Radius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
		bool bExplode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
		bool bDamageFallOff;

	UPROPERTY()
		FString FactionName;

	void SpawnNiagaraSystems(AActor* Launcher);

private:
	void OnHit(AActor* Actor, UActorComponent* Component, int32 Instance);

	void Explode(class ACamera* Camera);

	void HitActor(class ACamera* Camera, AActor* Actor);
};
