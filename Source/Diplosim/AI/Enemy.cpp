#include "AI/Enemy.h"

#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"

#include "Universal/HealthComponent.h"
#include "AttackComponent.h"

AEnemy::AEnemy()
{
	Capsule->SetCapsuleSize(9.0f, 9.0f);
	Capsule->SetCollisionObjectType(ECollisionChannel::ECC_GameTraceChannel4);

	Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, -9.0f));

	HealthComponent->MaxHealth = 100;
	HealthComponent->Health = HealthComponent->MaxHealth;

	SpawnComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SpawnComponent"));

	ZapComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ZapComponent"));
	ZapComponent->SetupAttachment(RootComponent, "ParticleSocket");
	ZapComponent->SetAutoActivate(false);
}

void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	SpawnComponent->SetWorldLocation(GetActorLocation());
}

void AEnemy::Zap(FVector Location)
{
	ZapComponent->SetVariableLinearColor("Colour", Colour);
	ZapComponent->SetVariablePosition("EndLocation", Location);

	ZapComponent->Activate();
}