#include "Buildings/Work/Defence/Fort.h"

#include "AI/AttackComponent.h"

AFort::AFort()
{
	
}

void AFort::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (CitizensWithBoostedDamage.Contains(Citizen))
		return;

	Citizen->AttackComponent->Damage *= 3;

	CitizensWithBoostedDamage.Add(Citizen);
}

bool AFort::RemoveCitizen(ACitizen* Citizen)
{
	bool bCheck = Super::AddCitizen(Citizen);

	if (!bCheck || !CitizensWithBoostedDamage.Contains(Citizen))
		return false;

	Citizen->AttackComponent->Damage /= 3;

	CitizensWithBoostedDamage.Remove(Citizen);

	return true;
}