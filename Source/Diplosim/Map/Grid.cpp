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

	HISMGround = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMGround"));
	HISMGround->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMGround->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);
	HISMGround->NumCustomDataFloats = 4;

	HISMMound = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMMound"));
	HISMMound->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMMound->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);
	HISMMound->NumCustomDataFloats = 4;

	HISMHill = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMHill"));
	HISMHill->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMHill->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);

	HISMMountain = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMMountain"));
	HISMMountain->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMMountain->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);

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
			int32 mean = 1;
			UHierarchicalInstancedStaticMeshComponent* choice = HISMGround;

			int32 low = 0;
			int32 high = 1000;

			if (x < (Size - (Size - 20)) || y < (Size - (Size - 20)) || x > (Size - 21) || y > (Size - 21)) {
				choice = HISMWater;
			}
			else {
				int32 value = FMath::RandRange(-1, 1);

				FTileStruct xTile = Storage.Last();
				FTileStruct yTile = Storage[x + ((y - 1) * Size)];

				int32 fertility = (GetFertility(yTile) + GetFertility(xTile) + 1) / 2;

				mean = FMath::Clamp(fertility + value, 1, 5);
				
				// Calculating tile type based on adjacant tiles
				UHierarchicalInstancedStaticMeshComponent* checkX = xTile.Choice;
				UHierarchicalInstancedStaticMeshComponent* checkY = yTile.Choice;

				TArray<UHierarchicalInstancedStaticMeshComponent*> choiceList;

				if (checkX == HISMMountain || checkY == HISMMountain) {
					for (int32 i = 0; i < 20; i++) {
						choiceList.Add(HISMMountain);
					}
					choiceList.Add(HISMHill);
				}
				if (checkX == HISMHill || checkY == HISMHill) {
					choiceList.Add(HISMMountain);
					for (int32 i = 0; i < 20; i++) {
						choiceList.Add(HISMHill);
					}
					choiceList.Add(HISMMound);
				}
				if (checkX == HISMMound || checkY == HISMMound) {
					choiceList.Add(HISMHill);
					for (int32 i = 0; i < 40; i++) {
						choiceList.Add(HISMMound);
					}
					choiceList.Add(HISMGround);
				}
				if (checkX == HISMGround || checkY == HISMGround) {
					choiceList.Add(HISMMound);
					for (int32 i = 0; i < 40; i++) {
						choiceList.Add(HISMGround);
					}
					choiceList.Add(HISMWater);
				}
				if (checkX == HISMWater || checkY == HISMWater) {
					choiceList.Add(HISMGround);
					for (int32 i = 0; i < 40; i++) {
						choiceList.Add(HISMWater);
					}
				}

				if (checkX == HISMMountain || checkY == HISMMountain) {
					choiceList.Remove(HISMMound);
					choiceList.Remove(HISMGround);
					choiceList.Remove(HISMWater);
				}
				if (checkX == HISMHill || checkY == HISMHill) {
					choiceList.Remove(HISMGround);
					choiceList.Remove(HISMWater);
				}
				if (checkX == HISMMound || checkY == HISMMound) {
					choiceList.Remove(HISMMountain);
					choiceList.Remove(HISMWater);
				}
				if (checkX == HISMGround || checkY == HISMGround) {
					choiceList.Remove(HISMMountain);
					choiceList.Remove(HISMHill);
				}
				if (checkX == HISMWater || checkY == HISMWater) {
					choiceList.Remove(HISMMountain);
					choiceList.Remove(HISMHill);
					choiceList.Remove(HISMMound);
				}

				int32 choiceVal = FMath::RandRange(0, (choiceList.Num() - 1));

				choice = choiceList[choiceVal];
			}

			// Spawn Actor
			GenerateTile(choice, mean, x, y);
		}
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

void AGrid::GenerateTile(UHierarchicalInstancedStaticMeshComponent* Choice, int32 Mean, int32 x, int32 y)
{
	FTransform transform;
	FVector loc = FVector(100.0f * x - (100.0f * (Size / 2)), 100.0f * y - (100.0f * (Size / 2)), 0);
	transform.SetLocation(loc);

	int32 inst;
	if (Choice == HISMWater) {
		inst = HISMWater->AddInstance(transform);
	}
	else if (Choice == HISMGround || Choice == HISMMound) {
		float r = 0.0f;
		float g = 0.0f;
		float b = 0.0f;

		if (Mean == 1) {
			FTileStruct xTile = Storage.Last();
			FTileStruct yTile = Storage[x + ((y - 1) * Size)];

			if (xTile.Choice == HISMWater || yTile.Choice == HISMWater) {
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
		else if (Mean == 2) {
			r = 152.0f;
			g = 191.0f;
			b = 100.0f;
		}
		else if (Mean == 3) {
			r = 86.0f;
			g = 228.0f;
			b = 68.0f;
		}
		else if (Mean == 4) {
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
			HISMGround->SetCustomDataValue(inst, 3, Mean);
		}
		else {
			inst = HISMMound->AddInstance(transform);
			HISMMound->SetCustomDataValue(inst, 0, r);
			HISMMound->SetCustomDataValue(inst, 1, g);
			HISMMound->SetCustomDataValue(inst, 2, b);
			HISMMound->SetCustomDataValue(inst, 3, Mean);
		}
	}
	else if (Choice == HISMHill) {
		inst = HISMHill->AddInstance(transform);
	}
	else {
		inst = HISMMountain->AddInstance(transform);
	}

	FTileStruct tile;
	tile.Choice = Choice;
	tile.Instance = inst;

	Storage.Add(tile);
}

void AGrid::GenerateResource(int32 Pos)
{
	FTileStruct tile = Storage[Pos];

	int32 choiceVal = FMath::RandRange(1, 100);

	if (tile.Choice == HISMGround || tile.Choice == HISMMound) {
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

			if (xTile.Choice == HISMHill || yTile.Choice == HISMHill)
				return;

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
}

void AGrid::SpawnResource(int32 Pos, FVector Location, TSubclassOf<AResource> ResourceClass)
{
	FTileStruct tile = Storage[Pos];

	AResource* resource = GetWorld()->SpawnActor<AResource>(ResourceClass, Location, GetActorRotation());

	tile.Resource.Add(resource);

	Storage[Pos] = tile;
}

int32 AGrid::GetFertility(FTileStruct Tile)
{
	int32 fertility = 1;
	if (Tile.Choice == HISMGround || Tile.Choice == HISMMound) {
		fertility = Tile.Choice->PerInstanceSMCustomData[Tile.Instance * 4 + 3];
	}

	return fertility;
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
	HISMHill->ClearInstances();
	HISMMountain->ClearInstances();
}