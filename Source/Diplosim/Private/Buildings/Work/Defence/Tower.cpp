#include "Buildings/Work/Defence/Tower.h"

#include "Components/SphereComponent.h"

#include "Universal/AttackComponent.h"
#include "Universal/HealthComponent.h"

ATower::ATower()
{
	HealthComponent->MaxHealth = 50;
	HealthComponent->Health = HealthComponent->MaxHealth;
	
	AttackComponent = CreateDefaultSubobject<UAttackComponent>(TEXT("AttackComponent"));
	AttackComponent->AttackTime = 5.0f;

	RangeComponent->SetSphereRadius(1000.0f);
}

void ATower::BeginPlay()
{
	Super::BeginPlay();

	AttackComponent->Camera = Camera;
}