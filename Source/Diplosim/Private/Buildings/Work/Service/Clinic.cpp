#include "Buildings/Work/Service/Clinic.h"

#include "AI/DiplosimAIController.h"
#include "AI/Citizen/Citizen.h"
#include "Player/Camera.h"
#include "Player/Managers/DiseaseManager.h"
#include "Player/Managers/DiplosimTimerManager.h"
#include "Player/Managers/ConquestManager.h"
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

	Camera->DiseaseManager->Cure(Citizen, Citizen);

	return true;
}

bool AClinic::RemoveCitizen(ACitizen* Citizen)
{
	bool bCheck = Super::RemoveCitizen(Citizen);

	if (!bCheck)
		return false;

	Camera->TimerManager->RemoveTimer("Healing", Citizen);

	return true;
}

bool AClinic::IsAtWork(ACitizen* Citizen)
{
	bool bIsWorking = Super::IsAtWork(Citizen);
	AActor* goal = Citizen->AIController->MoveRequest.GetGoalActor();

	if (!bIsWorking && IsValid(goal) && Camera->ConquestManager->GetFaction(FactionName)->Citizens.Contains(goal) && !Cast<ACitizen>(goal)->HealthIssues.IsEmpty())
		bIsWorking = true;

	return bIsWorking;
}