#include "AI/Enemy.h"

#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"

#include "HealthComponent.h"
#include "AttackComponent.h"

AEnemy::AEnemy()
{
	GetCapsuleComponent()->SetCapsuleSize(9.0f, 9.0f);

	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -9.0f));

	AttackComponent->RangeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);

	HealthComponent->MaxHealth = 100;
	HealthComponent->Health = HealthComponent->MaxHealth;

	SpawnComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SpawnComponent"));
}

void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	SpawnComponent->SetWorldLocation(GetActorLocation());
}