#include "Grid.h"

#include "Math/UnrealMathUtility.h"
#include "Kismet/GameplayStatics.h"

#include "Tile.h"
#include "Camera.h"

AGrid::AGrid()
{
	PrimaryActorTick.bCanEverTick = true;

	Size = 100;
}

void AGrid::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Camera = PController->GetPawn<ACamera>();

	Render();
}

void AGrid::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AGrid::Render()
{
	TArray<TSubclassOf<ATile>> choices;
	TSubclassOf<ATile> Arr[] = { Water, Ground, Hill };

	choices.Append(Arr, sizeof(Arr));

	ATile* storage[100][100] = { };

	FActorSpawnParameters SpawnInfo;

	for (int32 y = 0; y < Size; y++)
	{
		for (int32 x = 0; x < Size; x++)
		{
			int32 mean = 3;
			TSubclassOf<ATile> choice = Ground;

			int32 low = 0;
			int32 high = 1000;


			if (x < (Size - (Size - 5)) || y < (Size - (Size - 5)) || x > (Size - 6) || y > (Size - 6)) {
				choice = Water;
			}
			else {
				if (x == 0 && y == 0) {
					mean = FMath::RandRange(0, 5);
				}
				else {
					ATile* yTile = nullptr;
					ATile* xTile = nullptr;

					int32 fertility = 3;
					int32 value = FMath::RandRange(-1, 1);

					if (y > 0) {
						yTile = storage[x][y - 1];
					}

					if (x > 0) {
						xTile = storage[x - 1][y];
					}
					if (yTile != nullptr) {
						if (xTile != nullptr) {
							int32 yFertility = yTile->GetFertility();
							int32 xFertility = xTile->GetFertility();

							fertility = (yFertility + xFertility) / 2;

							if (xTile->GetType() == EType::Water || yTile->GetType() == EType::Water) {
								high = 10;
							}
							else if (xTile->GetType() == EType::Hill || yTile->GetType() == EType::Hill) {
								low = 990;
							}
						}
						else {
							fertility = yTile->GetFertility();

							if (yTile->GetType() == EType::Water) {
								high = 10;
							}
							else if (yTile->GetType() == EType::Hill) {
								low = 990;
							}
						}
					}
					else {
						fertility = xTile->GetFertility();

						if (xTile->GetType() == EType::Water) {
							high = 10;
						}
						else if (xTile->GetType() == EType::Hill) {
							low = 990;
						}
					}

					mean = FMath::Clamp(fertility + value, 0, 5);

					int32 choiceVal = FMath::RandRange(low, high);

					if (choiceVal < 6) {
						choiceVal = 0;
					}
					else if (choiceVal > 994) {
						choiceVal = 2;
					}
					else {
						choiceVal = 1;
					}

					choice = choices[choiceVal];
				}
			}

			// Spawn Actor
			ATile* tile = GetWorld()->SpawnActor<ATile>(choice, SpawnInfo);

			FVector min;
			FVector max;
			tile->TileMesh->GetLocalBounds(min, max);

			FVector center = min - max;
			FVector loc = FVector(center.X * x - (center.X * (Size / 2)), center.Y * y - (center.Y * (Size / 2)), 0);

			tile->SetActorLocation(loc);

			tile->SetFertility(mean);

			storage[x][y] = tile;

			int32 boundsSize = (Size - 1);
			if ((x == boundsSize) && (y == boundsSize)) {
				FVector c2 = loc;
				
				tile = storage[0][0];

				FVector c1 = tile->GetActorLocation();

				Camera->SetBounds(c1, c2);
			}
		}
	}
}