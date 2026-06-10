#include "Buildings/Misc/Special/Special.h"

#include "Player/Camera.h"
#include "Player/Managers/ResearchManager.h"
#include "Universal/HealthComponent.h"

ASpecial::ASpecial()
{
	HealthComponent->Health = 0;
	HealthComponent->MaxHealth = 200;
}

void ASpecial::Rebuild(FString NewFactionName)
{
	if (!Camera->ResearchManager->IsResearched("Unlock ancient buildings", NewFactionName)) {
		Camera->DisplayWarning("Must research: Unlock Ancient Buildings");

		return;
	}

	Super::Rebuild(NewFactionName);
}

void ASpecial::OnBuilt()
{
	ActualMesh = ReplacementMesh;

	Super::OnBuilt();
}