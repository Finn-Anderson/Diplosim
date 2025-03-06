#include "Farm.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"

#include "Map/Grid.h"
#include "AI/Citizen.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
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

	FTimerStruct timer;
	timer.CreateTimer("Farm", this, TimeLength / 10.0f / Citizen->GetProductivity(), FTimerDelegate::CreateUObject(this, &AFarm::Production, Citizen), false, true);

	if (!Occupied.Contains(Citizen) || Camera->CitizenManager->Timers.Contains(timer))
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

		ProductionDone(workers[0]);
	}
	else
		StartTimer(Citizen);
}

void AFarm::ProductionDone(ACitizen* Citizen)
{
	Citizen->Carry(Crop->GetDefaultObject<AResource>(), GetYield(), this);

	StoreResource(Citizen);

	for (UStaticMeshComponent* mesh : CropMeshes)
		mesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.0f));

	StartTimer(Citizen);
}

void AFarm::StartTimer(ACitizen* Citizen)
{
	FTimerStruct timer;
	timer.CreateTimer("Farm", this, TimeLength / 10.0f / Citizen->GetProductivity(), FTimerDelegate::CreateUObject(this, &AFarm::Production, Citizen), false, true);

	Camera->CitizenManager->Timers.Add(timer);
}

int32 AFarm::GetFertility()
{
	int32 fertility = 5;
	
	if (bAffectedByFerility) {
		auto bound = FMath::FloorToInt32(FMath::Sqrt((double)Camera->Grid->Size));

		int32 x = GetActorLocation().X / 100.0f + bound / 2;
		int32 y = GetActorLocation().Y / 100.0f + bound / 2;

		fertility = Camera->Grid->Storage[x][y].Fertility;
	}

	return fertility;
}

int32 AFarm::GetYield()
{
	return Yield * GetFertility();
}