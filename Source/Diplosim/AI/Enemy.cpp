#include "AI/Enemy.h"

#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"

#include "Universal/HealthComponent.h"
#include "AttackComponent.h"

AEnemy::AEnemy()
{
	GetCapsuleComponent()->SetCapsuleSize(9.0f, 9.0f);

	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -9.0f));

	AttackComponent->RangeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);

	HealthComponent->MaxHealth = 100;
	HealthComponent->Health = HealthComponent->MaxHealth;

	SpawnComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SpawnComponent"));

	EffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("EffectComponent"));
	EffectComponent->SetupAttachment(GetMesh(), "ParticleSocket");

	ZapComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ZapComponent"));
	ZapComponent->SetupAttachment(GetMesh(), "ParticleSocket");
	ZapComponent->SetAutoActivate(false);
}

void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	SpawnComponent->SetWorldLocation(GetActorLocation());

	GetMesh()->PlayAnimation(MoveAnim, true);
}

void AEnemy::Zap(FVector Location)
{
	ZapComponent->SetVariableLinearColor("Colour", Colour);
	ZapComponent->SetVariablePosition("EndLocation", Location);

	ZapComponent->Activate();
}