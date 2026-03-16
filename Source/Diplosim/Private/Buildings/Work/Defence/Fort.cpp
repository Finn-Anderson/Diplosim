#include "Buildings/Work/Defence/Fort.h"

#include "AI/Citizen/Citizen.h"

AFort::AFort()
{

}

int32 AFort::GetZ()
{
	return FMath::CeilToInt(BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize().Z / 75.0f);
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