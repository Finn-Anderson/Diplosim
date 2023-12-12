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
	HISMWater->SetCanEverAffectNavigation(false);
	HISMWater->NumCustomDataFloats = 1;

	HISMGround = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMGround"));
	HISMGround->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMGround->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);
	HISMGround->NumCustomDataFloats = 4;

	HISMMound = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMMound"));
	HISMMound->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMMound->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);
	HISMMound->NumCustomDataFloats = 4;

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
	// Set tile struct to each storage slot based on size chosen
	TArray<int32> pos;

	for (int32 y = 0; y < Size; y++)
	{
		for (int32 x = 0; x < Size; x++)
		{
			FTileStruct tile;
			tile.X = x;
			tile.Y = y;

			pos.Add(Storage.Num());

			Storage.Add(tile);
		}
	}

	// Set the sea size for the outer rim of the map
	int32 seaSize = 20;

	for (FTileStruct tile : Storage) {
		if (tile.X >= seaSize && tile.Y >= seaSize && tile.X <= (Size - seaSize - 1) && tile.Y <= (Size - seaSize - 1))
			continue;

		tile.Choice = HISMWater;
		tile.Fertility = 1;
		tile.bIsSea = true;

		Storage[tile.X + (tile.Y * Size)] = tile;

		pos.Remove(tile.X + (tile.Y * Size));

		TArray<FTileStruct> tiles;
		TArray<class UHierarchicalInstancedStaticMeshComponent*> choices;

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

		SetTileChoices(tiles);
	}
	
	// Set tile information based on adjacent tile types until all tile struct choices are set
	bool bChoicesPending = true;
	while (bChoicesPending) {
		// Decalre variables
		TArray<FTileStruct> tiles;

		TArray<FTileStruct> chosenList;
		FTileStruct chosenTile;

		TArray<UHierarchicalInstancedStaticMeshComponent*> targetBanList = {};

		UHierarchicalInstancedStaticMeshComponent* targetChoice = GetTargetChoice(pos, targetBanList);
		targetBanList.Add(targetChoice);

		// Go down quantity or repick targetChoice
		while (chosenList.IsEmpty()) {
			chosenList = GetChosenList(pos, targetChoice);

			if (chosenList.IsEmpty()) {
				targetChoice = GetTargetChoice(pos, targetBanList);
			}
		}

		int32 chosenNum = FMath::RandRange(0, chosenList.Num() - 1);
		chosenTile = chosenList[chosenNum];

		tiles.Empty();

		tiles.Add(Storage[(chosenTile.X - 1) + (chosenTile.Y * Size)]);
		tiles.Add(Storage[chosenTile.X + ((chosenTile.Y - 1) * Size)]);
		tiles.Add(Storage[(chosenTile.X + 1) + (chosenTile.Y * Size)]);
		tiles.Add(Storage[chosenTile.X + ((chosenTile.Y + 1) * Size)]);

		// Set fertility
		if (targetChoice == HISMWater) {
			chosenTile.Fertility = 1;

			for (FTileStruct t : tiles) {
				if (!t.bIsSea)
					continue;

				chosenTile.bIsSea = true;

				break;
			}
		}
		else {
			int32 value = FMath::RandRange(-1, 1);

			int32 fTile = 0;
			int32 count = 0;
			for (FTileStruct t : tiles) {
				int32 f = GetFertility(t);

				if (f > 0) {
					fTile += f;
					count++;
				}
			}

			fTile = (fTile + 1) / count;

			chosenTile.Fertility = FMath::Clamp(fTile + value, 1, 5);
		}
				
		// Calculating tile type based on adjacent tile choices
		chosenTile.Choice = targetChoice;

		Storage[chosenTile.X + (chosenTile.Y * Size)] = chosenTile;

		SetTileChoices(tiles);

		pos.Remove(chosenTile.X + (chosenTile.Y * Size));

		if (pos.IsEmpty()) {
			bChoicesPending = false;
		}
	}

	// Spawn Actor
	for (FTileStruct tile : Storage) {
		GenerateTile(tile.Choice, tile.Fertility, tile.X, tile.Y);
	}

	// Set Camera Bounds
	FTransform transform;

	FTileStruct min = Storage[10 + 10 * Size];
	min.Choice->GetInstanceTransform(min.Instance, transform);
	FVector c1 = transform.GetLocation();

	FTileStruct max = Storage[139 + 139 * Size];
	max.Choice->GetInstanceTransform(max.Instance, transform);
	FVector c2 = transform.GetLocation();

	Camera->MovementComponent->SetBounds(c2, c1);

	// Spawn resources
	for (int32 i = 0; i < Storage.Num(); i++) {
		GenerateResource(i);
	}
}

