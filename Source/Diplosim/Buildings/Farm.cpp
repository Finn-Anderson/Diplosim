#include "Buildings/Farm.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"

#include "Map/Vegetation.h"
#include "Map/Grid.h"
#include "Player/Camera.h"
#include "Player/ResourceManager.h"

AFarm::AFarm()
{
	Crop = nullptr;
}

void AFarm::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (Occupied.Contains(Citizen)) {
		if (Crop == nullptr) {
			int32 finalX = GetActorLocation().X;
			int32 finalY = GetActorLocation().Y;
			int32 finalZ = GetActorLocation().Z + BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize().Z;

			FVector pos = FVector(finalX, finalY, finalZ);

			Crop = GetWorld()->SpawnActor<AVegetation>(Camera->ResourceManagerComponent->GetResource(this), pos, GetActorRotation());
			Crop->IntialScale = FVector(1.0f, 1.0f, 0.0f);
			Crop->YieldStatus();
			Crop->SetOwner(this);
		}
		else {
			Crop->IsChoppable();
		}
	}
}

void AFarm::ProductionDone()
{
	TArray<ACitizen*> workers = GetWorkers();

	if (workers.Num() > 0) {
		int32 yield = Crop->GetYield();

		FHitResult hit;

		FVector start = GetActorLocation();
		FVector end = GetActorLocation() - FVector(0.0f, 0.0f, 100.0f);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		if (GetWorld()->LineTraceSingleByChannel(hit, start, end, ECollisionChannel::ECC_Visibility, QueryParams)) {
			if (hit.GetActor()->IsA<AGrid>()) {
				AGrid* grid = Cast<AGrid>(hit.GetActor());

				int32 fertility = grid->HISMGround->PerInstanceSMCustomData[hit.Item * 4 + 3];

				yield *= fertility;
			}
		}

		Store(yield, workers[0]);
	}
}