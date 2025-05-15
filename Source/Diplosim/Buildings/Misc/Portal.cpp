#include "Buildings/Misc/Portal.h"

#include "Universal/HealthComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/ConquestManager.h"

APortal::APortal()
{
	HealthComponent->Health = 0;
	HealthComponent->MaxHealth = 200;
}

void APortal::Build(bool bRebuild, bool bUpgrade, int32 Grade)
{
	Super::Build(bRebuild, bUpgrade, Grade);

	ActualMesh = ReplacementMesh;
}

void APortal::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	Camera->ConquestManager->StartTransmissionTimer(Citizen);
}