#include "Buildings/Work/Service/Clinic.h"

#include "NiagaraComponent.h"
#include "Components/WidgetComponent.h"

#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Universal/HealthComponent.h"

AClinic::AClinic()
{
	HealthComponent->MaxHealth = 50;
	HealthComponent->Health = HealthComponent->MaxHealth;
}

bool AClinic::AddCitizen(ACitizen* Citizen)
{
	bool bCheck = Super::AddCitizen(Citizen);

	if (!bCheck)
		return false;

	Camera->CitizenManager->Cure(Citizen);

	Camera->CitizenManager->Infectible.Remove(Citizen);

	return true;
}

bool AClinic::RemoveCitizen(ACitizen* Citizen)
{
	bool bCheck = Super::RemoveCitizen(Citizen);

	if (!bCheck)
		return false;

	Camera->CitizenManager->Infectible.Add(Citizen);

	return true;
}

void AClinic::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (!GetOccupied().Contains(Citizen))
		return;

	Camera->CitizenManager->PickCitizenToHeal(Citizen);
}