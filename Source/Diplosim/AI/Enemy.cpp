#include "AI/Enemy.h"

#include "Kismet/GameplayStatics.h"

#include "Citizen.h"
#include "HealthComponent.h"
#include "Buildings/Broch.h"

AEnemy::AEnemy()
{
	PrimaryActorTick.bStartWithTickEnabled = true;

	GetMesh()->SetSimulatePhysics(true);
}

void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	SetActorTickEnabled(true);
}

void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float spin = FMath::Abs(GetVelocity().Length());
	SetActorRotation(GetActorRotation() + FRotator(spin, 0.0f, 0.0f));
}

void AEnemy::MoveToBroch()
{
	if (!IsValidLowLevelFast())
		return;

	TArray<AActor*> brochs;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABroch::StaticClass(), brochs);

	if (brochs.IsEmpty() || HealthComponent->Health == 0)
		return;

	MoveTo(brochs[0]);
}