#include "Buildings/Misc/Parliament.h"

#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Universal/HealthComponent.h"

AParliament::AParliament()
{
	HealthComponent->MaxHealth = 300;
	HealthComponent->Health = HealthComponent->MaxHealth;
}

void AParliament::OnBuilt()
{
	Super::OnBuilt();

	Camera->CitizenManager->Election();

	Camera->CitizenManager->StartElectionTimer();
}