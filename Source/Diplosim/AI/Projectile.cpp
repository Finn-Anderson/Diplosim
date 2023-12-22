#include "AI/Projectile.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

#include "AI.h"
#include "Buildings/Building.h"
#include "HealthComponent.h"
#include "AttackComponent.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AIMesh"));
	ProjectileMesh->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	ProjectileMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	ProjectileMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody, ECollisionResponse::ECR_Overlap);
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->bCastDynamicShadow = true;
	ProjectileMesh->CastShadow = true;

    ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
    ProjectileMovementComponent->SetUpdatedComponent(ProjectileMesh);
    ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->InitialSpeed = 650.0f;
    ProjectileMovementComponent->MaxSpeed = 650.0f;

	Damage = 20;
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (TrailSystem) {
		UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(TrailSystem, ProjectileMesh, FName("Trail1"), FVector(0.0f), FRotator(0.0f), EAttachLocation::Type::KeepRelativeOffset, true);
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