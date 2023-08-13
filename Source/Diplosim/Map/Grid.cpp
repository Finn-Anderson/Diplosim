#include "Grid.h"

#include "Math/UnrealMathUtility.h"
#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

#include "Mineral.h"
#include "Vegetation.h"
#include "Player/Camera.h"
#include "Player/CameraMovementComponent.h"
#include "Player/BuildComponent.h"
#include "Buildings/Building.h"

AGrid::AGrid()
{
	PrimaryActorTick.bCanEverTick = false;

	HISMWater = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMWater"));
	HISMWater->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMWater->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);

	HISMGround = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMGround"));
	HISMGround->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMGround->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);

	HISMHill = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMHill"));
	HISMHill->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMHill->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);

	Size = 150;
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
	for (int32 y = 0; y < Size; y++)
	{
		for (int32 x = 0; x < Size; x++)
		{
			int32 mean = 3;
			FString choice = "Ground";

			int32 low = 0;
			int32 high = 1000;

			if (x < (Size - (Size - 20)) || y < (Size - (Size - 20)) || x > (Size - 21) || y > (Size - 21)) {
				choice = "Water";
			}
			else {
				int32 value = FMath::RandRange(-1, 1);

				FString checkX = "Ground";
				FString checkY = "Ground";

				FTileStruct xTile = Storage.Last();
				FTileStruct yTile = Storage[x + ((y - 1) * Size)];

				int32 fertility = (yTile.GetFertility() + xTile.GetFertility()) / 2;

				mean = FMath::Clamp(fertility + value, 0, 5);
				
				// Calculating tile type based on adjacant tiles
				if (xTile.Choice == "Water") {
					checkX = "Water";
				}
				else if (xTile.Choice == "Hill") {
					checkX = "Hill";
				}

				if (yTile.Choice == "Water") {
					checkY = "Water";
				}
				else if (yTile.Choice == "Hill") {
					checkY = "Hill";
				}

				int32 choiceVal = FMath::RandRange(low, high);

				int32 pass = 2;
				if (checkX == checkY && checkX != "Ground") {
					pass = 990;
				}
				else if (checkX != checkY) {
					pass = 500;
				}


				if (pass > choiceVal) {
					if (checkX == "Water" || checkY == "Water") {
						choice = "Water";
					}
					else {
						choice = "Hill";
					}
				}
			}

			// Spawn Actor
			GenerateTile(choice, mean, x, y);
		}
	}

	// Set Camera Bounds
	FTransform transform;

	FTileStruct min = Storage[10 + 10 * Size];
	if (min.Choice == "Water") {
		HISMWater->GetInstanceTransform(min.Instance, transform);
	}
	else if (min.Choice == "Ground") {
		HISMGround->GetInstanceTransform(min.Instance, transform);
	}
	else {
		HISMHill->GetInstanceTransform(min.Instance, transform);
	}
	FVector c1 = transform.GetLocation();

	FTileStruct max = Storage[139 + 139 * Size];
	if (max.Choice == "Water") {
		HISMWater->GetInstanceTransform(max.Instance, transform);
	}
	else if (max.Choice == "Ground") {
		HISMGround->GetInstanceTransform(max.Instance, transform);
	}
	else {
		HISMHill->GetInstanceTransform(max.Instance, transform);
	}
	FVector c2 = transform.GetLocation();

	Camera->MovementComponent->SetBounds(c2, c1);

	// Spawn resources
	for (int32 i = 0; i < Storage.Num(); i++) {
		GenerateResource(i);
	}
}

void AGrid::GenerateTile(FString Choice, int32 Mean, int32 x, int32 y)
{
	FTransform transform;
	FVector loc = FVector(100.0f * x - (100.0f * (Size / 2)), 100.0f * y - (100.0f * (Size / 2)), 0);
	transform.SetLocation(loc);

	int32 inst;
	if (Choice == "Water") {
		inst = HISMWater->AddInstance(transform);
	}
	else if (Choice == "Ground") {
		inst = HISMGround->AddInstance(transform);
	}
	else {
		inst = HISMHill->AddInstance(transform);
	}

	FTileStruct tile;
	tile.Choice = Choice;
	tile.Instance = inst;
	tile.Fertility = Mean;
	tile.Coords = x + (y * Size);

	Storage.Add(tile);
}

