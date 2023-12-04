#include "AI/Projectile.h"

#include "GameFramework/ProjectileMovementComponent.h"
//#include "../../Engine/Plugins/FX/Niagara/Source/Niagara/Public/NiagaraFunctionLibrary.h"
//#include "../../Engine/Plugins/FX/Niagara/Source/Niagara/Public/NiagaraComponent.h"

#include "AI.h"
#include "Buildings/Building.h"
#include "HealthComponent.h"
#include "AttackComponent.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AIMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->bCastDynamicShadow = true;
	ProjectileMesh->CastShadow = true;

    ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
    ProjectileMovementComponent->SetUpdatedComponent(ProjectileMesh);
    ProjectileMovementComponent->bRotationFollowsVelocity = true;
    ProjectileMovementComponent->InitialSpeed = 300.0f;
    ProjectileMovementComponent->MaxSpeed = 300.0f;

	Damage = 20;
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (TrailSystem) {
		//UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(TrailSystem, WeaponMuzzle, NAME_None, FVector(0.f), FRotator(0.f), EAttachLocation::Type::KeepRelativeOffset, true);
		//NiagaraComp->SetNiagaraVariableFloat(FString("StrengthCoef"), CoefStrength);
	}
	
	ProjectileMesh->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
}

void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) 
{
	if (OtherActor == GetOwner())
		return;

	UHealthComponent* healthComp = OtherActor->GetComponentByClass<UHealthComponent>();

	if (healthComp) {
		healthComp->TakeHealth(Damage, GetActorLocation());
	}

	Destroy();
}