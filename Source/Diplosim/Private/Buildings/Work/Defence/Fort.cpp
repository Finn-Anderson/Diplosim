#include "Buildings/Work/Defence/Fort.h"

#include "AI/Citizen/Citizen.h"

AFort::AFort()
{

}

int32 AFort::GetZ()
{
	return FMath::Clamp(BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize().Z / 100.0f, 1, 5);
}

void AFort::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (!GetOccupied().Contains(Citizen))
		return;

	Citizen->Range *= GetZ();
}

void AFort::Leave(ACitizen* Citizen)
{
	Super::Leave(Citizen);

	if (!GetOccupied().Contains(Citizen))
		return;

	Citizen->Range /= GetZ();
}