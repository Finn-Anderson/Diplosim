#include "AI/Enemy.h"

#include "Kismet/GameplayStatics.h"

#include "HealthComponent.h"
#include "Buildings/Broch.h"

AEnemy::AEnemy()
{
	HealthComponent->Health = HealthComponent->MaxHealth;
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