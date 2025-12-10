#include "Farm.h"

#include "Map/Grid.h"
#include "AI/Citizen.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Universal/HealthComponent.h"

AFarm::AFarm()
{
	HealthComponent->MaxHealth = 50;
	HealthComponent->Health = HealthComponent->MaxHealth;

	Yield = 5;

	TimeLength = 30.0f;

	bAffectedByFerility = true;
}

void AFarm::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (!GetOccupied().Contains(Citizen) || Camera->CitizenManager->FindTimer("Farm", this))
		return;

	if (CropMeshes[0]->GetRelativeScale3D().Z == 1.0f)
		ProductionDone(Citizen);
	else
		StartTimer(Citizen);
}

void AFarm::Production(ACitizen* Citizen)
{
	for (UStaticMeshComponent* mesh : CropMeshes)
		mesh->SetRelativeScale3D(mesh->GetRelativeScale3D() + FVector(0.0f, 0.0f, 0.1f));

	if (CropMeshes[0]->GetRelativeScale3D().Z >= 1.0f) {
		TArray<ACitizen*> workers = GetCitizensAtBuilding();

		if (workers.IsEmpty())
			return;

		Super::Production(Citizen);

		ProductionDone(workers[0]);
	}
	else
		StartTimer(Citizen);
}

void AFarm::ProductionDone(ACitizen* Citizen)
{
	Citizen->Carry(Crop->GetDefaultObject<AResource>(), GetYield() * FMath::Max(Boosters * 2, 1), this);

	StoreResource(Citizen);

	for (UStaticMeshComponent* mesh : CropMeshes)
		mesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.0f));

	StartTimer(Citizen);
}

void AFarm::StartTimer(ACitizen* Citizen)
{
	TArray<FTimerParameterStruct> params;
	Camera->CitizenManager->SetParameter(Citizen, params);
	Camera->CitizenManager->CreateTimer("Farm", this, TimeLength / 10.0f / Citizen->GetProductivity(), "Production", params, false, true);
}

int32 AFarm::GetFertility()
{
	int32 fertility = 5;
	
	if (bAffectedByFerility) {
		FTileStruct* tile = Camera->Grid->GetTileFromLocation(GetActorLocation());

		fertility = tile->Fertility;
	}

	return fertility;
}

int32 AFarm::GetYield()
{
	return Yield * GetFertility();
}