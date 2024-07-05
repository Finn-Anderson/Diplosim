#include "Farm.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"

#include "Map/Grid.h"
#include "AI/Citizen.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"

AFarm::AFarm()
{
	CropMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CropMesh"));
	CropMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CropMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	CropMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.0f));

	Yield = 5;

	TimeLength = 30.0f;
}

void AFarm::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (!Occupied.Contains(Citizen) || GetWorldTimerManager().IsTimerActive(ProdTimer))
		return;

	if (CropMesh->GetRelativeScale3D().Z == 1.0f)
		ProductionDone(Citizen);
	else
		GetWorldTimerManager().SetTimer(ProdTimer, FTimerDelegate::CreateUObject(this, &AFarm::Production, Citizen), TimeLength / 10.0f, false);
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
		GetWorldTimerManager().SetTimer(ProdTimer, FTimerDelegate::CreateUObject(this, &AFarm::Production, Citizen), TimeLength / 10.0f, false);
}

void AFarm::ProductionDone(ACitizen* Citizen)
{
	auto bound = FMath::FloorToInt32(FMath::Sqrt((double)Camera->Grid->Size));

	int32 x = GetActorLocation().X / 100.0f + bound / 2;
	int32 y = GetActorLocation().Y / 100.0f + bound / 2;

	int32 fertility = Camera->Grid->Storage[x][y].Fertility;

	Citizen->Carry(Camera->ResourceManagerComponent->GetResource(this)->GetDefaultObject<AResource>(), Yield * fertility, this);

	StoreResource(Citizen);

	CropMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.0f));

	GetWorldTimerManager().SetTimer(ProdTimer, FTimerDelegate::CreateUObject(this, &AFarm::Production, Citizen), TimeLength / 10.0f, false);
}