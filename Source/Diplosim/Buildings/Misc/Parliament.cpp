#include "Buildings/Misc/Parliament.h"

#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
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

	Camera->CitizenManager->Election(*faction);

	Camera->CitizenManager->StartElectionTimer(faction);
}