#include "AI/Enemy.h"

#include "Components/CapsuleComponent.h"

#include "HealthComponent.h"

AEnemy::AEnemy()
{
	GetCapsuleComponent()->SetCapsuleSize(16.0f, 16.0f);

	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -15.0f));

	HealthComponent->MaxHealth = 100;
	HealthComponent->Health = HealthComponent->MaxHealth;
}