#include "Buildings/Work/Defence/Fort.h"

#include "AI/AttackComponent.h"
#include "Components/SphereComponent.h"

AFort::AFort()
{

}

void AFort::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (GetOccupied().Contains(Citizen))
		Citizen->AttackComponent->RangeComponent->SetSphereRadius(800.0f);
}

void AFort::Leave(ACitizen* Citizen)
{
	Super::Leave(Citizen);

	if (GetOccupied().Contains(Citizen))
		Citizen->AttackComponent->RangeComponent->SetSphereRadius(400.0f);
}