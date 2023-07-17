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
				int32 value = FMath::RandRange(-1, 1);

				TSubclassOf<ATile> checkX = Ground;
				TSubclassOf<ATile> checkY = Ground;

				ATile* xTile = Cast<ATile>(Storage[x - 1][y]);
				ATile* yTile = Cast<ATile>(Storage[x][y - 1]);

				int32 xFertility;
				int32 yFertility;
				if (xTile->GetClass() == Ground) {
					xFertility = xTile->GetFertility();
				}
				else {
					xFertility = FMath::RandRange(0, 3);
				}

				if (yTile->GetClass() == Ground) {
					yFertility = yTile->GetFertility();
				}
				else {
					yFertility = FMath::RandRange(0, 3);
				}

				int32 fertility = (yFertility + xFertility) / 2;

				if (xTile->GetClass() == Water) {
					checkX = Water;
				}
				else if (xTile->GetClass() == Hill) {
					checkX = Hill;
				}

				if (yTile->GetClass() == Water) {
					checkY = Water;
				}
				else if (yTile->GetClass() == Hill) {
					checkY= Hill;
				}

				mean = FMath::Clamp(fertility + value, 0, 5);

				int32 choiceVal = FMath::RandRange(low, high);

				int32 pass = 2;
				if (checkX == checkY && checkX != Ground) {
					pass = 990;
				}
				else if (checkX != checkY) {
					pass = 500;
				}


				if (pass > choiceVal) {
					if (checkX == Water || checkY == Water) {
						choice = Water;
					}
					else {
						choice = Hill;
					}
				}
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

	for (int32 y = 0; y < Size; y++)
	{
		for (int32 x = 0; x < Size; x++)
		{
			ATile* tile = Cast<ATile>(Storage[x][y]);
			GenerateResource(tile, x, y);
		}
	}
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

	tile->TileMesh->SetMobility(EComponentMobility::Static);

	/*if (tile->IsA<AGround>()) {
		AGrid* t = Cast<AGround>(tile);

		t->SetFertility(Mean);
	}*/

	Storage[x][y] = tile;
}

void AGrid::GenerateResource(ATile* tile, int32 x, int32 y)
{
	int32 choiceVal = FMath::RandRange(1, 30);

	if (tile->GetClass() == Hill) {
		TSubclassOf<AResource> choice = nullptr;
		if (choiceVal > 20) {
			choice = Rock;
		}

		if (choice != nullptr) {
			FVector loc = tile->GetActorLocation();

			AResource* resource = GetWorld()->SpawnActor<AResource>(choice, loc, GetActorRotation());

			resource->Grid = this;

			Storage[x][y] = resource;

			tile->Destroy();
		}
	}
	else if (tile->GetClass() == Ground) {
		ATile* xTile = Cast<ATile>(Storage[x - 1][y]);
		ATile* yTile = Cast<ATile>(Storage[x][y - 1]);

		if (xTile != nullptr && xTile->GetClass() == Ground && yTile != nullptr && yTile->GetClass() == Ground) {
			int32 xTrees = xTile->Trees.Num();
			int32 yTrees = yTile->Trees.Num();

			int32 value = FMath::RandRange(-1, 1);

			int32 trees = ((xTrees + yTrees) + 1) / 2;
			int32 mean = FMath::Clamp(trees + value, 0, 5);

			if (choiceVal > 27 || mean > 1) {
				for (int i = 0; i < mean; i++) {
					tile->GenerateTree();
				}
			}
		}
				
	}
	else {
		// Water resources (fish/oil);
	}
}

void AGrid::Clear()
{
	for (int32 y = 0; y < Size; y++)
	{
		for (int32 x = 0; x < Size; x++)
		{
			AActor* tile = Storage[x][y];

			if (tile->IsA<ATile>()) {
				ATile* rList = Cast<ATile>(tile);

				for (int i = 0; i < rList->Trees.Num(); i++) {
					rList->Trees[i]->Destroy();
				}
			}

			tile->Destroy();
		}
	}
}