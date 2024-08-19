#include "AI/Projectile.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/SphereComponent.h"

#include "Universal/HealthComponent.h"
#include "Enemy.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AIMesh"));
	ProjectileMesh->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	ProjectileMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	ProjectileMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody, ECollisionResponse::ECR_Overlap);
	ProjectileMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->bCastDynamicShadow = true;
	ProjectileMesh->CastShadow = true;

    ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
    ProjectileMovementComponent->SetUpdatedComponent(ProjectileMesh);
    ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->InitialSpeed = 600.0f;
    ProjectileMovementComponent->MaxSpeed = 600.0f;

	ExplosionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("RangeComponent"));
	ExplosionComponent->SetCollisionProfileName("Spectator", true);
	ExplosionComponent->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	ExplosionComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Ignore);
	ExplosionComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody, ECollisionResponse::ECR_Ignore);
	ExplosionComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	ExplosionComponent->SetSphereRadius(300.0f);

	Damage = 10;

	bExplode = false;
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (TrailSystem)
		UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(TrailSystem, ProjectileMesh, FName("Trail1"), FVector(0.0f), FRotator(0.0f), EAttachLocation::Type::KeepRelativeOffset, true);

	ExplosionComponent->OnComponentBeginOverlap.AddDynamic(this, &AProjectile::OnOverlapBegin);
	ExplosionComponent->OnComponentEndOverlap.AddDynamic(this, &AProjectile::OnOverlapEnd);
	
	ProjectileMesh->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
}

void AProjectile::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->IsA<AEnemy>())
		OverlappingActors.Add(OtherActor);
}

void AProjectile::OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OverlappingActors.Contains(OtherActor))
		OverlappingActors.Remove(OtherActor);
}

void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) 
{
	if (OtherActor == GetOwner())
		return;

	if (bExplode) {
		for (AActor* actor : OverlappingActors) {
			UHealthComponent* healthComp = actor->GetComponentByClass<UHealthComponent>();

			float distance = FVector::Dist(GetActorLocation(), actor->GetActorLocation());

			int32 dmg = Damage / FMath::Pow(FMath::LogX(50.0f, distance), 5.0f);

			healthComp->TakeHealth(dmg, this);
		}
	}
	else {
		UHealthComponent* healthComp = OtherActor->GetComponentByClass<UHealthComponent>();

		if (healthComp)
			healthComp->TakeHealth(Damage, this);
	}

	Destroy();
}