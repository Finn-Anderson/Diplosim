#include "Buildings/Misc/Portal.h"

#include "Universal/HealthComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/ConquestManager.h"

APortal::APortal()
{
	
}

void APortal::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (HealthComponent->GetHealth() == 0)
		return;

	Camera->ConquestManager->StartTransmissionTimer(Citizen);
}