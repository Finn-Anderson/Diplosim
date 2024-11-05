#include "Buildings/Misc/Parliament.h"

#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"

AParliament::AParliament()
{

}

void AParliament::OnBuilt()
{
	Super::OnBuilt();

	Camera->CitizenManager->Election();

	Camera->CitizenManager->StartElectionTimer();
}