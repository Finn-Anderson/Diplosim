#include "Buildings/Work/Service/Religion.h"

#include "Components/DecalComponent.h"

#include "Buildings/House.h"
#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"

AReligion::AReligion()
{
	DecalComponent->SetVisibility(true);

	bRange = true;
}

void AReligion::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	Citizen->bWorshipping = true;
}

void AReligion::Leave(ACitizen* Citizen)
{
	Super::Leave(Citizen);

	Citizen->bWorshipping = false;
}