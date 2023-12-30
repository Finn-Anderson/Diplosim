#include "Grid.h"

#include "Math/UnrealMathUtility.h"
#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

#include "Clouds.h"
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
	HISMWater->SetCanEverAffectNavigation(false);
	HISMWater->NumCustomDataFloats = 1;

	HISMBeach = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMBeach"));
	HISMBeach->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMBeach->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);

	HISMGround = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMGround"));
	HISMGround->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMGround->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);
	HISMGround->NumCustomDataFloats = 4;

	Size = 150;

	PercentageGround = 70;
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
	// Set tile struct to each storage slot based on size chosen
	for (int32 y = 0; y < Size; y++)
	{
		for (int32 x = 0; x < Size; x++)
		{
			FTileStruct tile;
			tile.X = x;
			tile.Y = y;

			Storage.Add(tile);
		}
	}

	TArray<FTileStruct> chooseableTiles = Storage;

	int32 PercentageWater = 100 - PercentageGround;

	float levelCount = 0.0f;

	int32 level = 5;

	// Set tile information based on adjacent tile types until all tile struct choices are set
	while (!chooseableTiles.IsEmpty()) {
		// Set current level
		if (levelCount <= 0.0f) {
			level--;

			if (level < 0) {
				levelCount = Storage.Num() * (PercentageWater / 100.0f);
			}
			else {
				levelCount = Storage.Num() * (PercentageGround / 100.0f);
			}

			if (level == 4) {
				levelCount *= 0.02f;
			} else if (level == 3) {
				levelCount *= 0.10f;
			}
			else if (level == 2) {
				levelCount *= 0.18f;
			}
			else if (level == 1) {
				levelCount *= 0.30f;
			}
			else if (level == 0) {
				levelCount *= 0.40f;
			}
			else if (level == -1) {
				levelCount *= 0.10f;
			}
			else if (level == -2) {
				levelCount *= 0.20f;
			}
			else if (level == -3) {
				levelCount *= 0.30f;
			}
			else {
				levelCount *= 0.40f;
			}
		}

		// Get Tile and adjacent tiles
		FTileStruct chosenTile = GetChosenTile(chooseableTiles, level);

		TArray<FTileStruct> tiles;

		if (chosenTile.X > 0) {
			tiles.Add(Storage[(chosenTile.X - 1) + (chosenTile.Y * Size)]);
		}
		if (chosenTile.Y > 0) {
			tiles.Add(Storage[chosenTile.X + ((chosenTile.Y - 1) * Size)]);
		}
		if (chosenTile.X < (Size - 1)) {
			tiles.Add(Storage[(chosenTile.X + 1) + (chosenTile.Y * Size)]);
		}
		if (chosenTile.Y < (Size - 1)) {
			tiles.Add(Storage[chosenTile.X + ((chosenTile.Y + 1) * Size)]);
		}

		// Set Level
		chosenTile.Level = level;

		// Set fertility
		if (chosenTile.Level < 0) {
			chosenTile.Fertility = 1;
		}
		else {
			int32 value = FMath::RandRange(-1, 1);

			int32 fTile = 0;
			int32 count = 0;
			for (FTileStruct t : tiles) {
				int32 f = t.Fertility;

				if (f > 0) {
					fTile += f;
					count++;
				}
			}

			if (count > 0) {
				fTile = (fTile + 1) / count;
			}

			chosenTile.Fertility = FMath::Clamp(fTile + value, 1, 5);
		}

		// Set Rotation
		int32 rand = FMath::RandRange(0, 3);

		chosenTile.Rotation = (FRotator(0.0f, 90.0f, 0.0f) * rand).Quaternion();
				
		// Set in storage for generation;
		Storage[chosenTile.X + (chosenTile.Y * Size)] = chosenTile;

		chooseableTiles.Remove(chosenTile);

		levelCount--;
	}

	for (FTileStruct tile : Storage) {
		FillGaps(tile);
	}

	// Generate Beaches
	SetBeaches();

	// Spawn Actor
	for (FTileStruct tile : Storage) {
		GenerateTile(tile.Level, tile.Fertility, tile.X, tile.Y, tile.Rotation);
	}

	// Set Camera Bounds
	FTileStruct min = Storage[10 + 10 * Size];
	FVector c1 = FVector(100.0f * min.X - (100.0f * (Size / 2)), 100.0f * min.Y - (100.0f * (Size / 2)), 0);

	FTileStruct max = Storage[(Size - 10) + (Size - 10) * Size];
	FVector c2 = FVector(100.0f * max.X - (100.0f * (Size / 2)), 100.0f * max.Y - (100.0f * (Size / 2)), 0);

	Camera->MovementComponent->SetBounds(c2, c1);

	// Spawn resources
	for (int32 i = 0; i < Storage.Num(); i++) {
		GenerateResource(i);
	}

	AClouds* clouds = GetWorld()->SpawnActor<AClouds>(CloudsClass, FVector(0.0f, 0.0f, 0.0f), FRotator(0.0f, 0.0f, 0.0f));
	clouds->GetCloudBounds(Size);
}

