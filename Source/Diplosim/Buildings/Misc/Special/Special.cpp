#include "Buildings/Misc/Special/Special.h"

#include "Universal/HealthComponent.h"

ASpecial::ASpecial()
{
	HealthComponent->Health = 0;
	HealthComponent->MaxHealth = 200;
}

void ASpecial::Build(bool bRebuild, bool bUpgrade, int32 Grade)
{
	Super::Build(bRebuild, bUpgrade, Grade);

	ActualMesh = ReplacementMesh;
}