#include "Buildings/Work/Defence/Fort.h"

#include "AI/AttackComponent.h"
#include "Components/SphereComponent.h"
#include "AI/Citizen.h"

AFort::AFort()
{

}

void AFort::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (!GetOccupied().Contains(Citizen))
		return;

	int32 z = FMath::Clamp(BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize().Z / 100.0f, 1, 5);

	Citizen->AttackComponent->RangeComponent->SetSphereRadius(Citizen->AttackComponent->RangeComponent->GetUnscaledSphereRadius() * z);
}

void AFort::Leave(ACitizen* Citizen)
{
	Super::Leave(Citizen);

	if (!GetOccupied().Contains(Citizen))
		return;

	int32 z = FMath::Clamp(BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize().Z / 100.0f, 1, 5);

	Citizen->AttackComponent->RangeComponent->SetSphereRadius(Citizen->AttackComponent->RangeComponent->GetUnscaledSphereRadius() / z);
}