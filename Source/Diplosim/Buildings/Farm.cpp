#include "Buildings/Farm.h"

#include "Map/Vegetation.h"
#include "Map/Grid.h"
#include "Player/Camera.h"
#include "Player/ResourceManager.h"

AFarm::AFarm()
{
	TimeLength = 180.0f;

	Crop = nullptr;
}

void AFarm::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (Crop == nullptr) {
		int32 finalX = GetActorLocation().X;
		int32 finalY = GetActorLocation().Y;
		int32 finalZ = GetActorLocation().Z + BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize().Z;

		FVector pos = FVector(finalX, finalY, finalZ);

		Crop = GetWorld()->SpawnActor<AVegetation>(Camera->ResourceManagerComponent->GetResource(this), pos, GetActorRotation());

		Crop->ResourceMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.0f));

		Store(0, Citizen);
	}
}

void AFarm::Production(ACitizen* Citizen)
{
	GetWorldTimerManager().SetTimer(ProdTimer, FTimerDelegate::CreateUObject(this, &AFarm::ProductionDone, Citizen), TimeLength, true);
}

void AFarm::ProductionDone(ACitizen* Citizen)
{
	if (!(Crop->ResourceMesh->GetRelativeScale3D().Z >= 1.0f)) {
		FVector curHeight = Crop->ResourceMesh->GetRelativeScale3D();
		curHeight.Z += 0.1f;

		Crop->ResourceMesh->SetRelativeScale3D(curHeight);

		return;
	}
	else if (AtWork.Num() == Capacity) {
		GetWorldTimerManager().ClearTimer(ProdTimer);

		int32 yield = Crop->GetYield();

		TArray<FTileStruct> tileArr = Grid->GetDefaultObject<AGrid>()->Storage;
		for (int32 i = 0; i < tileArr.Num(); i++) {
			if (tileArr[i].Location == GetActorLocation()) {
				yield *= tileArr[i].Fertility;

				break;
			}
		}

		Crop->ResourceMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.0f));

		Store(yield, Citizen);
	}
}