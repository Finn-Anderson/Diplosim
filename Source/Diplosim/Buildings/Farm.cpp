#include "Buildings/Farm.h"

#include "Map/Vegetation.h"

AFarm::AFarm()
{
	TimeLength = 180.0f;
}

void AFarm::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	UStaticMeshComponent* cropMesh = Crop->GetDefaultObject<AVegetation>()->ResourceMesh;

	FVector bounds = BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize();
	FVector cropSize = cropMesh->GetStaticMesh()->GetBounds().GetBox().GetSize();

	int32 xQ = FMath::Floor(bounds.X / cropSize.X);
	int32 yQ = FMath::Floor(bounds.Y / cropSize.Y);

	if (yQ == 0 || xQ == 0)
		return;

	for (int32 i = 0; i < yQ; i++) {
		for (int32 j = 0; j < xQ; j++) {
			int32 x = bounds.X / xQ * i;
			int32 y = bounds.Y / yQ * j;

			int32 finalX = GetActorLocation().X - (bounds.X / 2) + x;
			int32 finalY = GetActorLocation().Y - (bounds.Y / 2) + y;
			int32 finalZ = bounds.Z / 2;

			FVector pos = FVector(finalX, finalY, finalZ);

			AVegetation* crop = GetWorld()->SpawnActor<AVegetation>(Crop, pos, GetActorRotation());

			crop->YieldStatus();

			CropList.Add(crop);
		}
	}
}

void AFarm::Production(ACitizen* Citizen)
{
	GetWorldTimerManager().SetTimer(ProdTimer, FTimerDelegate::CreateUObject(this, &AFarm::ProductionDone, Citizen), TimeLength, false);
}

void AFarm::ProductionDone(ACitizen* Citizen)
{
	if (Storage < StorageCap) {
		AResource* r = GetWorld()->SpawnActor<AResource>(Resource);

		Store(r->GetYield(), Citizen);

		r->Destroy();
	}
}