#include "Buildings/Work/Service/Research.h"

#include "Player/Camera.h"
#include "Player/Managers/ResearchManager.h"
#include "Player/Managers/CitizenManager.h"
#include "AI/Citizen.h"

AResearch::AResearch()
{
	TimeLength = 5.0f;
}

void AResearch::Enter(class ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (!GetOccupied().Contains(Citizen))
		return;

	FTimerStruct* timer = Camera->CitizenManager->FindTimer("Research", this);

	if (timer == nullptr)
		SetResearchTimer();
	else
		UpdateResearchTimer();
}

void AResearch::Leave(class ACitizen* Citizen)
{
	Super::Leave(Citizen);

	if (!GetCitizensAtBuilding().IsEmpty())
		return;

	Camera->CitizenManager->RemoveTimer("Research", this);
}

float AResearch::GetTime()
{
	float time = TimeLength / GetCitizensAtBuilding().Num();

	for (ACitizen* citizen : GetCitizensAtBuilding())
		time -= (time / GetCitizensAtBuilding().Num()) * (citizen->GetProductivity() - 1.0f);

	return time;
}

void AResearch::SetResearchTimer()
{
	FTimerStruct timer;
	timer.CreateTimer("Research", this, GetTime(), FTimerDelegate::CreateUObject(this, &AResearch::Production, GetCitizensAtBuilding()[0]), true);

	Camera->CitizenManager->Timers.Add(timer);
}

void AResearch::UpdateResearchTimer()
{
	FTimerStruct* timer = Camera->CitizenManager->FindTimer("Research", this);

	if (timer == nullptr)
		return;

	timer->Target = GetTime();
}

void AResearch::Production(class ACitizen* Citizen)
{
	Super::Production(Citizen);

	Camera->ResearchManager->Research(1.0f);
}