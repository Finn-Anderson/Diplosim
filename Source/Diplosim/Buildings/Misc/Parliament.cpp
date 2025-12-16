#include "Buildings/Misc/Parliament.h"

#include "Player/Camera.h"
#include "Player/Managers/PoliticsManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Universal/HealthComponent.h"

AParliament::AParliament()
{
	HealthComponent->MaxHealth = 300;
	HealthComponent->Health = HealthComponent->MaxHealth;
}

void AParliament::OnBuilt()
{
	Super::OnBuilt();

	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	Camera->PoliticsManager->Election(*faction);

	Camera->PoliticsManager->StartElectionTimer(faction);
}