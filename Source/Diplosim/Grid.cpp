#include "Grid.h"

#include "Math/UnrealMathUtility.h"
#include "Kismet/GameplayStatics.h"
#include "Components/InstancedStaticMeshComponent.h"

#include "Tile.h"
#include "Camera.h"
#include "CameraMovementComponent.h"

AGrid::AGrid()
{
	PrimaryActorTick.bCanEverTick = false;

	Size = 150;

	Storage[150][150] = { };
}

void AGrid::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Camera = PController->GetPawn<ACamera>();

	Camera->Grid = this;

	Render();
}

void AGrid::Render()
{
	TArray<TSubclassOf<ATile>> choices;
	TSubclassOf<ATile> Arr[] = { Water, Ground, Hill };

	choices.Append(Arr, sizeof(Arr));

	FActorSpawnParameters SpawnInfo;

	for (int32 y = 0; y < Size; y++)
	{
		for (int32 x = 0; x < Size; x++)
		{
			int32 mean = 3;
			TSubclassOf<ATile> choice = Ground;

			int32 low = 0;
			int32 high = 1000;

			if (x < (Size - (Size - 20)) || y < (Size - (Size - 20)) || x > (Size - 21) || y > (Size - 21)) {
				choice = Water;
			}
			else {
				ATile* yTile = nullptr;
				ATile* xTile = nullptr;

				int32 fertility = 3;
				int32 value = FMath::RandRange(-1, 1);

				bool checkWater = false;
				bool checkHill = false;

				bool mandoHill = false;
				bool mandoWater = false;

				if (y > 0) {
					yTile = Storage[x][y - 1];
				}

				if (x > 0) {
					xTile = Storage[x - 1][y];
				}

				if (yTile != nullptr) {
					if (xTile != nullptr) {
						int32 yFertility = yTile->GetFertility();
						int32 xFertility = xTile->GetFertility();

						fertility = (yFertility + xFertility) / 2;

						if (!(xTile->GetType() == EType::Water && yTile->GetType() == EType::Hill) || !(xTile->GetType() == EType::Hill && yTile->GetType() == EType::Water)) {
							if (xTile->GetType() == EType::Water || yTile->GetType() == EType::Water) {
								if (xTile->GetType() == EType::Water && yTile->GetType() == EType::Water) {
									mandoWater = true;
								}
								else {
									checkWater = true;
								}
							}
							else if (xTile->GetType() == EType::Hill || yTile->GetType() == EType::Hill) {
								if (xTile->GetType() == EType::Hill && yTile->GetType() == EType::Hill) {
									mandoHill = true;
								}
								else {
									checkHill = true;
								}
							}
						}
					}
					else {
						fertility = yTile->GetFertility();

						if (yTile->GetType() == EType::Water) {
							checkWater = true;
						}
						else if (yTile->GetType() == EType::Hill) {
							checkHill = true;
						}
					}
				}
				else {
					fertility = xTile->GetFertility();

					if (xTile->GetType() == EType::Water) {
						checkWater = true;
					}
					else if (xTile->GetType() == EType::Hill) {
						checkHill = true;
					}
				}

				mean = FMath::Clamp(fertility + value, 0, 5);

				int32 choiceVal = FMath::RandRange(low, high);

				if (mandoHill || mandoWater) {
					if (mandoWater) {
						if (choiceVal < 990) {
							choiceVal = 0;
						}
						else {
							choiceVal = 1;
						}
					}
					else {
						choiceVal = 2;
					}
				}
				else {
					if (checkWater || checkHill) {
						int32 pass = 500;

						if (choiceVal < pass) {
							if (checkWater) {
								choiceVal = 0;
							}
							else {
								choiceVal = 2;
							}
						}
						else {
							choiceVal = 1;
						}
					}
					else {
						if (choiceVal < 2) {
							choiceVal = 0;
						}
						else if (choiceVal > 998) {
							choiceVal = 2;
						}
						else {
							choiceVal = 1;
						}
					}
				}

				choice = choices[choiceVal];
			}

			// Spawn Actor
			ATile* tile = GetWorld()->SpawnActor<ATile>(choice, SpawnInfo);

			FVector min;
			FVector max;
			tile->ISMComponent->GetLocalBounds(min, max);

			FVector center = min - max;
			FVector loc = FVector(center.X * x - (center.X * (Size / 2)), center.Y * y - (center.Y * (Size / 2)), 0);

			tile->SetActorLocation(loc);

			tile->SetFertility(mean);

			tile->ISMComponent->SetMobility(EComponentMobility::Static);

			Storage[x][y] = tile;

			int32 boundsSize = (Size - 1);
			if ((x == boundsSize) && (y == boundsSize)) {
				FVector c2 = loc;
				
				tile = Storage[0][0];

				FVector c1 = tile->GetActorLocation();

				Camera->MovementComponent->SetBounds(c1, c2);
			}
		}
	}
}

void AGrid::Clear()
{
	for (int32 y = 0; y < Size; y++)
	{
		for (int32 x = 0; x < Size; x++)
		{
			ATile* tile = Storage[x][y];

			tile->Destroy();
		}
	}
}