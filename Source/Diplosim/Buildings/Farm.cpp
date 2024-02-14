#include "Buildings/Farm.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"

#include "Map/Grid.h"
#include "Player/Camera.h"
#include "Player/ResourceManager.h"
#include "Resource.h"

AFarm::AFarm()
{
	CropMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CropMesh"));
	CropMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CropMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	CropMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.0f));

	MinYield = 1;
	MaxYield = 5;

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
		TArray<ACitizen*> workers = GetWorkers();

		if (workers.Num() == 0)
			return;

		ProductionDone(workers[0]);
	}
	else
		GetWorldTimerManager().SetTimer(ProdTimer, FTimerDelegate::CreateUObject(this, &AFarm::Production, Citizen), TimeLength / 10.0f, false);
}

void AFarm::ProductionDone(ACitizen* Citizen)
{
	int32 yield = FMath::RandRange(MinYield, MaxYield);

	FHitResult hit;

	FVector start = GetActorLocation();
	FVector end = GetActorLocation() - FVector(0.0f, 0.0f, 100.0f);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(hit, start, end, ECollisionChannel::ECC_GameTraceChannel1, QueryParams)) {
		AGrid* grid = Cast<AGrid>(hit.GetActor());

		int32 fertility = grid->HISMGround->PerInstanceSMCustomData[hit.Item * 4 + 3];

		yield *= fertility;
	}

	Store(yield, Citizen);

	CropMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.0f));

	GetWorldTimerManager().SetTimer(ProdTimer, FTimerDelegate::CreateUObject(this, &AFarm::Production, Citizen), TimeLength / 10.0f, false);
}