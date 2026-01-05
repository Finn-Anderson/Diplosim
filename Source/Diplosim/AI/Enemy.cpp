#include "AI/Enemy.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "NiagaraComponent.h"

#include "Universal/HealthComponent.h"
#include "AIMovementComponent.h"
#include "Player/Camera.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"

AEnemy::AEnemy()
{
	HealthComponent->MaxHealth = 100;
	HealthComponent->Health = HealthComponent->MaxHealth;

	ZapComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ZapComponent"));
	ZapComponent->SetupAttachment(RootComponent, "ParticleSocket");
	ZapComponent->SetAutoActivate(false);

	SpawnLocation = FVector::Zero();
}

void AEnemy::Zap(FVector Location)
{
	ZapComponent->SetVariableLinearColor("Colour", Colour);
	ZapComponent->SetVariablePosition("StartLocation", MovementComponent->Transform.GetLocation() + Camera->Grid->AIVisualiser->GetAIHISM(this).Key->GetStaticMesh()->GetBounds().GetBox().GetSize().Z / 2.0f);
	ZapComponent->SetVariablePosition("EndLocation", Location);

	ZapComponent->Activate();
}