int32 AGrid::GetFertility(FTileStruct Tile)
{
	return Tile.Fertility;
}

void AGrid::SetTileChoices(TArray<FTileStruct> Tiles)
{
	for (FTileStruct tile : Tiles) {
		TArray<class UHierarchicalInstancedStaticMeshComponent*> choices;

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
			if (t.Choice == nullptr)
				continue;

			if (t.Choice == HISMWater) {
				choices.Add(HISMWater);
				choices.Add(HISMGround);
			}
			else if (t.Choice == HISMGround) {
				choices.Add(HISMWater);
				choices.Add(HISMGround);
				choices.Add(HISMMound);
			}
			else {
				choices.Add(HISMGround);
				choices.Add(HISMMound);
			}
		}

		tile.ChoiceList = choices;

		Storage[tile.X + (tile.Y * Size)] = tile;
	}
}

UHierarchicalInstancedStaticMeshComponent* AGrid::GetTargetChoice(TArray<int32> Pos, TArray<UHierarchicalInstancedStaticMeshComponent*> TargetBanList)
{
	TArray<UHierarchicalInstancedStaticMeshComponent*> targetChoiceList;

	// Set target choice from all possible choices in the world
	for (int32 i = 0; i < Pos.Num(); i++) {
		FTileStruct tile = Storage[Pos[i]];

		if (tile.Choice != nullptr)
			continue;

		for (UHierarchicalInstancedStaticMeshComponent* c : tile.ChoiceList) {
			if (TargetBanList.Contains(c))
				continue;

			targetChoiceList.Add(c);
		}
	}

	int32 targetNum = FMath::RandRange(0, targetChoiceList.Num() - 1);

	return targetChoiceList[targetNum];
}

TArray<FTileStruct> AGrid::GetChosenList(TArray<int32> Pos, class UHierarchicalInstancedStaticMeshComponent* TargetChoice) 
{
	int32 targetQuantity = 0;

	TArray<FTileStruct> tiles;

	TArray<FTileStruct> chosenList;

	for (int32 i = 0; i < Pos.Num(); i++) {
		FTileStruct tile = Storage[Pos[i]];

		if (tile.ChoiceList.IsEmpty() || !tile.ChoiceList.Contains(TargetChoice))
			continue;

		tiles.Empty();

		tiles.Add(Storage[(tile.X - 1) + (tile.Y * Size)]);
		tiles.Add(Storage[tile.X + ((tile.Y - 1) * Size)]);
		tiles.Add(Storage[(tile.X + 1) + (tile.Y * Size)]);
		tiles.Add(Storage[tile.X + ((tile.Y + 1) * Size)]);

		int32 quantity = 0;
		for (FTileStruct t : tiles) {
			if (t.Choice == TargetChoice) {
				quantity++;
			}
		}

		int32 count = 0;
		for (int32 y = 1; y < 9; y++)
		{
			for (int32 x = 1; x < 9; x++)
			{
				if (Storage[(tile.X - x) + (tile.Y * Size)].Choice == TargetChoice) {
					count++;
				}
				if (Storage[(tile.X + x) + (tile.Y * Size)].Choice == TargetChoice) {
					count++;
				}
				if (Storage[tile.X + ((tile.Y - y) * Size)].Choice == TargetChoice) {
					count++;
				}
				if (Storage[tile.X + ((tile.Y + y) * Size)].Choice == TargetChoice) {
					count++;
				}
			}
		}

		if (quantity < targetQuantity || count > 125)
			continue;

		if (quantity > targetQuantity) {
			chosenList.Empty();

			targetQuantity = quantity;
		}

		chosenList.Add(tile);
	}

	return chosenList;
}

