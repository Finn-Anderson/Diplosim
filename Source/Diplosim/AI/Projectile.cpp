#include "AI/Projectile.h"

#include "GameFramework/ProjectileMovementComponent.h"

#include "AI.h"
#include "Buildings/Building.h"
#include "HealthComponent.h"

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
	
	ProjectileMesh->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
}

void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) 
{
	if (OtherActor == GetOwner())
		return;

	if (OtherActor->IsA<AAI>()) {
		AAI* ai = Cast<AAI>(OtherActor);

		ai->HealthComponent->TakeHealth(Damage, GetActorLocation());
	}
	else if (OtherActor->IsA<ABuilding>()) {
		ABuilding* building = Cast<ABuilding>(OtherActor);

		building->HealthComponent->TakeHealth(Damage, GetActorLocation());
	}

	Destroy();
}