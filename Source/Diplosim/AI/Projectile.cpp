#include "AI/Projectile.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	Speed = 300.0f;
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

