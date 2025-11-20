#include "Buildings/Work/Defence/Fort.h"

#include "AI/AttackComponent.h"
#include "Components/SphereComponent.h"
#include "AI/Citizen.h"

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