void AGrid::GenerateTile(UHierarchicalInstancedStaticMeshComponent* Choice, int32 Fertility, int32 x, int32 y)
{
	FTransform transform;
	FVector loc = FVector(100.0f * x - (100.0f * (Size / 2)), 100.0f * y - (100.0f * (Size / 2)), 0);
	transform.SetLocation(loc);

	int32 inst;
	if (Choice == HISMWater) {
		inst = HISMWater->AddInstance(transform);

		double distToChosen = FVector::Dist(loc, FVector(1000000.0f, 1000000.0f, 1000000.0f));

		for (FTileStruct tile : Storage) {
			if (tile.Choice == HISMWater)
				continue;
			
			FVector tileLoc = FVector(100.0f * tile.X - (100.0f * (Size / 2)), 100.0f * tile.Y - (100.0f * (Size / 2)), 0);

			double distToTile = FVector::Dist(loc, tileLoc);

			if (distToTile < distToChosen) {
				distToChosen = distToTile;
			}
		}

		float data = FMath::Sqrt(distToChosen / 1000.0f);

		HISMWater->SetCustomDataValue(inst, 0, data);
	}
	else {
		float r = 0.0f;
		float g = 0.0f;
		float b = 0.0f;

		if (Fertility == 1) {
			TArray<FTileStruct> tiles;
			tiles.Add(Storage[(x - 1) + (y * Size)]);
			tiles.Add(Storage[(x + 1) + (y * Size)]);
			tiles.Add(Storage[x + ((y - 1) * Size)]);
			tiles.Add(Storage[x + ((y + 1) * Size)]);

			bool bSand = false;
			for (FTileStruct tile : tiles) {
				if (tile.Choice == HISMWater) {
					bSand = true;

					break;
				}
			}

			if (bSand) {
				r = 231.0f;
				g = 215.0f;
				b = 90.0f;
			}
			else {
				r = 255.0f;
				g = 225.0f;
				b = 45.0f;
			}
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

		if (Choice == HISMGround) {
			inst = HISMGround->AddInstance(transform);
			HISMGround->SetCustomDataValue(inst, 0, r);
			HISMGround->SetCustomDataValue(inst, 1, g);
			HISMGround->SetCustomDataValue(inst, 2, b);
			HISMGround->SetCustomDataValue(inst, 3, Fertility);
		}
		else {
			inst = HISMMound->AddInstance(transform);
			HISMMound->SetCustomDataValue(inst, 0, r);
			HISMMound->SetCustomDataValue(inst, 1, g);
			HISMMound->SetCustomDataValue(inst, 2, b);
			HISMMound->SetCustomDataValue(inst, 3, Fertility);
		}
	}

	Storage[x + (y * Size)].Instance = inst;
}

void AGrid::GenerateResource(int32 Pos)
{
	FTileStruct tile = Storage[Pos];

	if (tile.Choice == HISMWater)
		return;

	int32 choiceVal = FMath::RandRange(1, 100);

	FTransform transform;
	tile.Choice->GetInstanceTransform(tile.Instance, transform);
	FVector loc = transform.GetLocation();

	int32 z = tile.Choice->GetStaticMesh()->GetBounds().GetBox().GetSize().Z - 50.0f;
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
	HISMGround->ClearInstances();
	HISMMound->ClearInstances();
}