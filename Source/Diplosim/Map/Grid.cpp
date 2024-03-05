#include "Grid.h"

#include "Math/UnrealMathUtility.h"
#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Blueprint/UserWidget.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

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
	HISMWater->SetCastShadow(false);

	HISMLava = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMLava"));
	HISMLava->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMLava->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);
	HISMLava->SetCastShadow(false);

	HISMGround = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMGround"));
	HISMGround->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMGround->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);
	HISMGround->NumCustomDataFloats = 4;

	HISMFlatGround = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMFlatGround"));
	HISMFlatGround->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMFlatGround->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);
	HISMFlatGround->SetCastShadow(false);
	HISMFlatGround->NumCustomDataFloats = 4;

	Size = 22500;
	Peaks = 2;

	PercentageGround = 30;
}

void AGrid::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Camera = PController->GetPawn<ACamera>();

	MapUIInstance = CreateWidget<UUserWidget>(PController, MapUI);
	MapUIInstance->AddToViewport();

	LoadUIInstance = CreateWidget<UUserWidget>(PController, LoadUI);

	Camera->Grid = this;

	Resources.Add("Mineral");
	Resources.Add("Vegetation");

	for (TSubclassOf<AResource> resourceClass : ResourceClasses) {
		AResource* resource = GetWorld()->SpawnActor<AResource>(resourceClass, FVector::Zero(), FRotator(0.0f));

		if (resource->IsA<AMineral>())
			Resources.Find("Mineral")->Add(resource);
		else
			Resources.Find("Vegetation")->Add(resource);
	}

	Load();
}

void AGrid::Load()
{
	// Add loading screen
	LoadUIInstance->AddToViewport();

	FTimerHandle RenderTimer;
	GetWorld()->GetTimerManager().SetTimer(RenderTimer, this, &AGrid::Render, 0.001, false);
}

void AGrid::Render()
{
	// Set tile limits for ground and water
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

	TArray<FTileStruct*> chooseableTiles = {};

	int32 PercentageWater = 100 - PercentageGround;

	float levelTotal = Size * (PercentageGround / 100.0f);

	float levelCount = 0.0f;

	int32 level = 8;

	// Set tile information based on adjacent tile types until all tile struct choices are set
	while (tileTally < Size) {
		// Set current level
		if (levelCount <= 0.0f) {
			level--;

			if (level == -1)
				levelTotal = Size * (PercentageWater / 100.0f);

			if (level == -5 || level == 0) {
				levelCount = levelTotal;
			}
			else if (level < 0) {
				levelCount = 0.2 * levelTotal;

				levelTotal -= levelCount;
			}
			else {
				levelCount = FMath::Pow(0.5, FMath::Abs(level)) * levelTotal;

				levelTotal -= levelCount;
			}
		}

		// Get Tile and adjacent tiles
		FTileStruct* chosenTile;

		if (tileTally < Peaks) {
			int32 x = Storage.Num();
			int32 y = Storage[0].Num();

			int32 chosenX = FMath::RandRange(bound / 4, x - bound / 4 - 1);
			int32 chosenY = FMath::RandRange(bound / 4, y - bound / 4 - 1);

			chosenTile = &Storage[chosenX][chosenY];
		}
		else {
			int32 chosenNum = FMath::RandRange(0, chooseableTiles.Num() - 1);
			chosenTile = chooseableTiles[chosenNum];
		}

		// Set level
		chosenTile->Level = level;

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
			FillHoles(&tile);

			SetTileDetails(&tile);
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
	ResourceTiles.Empty();

	for (TArray<FTileStruct>& row : Storage) {
		for (FTileStruct& tile : row) {
			if (tile.Level < 0 || tile.Level > 4)
				continue;

			ResourceTiles.Add(&tile);
		}
	}

	int32 num = ResourceTiles.Num() / 1000;

	for (int32 i = 0; i < num; i++) {
		int32 chosenNum = FMath::RandRange(0, ResourceTiles.Num() - 1);
		FTileStruct* chosenTile = ResourceTiles[chosenNum];

		GenerateMinerals(chosenTile);
	}

	for (int32 i = 0; i < num; i++) {
		int32 chosenNum = FMath::RandRange(0, ResourceTiles.Num() - 1);
		FTileStruct* chosenTile = ResourceTiles[chosenNum];

		int32 amount = FMath::RandRange(1, 5);

		GenerateTrees(chosenTile, amount);
	}
		

	// Spawn cloud spwaner
	Clouds = GetWorld()->SpawnActor<AClouds>(CloudsClass, FVector(0.0f, 0.0f, 0.0f), FRotator(0.0f, 0.0f, 0.0f));
	Clouds->GetCloudBounds(bound * 100 / 2);

	// Remove loading screen
	LoadUIInstance->RemoveFromParent();
}

void AGrid::FillHoles(FTileStruct* Tile)
{
	// Set level
	int32 prevLevel = Tile->Level;

	float tally = Tile->Level;
	int32 count = 1;

	for (auto& element : Tile->AdjacentTiles) {
		FTileStruct* t = element.Value;

		tally += t->Level;

		count++;
	}

	tally /= count;
	
	Tile->Level = FMath::RoundHalfFromZero(tally);

	if (prevLevel == Tile->Level)
		return;

	for (auto& element : Tile->AdjacentTiles)
		FillHoles(element.Value);
}

void AGrid::SetTileDetails(FTileStruct* Tile)
{
	// Set Rotation
	int32 rand = FMath::RandRange(0, 3);

	Tile->Rotation = (FRotator(0.0f, 90.0f, 0.0f) * rand).Quaternion();

	int32 count = 0;

	// Set fertility
	if (Tile->Level < 0 || Tile->Level == 7)
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

	if (count > 0)
		fTile = (fTile + 1) / count;

	Tile->Fertility = FMath::Clamp(fTile + value, 1, 5);
}

void AGrid::GenerateTile(FTileStruct* Tile)
{
	FTransform transform;
	FVector loc = FVector(Tile->X * 100.0f, Tile->Y * 100.0f, 0);
	transform.SetLocation(loc);
	transform.SetRotation(Tile->Rotation);

	int32 inst;
	if (Tile->Level == 7) {
		transform.SetLocation(loc + FVector(0.0f, 0.0f, 75.0f * 5));

		inst = HISMLava->AddInstance(transform);

		UNiagaraComponent* comp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), LavaSystem, transform.GetLocation());

		LavaComponents.Add(comp);
	}
	else if (Tile->Level < 0.0f) {
		bool bShore = false;

		for (auto& element : Tile->AdjacentTiles) {
			FTileStruct* t = element.Value;

			if (t->Level < 0.0f)
				continue;

			bShore = true;

			break;
		}

		if (!bShore)
			return;
		transform.SetLocation(loc + FVector(0.0f, 0.0f, -200.0f));

		inst = HISMWater->AddInstance(transform);
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

		bool bFlat = true;

		for (auto& element : Tile->AdjacentTiles) {
			FTileStruct* t = element.Value;

			if (Tile->AdjacentTiles.Num() == 4 && t->Level == Tile->Level)
				continue;

			bFlat = false;

			break;
		}

		if (bFlat) {
			inst = HISMFlatGround->AddInstance(transform);
			HISMFlatGround->SetCustomDataValue(inst, 0, r);
			HISMFlatGround->SetCustomDataValue(inst, 1, g);
			HISMFlatGround->SetCustomDataValue(inst, 2, b);
			HISMFlatGround->SetCustomDataValue(inst, 3, Tile->Fertility);
		}
		else {
			inst = HISMGround->AddInstance(transform);
			HISMGround->SetCustomDataValue(inst, 0, r);
			HISMGround->SetCustomDataValue(inst, 1, g);
			HISMGround->SetCustomDataValue(inst, 2, b);
			HISMGround->SetCustomDataValue(inst, 3, Tile->Fertility);
		}
	}

	Tile->Instance = inst;
}