FTileStruct AGrid::GetChosenTile(TArray<FTileStruct> Tiles, int32 TargetLevel)
{
	TArray<FTileStruct> chosenList;

	for (FTileStruct tile : Tiles) {
		TArray<FTileStruct> tiles;

		if (tile.X > 0) {
			tiles.Add(Storage[(tile.X - 1) + (tile.Y * Size)]);
		}
		if (tile.Y > 0) {
			tiles.Add(Storage[tile.X + ((tile.Y - 1) * Size)]);
		}
		if (tile.X < (Size - 1)) {
			tiles.Add(Storage[(tile.X + 1) + (tile.Y * Size)]);
		}
		if (tile.Y < (Size - 1)) {
			tiles.Add(Storage[tile.X + ((tile.Y + 1) * Size)]);
		}

		for (FTileStruct t : tiles) {
			if (t.Level != TargetLevel && t.Level != (TargetLevel + 1))
				continue;

			chosenList.Add(tile);

			break;
		}
	}

	if (chosenList.IsEmpty())
		chosenList = Tiles;

	int32 chosenNum = FMath::RandRange(0, chosenList.Num() - 1);
	FTileStruct chosenTile = chosenList[chosenNum];

	return chosenTile;
}

void AGrid::FillGaps(FTileStruct Tile)
{
	TArray<int32> levels = {0, 0, 0, 0, 0, 0, 0, 0, 0};

	TArray<FTileStruct> tiles;

	if (Tile.X > 0) {
		tiles.Add(Storage[(Tile.X - 1) + (Tile.Y * Size)]);
	}
	if (Tile.Y > 0) {
		tiles.Add(Storage[Tile.X + ((Tile.Y - 1) * Size)]);
	}
	if (Tile.X < (Size - 1)) {
		tiles.Add(Storage[(Tile.X + 1) + (Tile.Y * Size)]);
	}
	if (Tile.Y < (Size - 1)) {
		tiles.Add(Storage[Tile.X + ((Tile.Y + 1) * Size)]);
	}

	for (FTileStruct t : tiles) {
		levels[t.Level + 4] += 1;
	}

	int32 index = -1;

	if (levels.Contains(4)) {
		levels.Find(4, index);
	}
	else if (levels.Contains(3)) {
		levels.Find(3, index);
	}

	if (index < 0)
		return;

	if (Tile.Level == (index - 4))
		return;

	Tile.Level = index - 4;

	Storage[Tile.X + (Tile.Y * Size)] = Tile;

	for (FTileStruct t : tiles) {
		FillGaps(t);
	}
}

