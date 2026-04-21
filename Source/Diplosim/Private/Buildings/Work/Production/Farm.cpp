#include "Buildings/Work/Production/Farm.h"

#include "AI/Citizen/Citizen.h"
#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/DiplosimTimerManager.h"
#include "Universal/HealthComponent.h"

AFarm::AFarm()
{
	HealthComponent->MaxHealth = 50;
	HealthComponent->Health = HealthComponent->MaxHealth;

	Yield = 5;

	TimeLengthHours = 4.0f;

	CropHeight = 0.0f;

	bAffectedByFerility = true;
}

void AFarm::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (!GetOccupied().Contains(Citizen) || Camera->TimerManager->FindTimer("Farm", this))
		return;

	if (GetCropMeshes()[0]->GetRelativeScale3D().Z == 1.0f)
		ProductionDone(Citizen);
	else
		StartTimer(Citizen);
}

void AFarm::Production(ACitizen* Citizen)
{
	CropHeight += 0.1f;
	TArray<UStaticMeshComponent*> meshes = GetCropMeshes();

	for (UStaticMeshComponent* mesh : meshes)
		mesh->SetRelativeScale3D(FVector(1.0f, 1.0f, CropHeight));

	if (CropHeight >= 1.0f) {
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

	for (UStaticMeshComponent* mesh : GetCropMeshes())
		mesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.0f));

	StartTimer(Citizen);
}

TArray<UStaticMeshComponent*> AFarm::GetCropMeshes()
{
	TArray<UStaticMeshComponent*> components;
	GetComponents<UStaticMeshComponent>(components);

	components.Remove(BuildingMesh);

	return components;
}

void AFarm::StartTimer(ACitizen* Citizen)
{
	TArray<FTimerParameterStruct> params;
	Camera->TimerManager->SetParameter(Citizen, params);
	Camera->TimerManager->CreateTimer("Farm", this, GetTimeLength(Citizen) / 10.0f, "Production", params, false, true);
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

int32 AFarm::GetTimeLength(ACitizen* Citizen)
{
	int32 timeToCompleteDay = Camera->Grid->AtmosphereComponent->GetTimeToCompleteDay();
	int32 timeLength = TimeLengthHours * timeToCompleteDay / 24.0f / Citizen->GetProductivity();

	return timeLength;
}
