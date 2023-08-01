#include "Ground.h"

#include "Vegetation.h"

AGround::AGround()
{
	Fertility = 0;
}

void AGround::SetFertility(int32 Mean)
{
	int32 value = FMath::RandRange(-1, 1);

	Fertility = Mean + value;
}

int32 AGround::GetFertility()
{
	return Fertility;
}

void AGround::GenerateTree()
{
	FVector origin;
	FVector boxExtent;
	GetActorBounds(false, origin, boxExtent);

	TArray<int32> locListX;
	TArray<int32> locListY;
	for (int i = -45; i <= 45; i++) {
		locListX.Add(i);
		locListY.Add(i);
	}

	for (int i = 0; i < Trees.Num(); i++) {
		int32 xT = Trees[i]->GetActorLocation().X;
		int32 yT = Trees[i]->GetActorLocation().Y;

		for (int j = yT - 5; j <= yT + 5; j++) {
			if (locListX.Contains(j)) {
				locListX.Remove(j);
			}
		}

		for (int j = yT - 5; j <= yT + 5; j++) {
			if (locListY.Contains(j)) {
				locListY.Remove(j);
			}
		}
	}

	int32 indexX = FMath::RandRange(0, (locListX.Num() - 1));
	int32 indexY = FMath::RandRange(0, (locListY.Num() - 1));

	int32 x = locListX[indexX];
	int32 y = locListY[indexY];

	FVector location = FVector(GetActorLocation().X + x, GetActorLocation().Y + y, boxExtent.Z + origin.Z);

	AVegetation* tree = GetWorld()->SpawnActor<AVegetation>(Tree, location, GetActorRotation());

	Trees.Add(tree);
}

void AGround::DeleteTrees()
{
	for (int i = 0; i < Trees.Num(); i++) {
		Trees[i]->Destroy();
	}
}