void AGrid::SetBeaches()
{
	TArray<FTileStruct> chooseableTiles;

	for (FTileStruct tile : Storage) {
		if (tile.Level < 0)
			continue;

		TArray<FTileStruct> tiles;

		if (tile.X > 0) {
			tiles.Add(Storage[(tile.X - 1) + (tile.Y * Size)]);
		}
		if (tile.Y > 0) {
			tiles.Add(Storage[tile.X + ((tile.Y - 1) * Size)]);
		}
		if (tile.X < (Size - 1)) {
			tiles.Add(Storage[(tile.X + 1) + (tile.Y * Size)]);
		}
		if (tile.Y < (Size - 1)) {
			tiles.Add(Storage[tile.X + ((tile.Y + 1) * Size)]);
		}

		if (tiles.Num() < 4)
			continue;

		for (int32 i = 0; i < 2; i++) {
			if ((tiles[i * 2].Level >= 0 && tiles[i * 2 + 1].Level >= 0) || (tiles[i * 2].Level < 0 && tiles[i * 2 + 1].Level < 0))
				continue;

			chooseableTiles.Add(tile);
		}
	}

	for (int32 j = 0; j < 3; j++) {
		int32 chosenNum = FMath::RandRange(0, chooseableTiles.Num() - 1);
		FTileStruct chosenTile = chooseableTiles[chosenNum];

		for (int32 y = -8; y < 9; y++)
		{
			for (int32 x = -8; x < 9; x++)
			{
				if (chosenTile.X + x < 0 || chosenTile.X + x >= Size || chosenTile.Y + y < 0 || chosenTile.Y + y >= Size)
					continue;

				FTileStruct tile = Storage[(chosenTile.X + x) + ((chosenTile.Y + y) * Size)];
				
				if (!chooseableTiles.Contains(tile))
					continue;

				tile.Level = 100;

				TArray<FTileStruct> tiles;

				if (tile.X > 0) {
					tiles.Add(Storage[(tile.X - 1) + (tile.Y * Size)]);
				}
				if (tile.X < (Size - 1)) {
					tiles.Add(Storage[(tile.X + 1) + (tile.Y * Size)]);
				}
				if (tile.Y > 0) {
					tiles.Add(Storage[tile.X + ((tile.Y - 1) * Size)]);
				}
				if (tile.Y < (Size - 1)) {
					tiles.Add(Storage[tile.X + ((tile.Y + 1) * Size)]);
				}

				if (tiles.Num() < 4)
					continue;

				FVector groundLoc = FVector(0.0f, 0.0f, 0.0f);
				FVector waterLoc = FVector(0.0f, 0.0f, 0.0f);

				for (int32 i = 0; i < 2; i++) {
					if ((tiles[i * 2].Level >= 0 && tiles[i * 2 + 1].Level >= 0) || (tiles[i * 2].Level < 0 && tiles[i * 2 + 1].Level < 0) || tiles[i * 2].Level == 100 || tiles[i * 2 + 1].Level == 100)
						continue;

					if (tiles[i * 2].Level < 0) {
						waterLoc = FVector(100.0f * tiles[i * 2].X - (100.0f * (Size / 2)), 100.0f * tiles[i * 2].Y - (100.0f * (Size / 2)), 0);
						groundLoc = FVector(100.0f * tiles[i * 2 + 1].X - (100.0f * (Size / 2)), 100.0f * tiles[i * 2 + 1].Y - (100.0f * (Size / 2)), 0);
					}
					else {
						groundLoc = FVector(100.0f * tiles[i * 2].X - (100.0f * (Size / 2)), 100.0f * tiles[i * 2].Y - (100.0f * (Size / 2)), 0);
						waterLoc = FVector(100.0f * tiles[i * 2 + 1].X - (100.0f * (Size / 2)), 100.0f * tiles[i * 2 + 1].Y - (100.0f * (Size / 2)), 0);
					}

					break;
				}

				if (groundLoc.IsZero() || waterLoc.IsZero())
					continue;

				tile.Fertility = 1;

				FRotator direction = (groundLoc - waterLoc).Rotation();
				int32 remainder = (int32)direction.Yaw % 90;
				direction.Yaw -= remainder;

				tile.Rotation = (direction + FRotator(0.0f, 90.0f, 0.0f)).Quaternion();

				Storage[tile.X + (tile.Y * Size)] = tile;

				chooseableTiles.Remove(tile);
			}
		}
	}

	for (FTileStruct tile : Storage) {
		if (tile.Level != 100)
			continue;

		TArray<FTileStruct> tiles;

		if (tile.X > 0) {
			tiles.Add(Storage[(tile.X - 1) + (tile.Y * Size)]);
		}
		if (tile.X < (Size - 1)) {
			tiles.Add(Storage[(tile.X + 1) + (tile.Y * Size)]);
		}
		if (tile.Y > 0) {
			tiles.Add(Storage[tile.X + ((tile.Y - 1) * Size)]);
		}
		if (tile.Y < (Size - 1)) {
			tiles.Add(Storage[tile.X + ((tile.Y + 1) * Size)]);
		}

		for (FTileStruct t : tiles) {
			if (t.Level != 100 || t.Rotation == tile.Rotation)
				continue;

			tile.Level = 0;

			Storage[tile.X + (tile.Y * Size)] = tile;

			break;
		}
	}
}