void AGrid::GenerateResource(int32 Pos)
{
	FTileStruct tile = Storage[Pos];

	int32 choiceVal = FMath::RandRange(1, 30);

	if (tile.Choice == "Hill") {
		if (choiceVal < 21)
			return;

		TSubclassOf<AResource> choice;
		if (choiceVal > 20) {
			choice = Rock; // Setup for different mineral types.
		}

		FTransform transform;
		HISMHill->GetInstanceTransform(tile.Instance, transform);
		FVector loc = transform.GetLocation();

		if (loc != FVector(0.0f, 0.0f, 0.0f)) {
			HISMHill->RemoveInstance(tile.Instance);

			AMineral* resource = GetWorld()->SpawnActor<AMineral>(choice, loc, GetActorRotation());

			resource->SetQuantity();

			resource->Grid = this;

			tile.Resource.Add(resource);

			Storage[Pos] = tile;
		}
	}
	else if (tile.Choice == "Ground") {
		FTileStruct xTile = Storage[tile.Coords - 1];
		FTileStruct yTile = Storage[tile.Coords - 150];

		if (xTile.Choice == "Hill" || yTile.Choice == "Hill")
			return;

		int32 trees = (xTile.Resource.Num() + yTile.Resource.Num()) / 2;

		int32 value = 0;

		if (choiceVal == 30) {
			value = 3;
		}
		else if (choiceVal > 24) {
			value = FMath::RandRange(0, 1);
		}

		int32 mean = FMath::Clamp(trees + value, 0, 5);

		FTransform transform;
		HISMGround->GetInstanceTransform(tile.Instance, transform);
		FVector loc = transform.GetLocation();

		for (int i = 0; i < mean; i++) {
			GenerateVegetation(Pos, loc);
		}
	}
	else {
		// Water resources (fish/oil);
	}
}

void AGrid::GenerateVegetation(int32 Pos, FVector Location)
{
	FTileStruct tile = Storage[Pos];

	int32 z = HISMGround->GetStaticMesh()->GetBounds().GetBox().GetSize().Z / 2;

	TArray<int32> locListX;
	TArray<int32> locListY;
	for (int i = -45; i <= 45; i++) {
		locListX.Add(i);
		locListY.Add(i);
	}

	for (int i = 0; i < tile.Resource.Num(); i++) {
		int32 xT = tile.Resource[i]->GetActorLocation().X;
		int32 yT = tile.Resource[i]->GetActorLocation().Y;

		for (int j = yT - 10; j <= yT + 10; j++) {
			if (locListX.Contains(j)) {
				locListX.Remove(j);
			}
		}

		for (int j = yT - 10; j <= yT + 10; j++) {
			if (locListY.Contains(j)) {
				locListY.Remove(j);
			}
		}
	}

	int32 indexX = FMath::RandRange(0, (locListX.Num() - 1));
	int32 indexY = FMath::RandRange(0, (locListY.Num() - 1));

	int32 x = locListX[indexX];
	int32 y = locListY[indexY];

	FVector location = FVector(Location.X + x, Location.Y + y, z);

	AVegetation* tree = GetWorld()->SpawnActor<AVegetation>(Tree, location, GetActorRotation());

	tile.Resource.Add(tree);

	Storage[Pos] = tile;
}

void AGrid::Clear()
{
	for (int32 i = 0; i < Storage.Num(); i++) {
		FTileStruct tile = Storage[i];
		for (int j = 0; j < tile.Resource.Num(); j++) {
			tile.Resource[j]->Destroy();
		}

		tile.Resource.Empty();
	}

	Storage.Empty();

	HISMWater->ClearInstances();
	HISMGround->ClearInstances();
	HISMHill->ClearInstances();

	Camera->BuildComponent->Building->bMoved = false;
}