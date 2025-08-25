#include "Buildings/Misc/Special/Special.h"

#include "Universal/HealthComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/ResearchManager.h"

ASpecial::ASpecial()
{
	HealthComponent->Health = 0;
	HealthComponent->MaxHealth = 200;
}

void ASpecial::Rebuild()
{
	if (!Camera->ResearchManager->IsReseached("Unlock ancient buildings", FactionName)) {
		Camera->DisplayWarning("Must research: Unlock Ancient Buildings");

		FactionName = "";

		return;
	}

	Super::Rebuild();
}

void ASpecial::Build(bool bRebuild, bool bUpgrade, int32 Grade)
{
	Super::Build(bRebuild, bUpgrade, Grade);

	ActualMesh = ReplacementMesh;
}