void AGrid::GenerateTile(int32 Level, int32 Fertility, int32 x, int32 y, FQuat Rotation)
{
	FTransform transform;
	FVector loc = FVector(100.0f * x - (100.0f * (Size / 2)), 100.0f * y - (100.0f * (Size / 2)), 0);
	transform.SetLocation(loc);
	transform.SetRotation(Rotation);

	int32 inst;
	if (Level < 0.0f) {
		inst = HISMWater->AddInstance(transform);

		float data = -Level;

		HISMWater->SetCustomDataValue(inst, 0, data);
	}
	else if (Level > 4.0f) {
		inst = HISMBeach->AddInstance(transform);
	}
	else {
		transform.SetLocation(loc + FVector(0.0f, 0.0f, 50.0f * Level));

		float r = 0.0f;
		float g = 0.0f;
		float b = 0.0f;

		if (Fertility == 1) {
			r = 255.0f;
			g = 225.0f;
			b = 45.0f;
		} 
		else if (Fertility == 2) {
			r = 152.0f;
			g = 191.0f;
			b = 100.0f;
		}
		else if (Fertility == 3) {
			r = 86.0f;
			g = 228.0f;
			b = 68.0f;
		}
		else if (Fertility == 4) {
			r = 52.0f;
			g = 213.0f;
			b = 31.0f;
		}
		else {
			r = 36.0f;
			g = 146.0f;
			b = 21.0f;
		}

		r /= 255.0f;
		g /= 255.0f;
		b /= 255.0f;

		inst = HISMGround->AddInstance(transform);
		HISMGround->SetCustomDataValue(inst, 0, r);
		HISMGround->SetCustomDataValue(inst, 1, g);
		HISMGround->SetCustomDataValue(inst, 2, b);
		HISMGround->SetCustomDataValue(inst, 3, Fertility);
	}

	Storage[x + (y * Size)].Instance = inst;
}

void AGrid::GenerateResource(int32 Pos)
{
	FTileStruct tile = Storage[Pos];

	if (tile.Level < 0 || tile.Level > 4 || Pos - 1 < 0 || Pos - Size < 0)
		return;

	int32 choiceVal = FMath::RandRange(1, 100);

	FTransform transform;
	HISMGround->GetInstanceTransform(tile.Instance, transform);
	FVector loc = transform.GetLocation();

	int32 z = HISMGround->GetStaticMesh()->GetBounds().GetBox().GetSize().Z + 50.0f * tile.Level;
	loc.Z = z;

	if (choiceVal == 1) {
		SpawnResource(Pos, loc, Rock);
	}
	else {
		FTileStruct xTile = Storage[Pos - 1];
		FTileStruct yTile = Storage[Pos - Size];

		int32 trees = (xTile.Resource.Num() + yTile.Resource.Num()) / 2;

		int32 value = 0;

		if (choiceVal == 100) {
			value = 3;
		}
		else if (choiceVal > 80) {
			value = FMath::RandRange(0, 1);
		}

		int32 mean = FMath::Clamp(trees + value, 0, 5);

		TArray<int32> locListX;
		TArray<int32> locListY;
		for (int32 i = -40; i <= 40; i++) {
			locListX.Add(i);
			locListY.Add(i);
		}

		for (int32 i = 0; i < mean; i++) {
			int32 indexX = FMath::RandRange(0, (locListX.Num() - 1));
			int32 indexY = FMath::RandRange(0, (locListY.Num() - 1));

			int32 x = locListX[indexX];
			int32 y = locListY[indexY];

			loc.X += x;
			loc.Y += y;

			SpawnResource(Pos, loc, Tree);

			for (int32 j = x - 10; j <= x + 10; j++) {
				if (locListX.Contains(j)) {
					locListX.Remove(j);
				}
			}

			for (int32 j = y - 10; j <= y + 10; j++) {
				if (locListY.Contains(j)) {
					locListY.Remove(j);
				}
			}
		}
	}
}

void AGrid::SpawnResource(int32 Pos, FVector Location, TSubclassOf<AResource> ResourceClass)
{
	FTileStruct tile = Storage[Pos];

	AResource* resource = GetWorld()->SpawnActor<AResource>(ResourceClass, Location, GetActorRotation());

	tile.Resource.Add(resource);

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

	Camera->BuildComponent->Building->bMoved = false;

	HISMWater->ClearInstances();
	HISMBeach->ClearInstances();
	HISMGround->ClearInstances();
}