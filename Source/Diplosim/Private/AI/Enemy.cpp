#include "AI/Enemy.h"

#include "NiagaraComponent.h"

#include "AI/AIMovementComponent.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Map/AIInstancedStaticMeshComponent.h"
#include "Player/Camera.h"
#include "Universal/HealthComponent.h"

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
	Async(EAsyncExecution::TaskGraphMainTick, [this, Location]() {
		ZapComponent->SetVariableLinearColor("Colour", Colour);
		ZapComponent->SetVariablePosition("StartLocation", MovementComponent->Transform.GetLocation() + Camera->Grid->AIVisualiser->GetAIHISM(this).Key->GetStaticMesh()->GetBounds().Origin);
		ZapComponent->SetVariablePosition("EndLocation", Location);

		ZapComponent->Activate();
	});
}