void AGrid::GenerateMinerals(FTileStruct* Tile)
{
	TArray<AResource*>* arr = Resources.Find("Mineral");

	int32 index = FMath::RandRange(0, arr->Num() - 1);

	AMineral* resource = Cast<AMineral>((*arr)[index]);

	int32 inst = resource->ResourceHISM->AddInstance(GetTransform(Tile));

	resource->SetRandomQuantity(inst);

	ResourceTiles.Remove(Tile);
}

void AGrid::GenerateTrees(FTileStruct* Tile, int32 Amount)
{
	int32 value = FMath::RandRange(Amount - 1, Amount);

	if (value == 0 || !ResourceTiles.Contains(Tile))
		return;

	TArray<int32> usedX;
	TArray<int32> usedY;

	for (int32 i = 0; i < Amount; i++) {
		bool validXY = false;
		int32 x = 0;
		int32 y = 0;

		while (!validXY) {
			x = FMath::RandRange(-40, 40);
			y = FMath::RandRange(-40, 40);

			if (!usedX.Contains(x) && !usedY.Contains(y))
				validXY = true;
		}

		FTransform transform;
		transform.SetLocation(GetTransform(Tile).GetLocation() + FVector(x, y, 0.0f));

		TArray<AResource*>* arr = Resources.Find("Vegetation");

		int32 index = FMath::RandRange(0, arr->Num() - 1);

		AVegetation* resource = Cast<AVegetation>((*arr)[index]);

		int32 inst = resource->ResourceHISM->AddInstance(transform);

		resource->SetQuantity(inst, resource->MaxYield);

		resource->ResourceHISM->SetCustomDataValue(inst, 3, 1.0f);
	}

	ResourceTiles.Remove(Tile);

	for (auto& element : Tile->AdjacentTiles)
		GenerateTrees(element.Value, value);
}

FTransform AGrid::GetTransform(FTileStruct* Tile)
{
	bool bFlat = true;

	for (auto& element : Tile->AdjacentTiles) {
		FTileStruct* t = element.Value;

		if (t->Level == Tile->Level)
			continue;

		bFlat = false;

		break;
	}

	FTransform transform;
	int32 z = HISMGround->GetStaticMesh()->GetBounds().GetBox().GetSize().Z;

	if (bFlat)
		HISMFlatGround->GetInstanceTransform(Tile->Instance, transform);
	else
		HISMGround->GetInstanceTransform(Tile->Instance, transform);

	transform.SetLocation(transform.GetLocation() + FVector(0.0f, 0.0f, z));

	return transform;
}

void AGrid::Clear()
{
	for (const TPair<FString, TArray<AResource*>>& pair : Resources) {
		for (AResource* r : pair.Value) {
			r->ResourceHISM->ClearInstances();
		}
	}

	Storage.Empty();

	for (UNiagaraComponent* comp : LavaComponents)
		comp->DestroyComponent();

	LavaComponents.Empty();

	Camera->BuildComponent->Building->Collisions.Empty();
	Camera->BuildComponent->Building->bMoved = false;

	Clouds->Destroy();

	HISMLava->ClearInstances();
	HISMWater->ClearInstances();
	HISMGround->ClearInstances();
	HISMFlatGround->ClearInstances();
}