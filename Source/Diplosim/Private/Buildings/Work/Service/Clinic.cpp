#include "Buildings/Work/Service/Clinic.h"

#include "AI/Citizen/Citizen.h"
#include "Player/Camera.h"
#include "Player/Managers/DiseaseManager.h"
#include "Player/Managers/DiplosimTimerManager.h"
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

	Camera->DiseaseManager->Cure(Citizen);

	Camera->DiseaseManager->Infectible.Remove(Citizen);

	return true;
}

bool AClinic::RemoveCitizen(ACitizen* Citizen)
{
	bool bCheck = Super::RemoveCitizen(Citizen);

	if (!bCheck)
		return false;

	Camera->DiseaseManager->Infectible.Add(Citizen);

	Camera->TimerManager->RemoveTimer("Healing", Citizen);

	return true;
}