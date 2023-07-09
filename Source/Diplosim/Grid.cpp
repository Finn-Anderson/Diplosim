#include "Grid.h"

#include "Math/UnrealMathUtility.h"

#include "Tile.h"

AGrid::AGrid()
{
	PrimaryActorTick.bCanEverTick = true;

	size = 10;

	Render();
}

void AGrid::BeginPlay()
{
	Super::BeginPlay();
}

void AGrid::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AGrid::Render() 
{
	TArray<ATile*> choices;
	ATile* Arr[] = { Water, Ground, Hill };

	TArray<TArray<ATile*>> storage;

	choices.Append(Arr, sizeof(Arr));

	FActorSpawnParameters SpawnInfo;

	for (int32 y = 0; y < size; y++) 
	{
		for (int32 x = 0; x < size; x++) 
		{
			int32 mean;
			ATile* choice;

			int32 low = 0;
			int32 high = 20;

			if (x == 0 && y == 0) {
				mean = FMath::RandRange(0, 5);
			}
			else {
				ATile* yTile = NULL;
				ATile* xTile = NULL;

				int32 fertility;
				int32 value = FMath::RandRange(-1, 1);

				if (y > 0) {
					yTile = storage[x][y - 1];
				}

				xTile = storage[x - 1][y];

				if (yTile != NULL) {
					int32 yFertility = yTile->GetFertility();
					int32 xFertility = xTile->GetFertility();

					fertility = (yFertility + xFertility) / 2;

					if (xTile == Water && yTile == Water) {
						high = 14;
					} 
					else if (xTile == Hill && yTile == Hill) {
						low = 5;
					}
				}
				else {
					fertility = xTile->GetFertility();

					if (xTile == Water) {
						high = 14;
					}
					else if (xTile == Hill) {
						low = 5;
					}
				}

				mean = FMath::Clamp(fertility + value, 0, 5);
			}

			int32 choiceVal = FMath::RandRange(low, high);
			choiceVal /= 10;

			choice = choices[choiceVal];

			// Spawn Actor
			FVector loc = FVector(GetActorLocation().X * x, GetActorLocation().Y * y, GetActorLocation().Z);

			ATile* tile = GetWorld()->SpawnActor<ATile>(choice->StaticClass(), loc, GetActorRotation(), SpawnInfo);

			tile->SetFertility(mean);

			storage[x][y] = choice;
		}
	}
}