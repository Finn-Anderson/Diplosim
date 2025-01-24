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
	CropMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CropMesh"));
	CropMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CropMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	CropMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.0f));

	HealthComponent->MaxHealth = 50;
	HealthComponent->Health = HealthComponent->MaxHealth;

	Yield = 5;

	TimeLength = 30.0f;
}

void AFarm::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	FTimerStruct timer;
	timer.CreateTimer("Farm", this, TimeLength / 10.0f, FTimerDelegate::CreateUObject(this, &AFarm::Production, Citizen), false);

	if (!Occupied.Contains(Citizen) || Camera->CitizenManager->Timers.Contains(timer))
		return;

	if (CropMesh->GetRelativeScale3D().Z == 1.0f)
		ProductionDone(Citizen);
	else
		StartTimer(Citizen);
}

void AFarm::Production(ACitizen* Citizen)
{
	CropMesh->SetRelativeScale3D(CropMesh->GetRelativeScale3D() + FVector(0.0f, 0.0f, 0.1f));

	if (CropMesh->GetRelativeScale3D().Z >= 1.0f) {
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
	Citizen->Carry(Camera->ResourceManager->GetResources(this)[0]->GetDefaultObject<AResource>(), GetYield(), this);

	StoreResource(Citizen);

	CropMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.0f));

	StartTimer(Citizen);
}

void AFarm::StartTimer(ACitizen* Citizen)
{
	FTimerStruct timer;
	timer.CreateTimer("Farm", this, TimeLength / 10.0f, FTimerDelegate::CreateUObject(this, &AFarm::Production, Citizen), false);

	Camera->CitizenManager->Timers.Add(timer);
}

int32 AFarm::GetFertility()
{
	auto bound = FMath::FloorToInt32(FMath::Sqrt((double)Camera->Grid->Size));

	int32 x = GetActorLocation().X / 100.0f + bound / 2;
	int32 y = GetActorLocation().Y / 100.0f + bound / 2;

	return Camera->Grid->Storage[x][y].Fertility;
}

int32 AFarm::GetYield()
{
	return Yield * GetFertility();
}