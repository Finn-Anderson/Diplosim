#include "Buildings/Work/Service/Clinic.h"

#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"

AClinic::AClinic()
{

}

void AClinic::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (!GetOccupied().Contains(Citizen))
		return;

	Citizen->CaughtDiseases.Empty();

	Camera->CitizenManager->PickCitizenToHeal(Citizen);
}