#include "Grid.h"

#include "Math/UnrealMathUtility.h"
#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Blueprint/UserWidget.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/DecalComponent.h"

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
	HISMWater->bReceivesDecals = false;

	HISMLava = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMLava"));
	HISMLava->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMLava->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);

	HISMGround = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMGround"));
	HISMGround->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMGround->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);
	HISMGround->NumCustomDataFloats = 4;

	Size = 22500;

	PercentageGround = 50;
}

void AGrid::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Camera = PController->GetPawn<ACamera>();

	MapUIInstance = CreateWidget<UUserWidget>(PController, MapUI);
	MapUIInstance->AddToViewport();

	Camera->Grid = this;

	Render();
}

void AGrid::Render()
{
	int32 tileTally = 0;

	auto bound = FMath::FloorToInt32(FMath::Sqrt((double)Size));

	for (int32 x = 0; x < bound; x++) {
		auto &row = Storage.Emplace_GetRef();

		for (int32 y = 0; y < bound; y++) {
			FTileStruct tile;

			tile.X = x - (bound / 2);
			tile.Y = y - (bound / 2);

			row.Add(tile);
		}
	}

	TArray<FTileStruct*> chooseableTiles = {&Storage[bound / 2][bound / 2]};

	int32 PercentageWater = 100 - PercentageGround;

	float levelCount = 0.0f;

	int32 level = 8;

	// Set tile information based on adjacent tile types until all tile struct choices are set
	while (tileTally < Size) {
		// Set current level
		if (levelCount <= 0.0f) {
			level--;

			if (level < 0) {
				levelCount = Size * (PercentageWater / 100.0f);
			}
			else {
				levelCount = Size * (PercentageGround / 100.0f);
			}
			
			if (level == 7) {
				levelCount *= 0.015f;
			}
			else if (level == 6) {
				levelCount *= 0.015f;
			} 
			else if (level == 5) {
				levelCount *= 0.03f;
			} 
			else if (level == 4) {
				levelCount *= 0.04f;
			} 
			else if (level == 3) {
				levelCount *= 0.10f;
			}
			else if (level == 2) {
				levelCount *= 0.15f;
			}
			else if (level == 1) {
				levelCount *= 0.25f;
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
		int32 chosenNum = FMath::RandRange(0, chooseableTiles.Num() - 1);
		FTileStruct* chosenTile = chooseableTiles[chosenNum];

		// Set level
		chosenTile->Level = level;

		// Set fertility if water
		if (level < 0 || level > 6)
			chosenTile->Fertility = 1;

		// Set adjacent tile information
		TMap<FString, FTileStruct*> map;

		if (chosenTile->X - 1 + bound / 2 > 0)
			map.Add("Left", &Storage[chosenTile->X - 1 + bound / 2][chosenTile->Y + bound / 2]);

		if (chosenTile->X + 1 + bound / 2 < bound)
			map.Add("Right", &Storage[chosenTile->X + 1 + bound / 2][chosenTile->Y + bound / 2]);

		if (chosenTile->Y - 1 + bound / 2 > 0)
			map.Add("Below", &Storage[chosenTile->X + bound / 2][chosenTile->Y - 1 + bound / 2]);

		if (chosenTile->Y + 1 + bound / 2 < bound)
			map.Add("Above", &Storage[chosenTile->X + bound / 2][chosenTile->Y + 1 + bound / 2]);
		

		for (auto& element : map) {
			FTileStruct* t = element.Value;

			FString oppositeKey;
			if (element.Key == "Left") {
				oppositeKey = "Right";
			}
			else if (element.Key == "Right") {
				oppositeKey = "Left";
			}
			else if (element.Key == "Below") {
				oppositeKey = "Above";
			}
			else {
				oppositeKey = "Below";
			}

			t->AdjacentTiles.Add(oppositeKey, chosenTile);
			chosenTile->AdjacentTiles.Add(element.Key, t);

			if (!chooseableTiles.Contains(t) && t->Level == -100)
				chooseableTiles.Add(t);
		}

		chooseableTiles.Remove(chosenTile);

		levelCount--;

		tileTally++;

		if (chooseableTiles.IsEmpty())
			break;
	}

	for (TArray<FTileStruct> &row : Storage) {
		for (FTileStruct &tile : row) {
			FillGaps(&tile);
		}
	}

	// Spawn Actor
	for (TArray<FTileStruct> &row : Storage) {
		for (FTileStruct &tile : row) {
			GenerateTile(&tile);
		}
	}

	// Set Camera Bounds
	FVector c1 = FVector(bound * 100, bound * 100, 0);
	FVector c2 = FVector(-bound * 100, -bound * 100, 0);

	Camera->MovementComponent->SetBounds(c1, c2);

	// Spawn resources
	for (TArray<FTileStruct>& row : Storage) {
		for (FTileStruct& tile : row) {
			GenerateResource(&tile);
		}
	}

	Clouds = GetWorld()->SpawnActor<AClouds>(CloudsClass, FVector(0.0f, 0.0f, 0.0f), FRotator(0.0f, 0.0f, 0.0f));
	Clouds->GetCloudBounds(bound * 100 / 2);
}

void AGrid::FillGaps(FTileStruct* Tile)
{
	if (Tile->Level == -100)
		return;

	// Set level
	float tally = Tile->Level;
	int32 count = 1;

	for (auto& element : Tile->AdjacentTiles) {
		FTileStruct* t = element.Value;

		tally += t->Level;

		count++;
	}

	tally /= count;
	
	Tile->Level = FMath::RoundHalfFromZero(tally);

	// Set Rotation
	int32 rand = FMath::RandRange(0, 3);

	Tile->Rotation = (FRotator(0.0f, 90.0f, 0.0f) * rand).Quaternion();

	// Set fertility
	if (Tile->Level < 0 || Tile->Level > 6)
		return;

	int32 value = FMath::RandRange(-1, 1);

	int32 fTile = 0;
	count = 0;

	for (auto& element : Tile->AdjacentTiles) {
		FTileStruct* t = element.Value;

		int32 f = t->Fertility;

		if (f > 0) {
			fTile += f;
			count++;
		}
	}

	if (count > 0) {
		fTile = (fTile + 1) / count;
	}

	Tile->Fertility = FMath::Clamp(fTile + value, 1, 5);
}

void AGrid::GenerateTile(FTileStruct* Tile)
{
	if (Tile->Level <= -4)
		return;

	FTransform transform;
	FVector loc = FVector(Tile->X * 100, Tile->Y * 100, 0);
	transform.SetLocation(loc);
	transform.SetRotation(Tile->Rotation);

	int32 inst;
	if (Tile->Level < 0) {
		inst = HISMWater->AddInstance(transform);

		float data = -Tile->Level;

		HISMWater->SetCustomDataValue(inst, 0, data);
	}
	else if (Tile->Level > 6) {
		transform.SetLocation(loc + FVector(0.0f, 0.0f, 75.0f * 5));

		inst = HISMLava->AddInstance(transform);

		UNiagaraComponent* comp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), LavaSystem, transform.GetLocation());

		LavaComponents.Add(comp);
	}
	else {
		transform.SetLocation(loc + FVector(0.0f, 0.0f, 75.0f * Tile->Level));

		float r = 0.0f;
		float g = 0.0f;
		float b = 0.0f;

		if (Tile->Fertility == 1) {
			r = 255.0f;
			g = 225.0f;
			b = 45.0f;
		} 
		else if (Tile->Fertility == 2) {
			r = 152.0f;
			g = 191.0f;
			b = 100.0f;
		}
		else if (Tile->Fertility == 3) {
			r = 86.0f;
			g = 228.0f;
			b = 68.0f;
		}
		else if (Tile->Fertility == 4) {
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
		HISMGround->SetCustomDataValue(inst, 3, Tile->Fertility);
	}

	Tile->Instance = inst;
}

void AGrid::GenerateResource(FTileStruct* Tile)
{
	if (Tile->Level < 0 || Tile->Level > 4)
		return;

	int32 choiceVal = FMath::RandRange(1, 100);

	FTransform transform;
	HISMGround->GetInstanceTransform(Tile->Instance, transform);
	FVector loc = transform.GetLocation();

	int32 z = HISMGround->GetStaticMesh()->GetBounds().GetBox().GetSize().Z + 75.0f * Tile->Level;
	loc.Z = z;

	if (choiceVal == 1) {
		SpawnResource(Tile, loc, Rock);
	}
	else {
		int32 tally = 0;
		int32 count = 0;

		for (auto& element : Tile->AdjacentTiles) {
			tally += element.Value->Resource.Num();

			count++;
		}

		int32 trees = tally / count;

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

			SpawnResource(Tile, loc, Tree);

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

void AGrid::SpawnResource(FTileStruct* Tile, FVector Location, TSubclassOf<AResource> ResourceClass)
{
	AResource* resource = GetWorld()->SpawnActor<AResource>(ResourceClass, Location, GetActorRotation());

	Tile->Resource.Add(resource);
}

void AGrid::Clear()
{
	for (TArray<FTileStruct>& row : Storage) {
		for (FTileStruct& tile : row) {
			for (int j = 0; j < tile.Resource.Num(); j++) {
				tile.Resource[j]->Destroy();
			}

			tile.Resource.Empty();
		}
	}

	Storage.Empty();

	for (UNiagaraComponent* comp : LavaComponents) {
		comp->DestroyComponent();
	}

	LavaComponents.Empty();

	Camera->BuildComponent->Building->Collisions.Empty();
	Camera->BuildComponent->Building->bMoved = false;

	Clouds->Destroy();

	HISMWater->ClearInstances();
	HISMLava->ClearInstances();
	HISMGround->ClearInstances();
}