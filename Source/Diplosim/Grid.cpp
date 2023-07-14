#include "Grid.h"

#include "Math/UnrealMathUtility.h"
#include "Kismet/GameplayStatics.h"
#include "Components/InstancedStaticMeshComponent.h"

#include "Tile.h"
#include "Resource.h"
#include "Camera.h"
#include "CameraMovementComponent.h"

AGrid::AGrid()
{
	PrimaryActorTick.bCanEverTick = false;

	WaterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WaterMesh"));
	WaterMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
	WaterMesh->bCastDynamicShadow = true;
	WaterMesh->CastShadow = true;

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
	WaterMesh->SetWorldScale3D(FVector(Size, Size, Size));

	TArray<TSubclassOf<ATile>> choices;
	TSubclassOf<ATile> Arr[] = { Water, Ground, Hill };

	choices.Append(Arr, sizeof(Arr));

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
					yTile = Cast<ATile>(Storage[x][y - 1]);
				}

				if (x > 0) {
					xTile = Cast<ATile>(Storage[x - 1][y]);
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
			GenerateTile(choice, mean, x, y);
		}
	}

	// Set Camera Bounds
	AActor* min = Storage[10][10];
	FVector c1 = min->GetActorLocation();

	AActor* max = Storage[139][139];
	FVector c2 = max->GetActorLocation();

	Camera->MovementComponent->SetBounds(c1, c2);

	GenerateResources();
}

void AGrid::GenerateTile(TSubclassOf<class ATile> Choice, int32 Mean, int32 x, int32 y)
{
	ATile* tile = GetWorld()->SpawnActor<ATile>(Choice);

	FVector min;
	FVector max;
	tile->TileMesh->GetLocalBounds(min, max);

	FVector center = min - max;
	FVector loc = FVector(center.X * x - (center.X * (Size / 2)), center.Y * y - (center.Y * (Size / 2)), 0);

	tile->SetActorLocation(loc);

	tile->SetFertility(Mean);

	tile->TileMesh->SetMobility(EComponentMobility::Static);

	Storage[x][y] = tile;
}

void AGrid::GenerateResources()
{
	for (int32 y = 0; y < Size; y++) {
		for (int32 x = 0; x < Size; x++) {
			ATile* tile = Cast<ATile>(Storage[x][y]);

			if (tile->GetType() == EType::Hill) {
				int32 choiceVal = FMath::RandRange(1, 20);

				TSubclassOf<AResource> choice;
				if (choiceVal > 10) {
					choice = Rock;
				}

				AResource* resource = GetWorld()->SpawnActor<AResource>(choice);

				FVector loc = tile->GetActorLocation();

				resource->SetActorLocation(loc);

				resource->ResourceMesh->SetMobility(EComponentMobility::Static);

				resource->Grid = this;

				tile->Destroy();

				Storage[x][y] = resource;
			}
			else if (tile->GetType() == EType::Ground) {
				int32 value = FMath::RandRange(-1, 1);
				int32 xTrees = Cast<ATile>(Storage[x - 1][y])->Trees;
				int32 yTrees = Cast<ATile>(Storage[x][y - 1])->Trees;

				int32 trees = (xTrees + yTrees) / 2;
				int32 mean = FMath::Clamp(trees + value, 0, 5);

				TArray<int32> xPrev;
				TArray<int32> yPrev;
				
				for (int i = 0; i < mean; i++) {
					FVector origin;
					FVector boxExtent;
					tile->GetActorBounds(false, origin, boxExtent);

					int32 xRand;
					int32 yRand;

					bool passed = false;

					if (xPrev.Num() > 0) {
						while (!passed) {
							xRand = FMath::RandRange(-45, 45);
							yRand = FMath::RandRange(-45, 45);

							for (int j = 0; j < xPrev.Num(); j++) {
								if (!((xRand < (xPrev[j] + 10) && xRand > (xPrev[j] - 10)) || (yRand < (yPrev[j] + 10) && yRand > (yPrev[j] - 10)))) {
									passed = true;
								}
							}
						}
					}
					else {
						xRand = FMath::RandRange(-45, 45);
						yRand = FMath::RandRange(-45, 45);
					}

					xPrev.Add(xRand);
					yPrev.Add(yRand);

					FVector location = FVector(tile->GetActorLocation().X + xRand, tile->GetActorLocation().Y + yRand, boxExtent.Z + origin.Z);

					GetWorld()->SpawnActor<AResource>(Tree, location, GetActorRotation());
				}

				tile->Trees = mean;
			}
			else {
				// Water resources (fish/oil);
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
			AActor* tile = Storage[x][y];

			tile->Destroy();
		}
	}
}