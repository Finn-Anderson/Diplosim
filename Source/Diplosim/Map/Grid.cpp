#include "Grid.h"

#include "Math/UnrealMathUtility.h"
#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMeshSocket.h"
#include "Blueprint/UserWidget.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "Engine/ExponentialHeightFog.h"

#include "Atmosphere/Clouds.h"
#include "Resources/Mineral.h"
#include "Resources/Vegetation.h"
#include "Atmosphere/AtmosphereComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Components/CameraMovementComponent.h"
#include "Universal/EggBasket.h"
#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"

AGrid::AGrid()
{
	PrimaryActorTick.bCanEverTick = false;

	FCollisionResponseContainer response;
	response.GameTraceChannel1 = ECR_Block;
	response.WorldStatic = ECR_Ignore;
	response.WorldDynamic = ECR_Overlap;
	response.Pawn = ECR_Ignore;
	response.PhysicsBody = ECR_Ignore;
	response.Vehicle = ECR_Overlap;
	response.Destructible = ECR_Ignore;
	response.GameTraceChannel2 = ECR_Ignore;
	response.GameTraceChannel3 = ECR_Ignore;
	response.GameTraceChannel4 = ECR_Ignore;

	HISMLava = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMLava"));
	HISMLava->SetupAttachment(GetRootComponent());
	HISMLava->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HISMLava->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMLava->SetCollisionResponseToChannels(response);
	HISMLava->SetCanEverAffectNavigation(false);
	HISMLava->SetCastShadow(false);

	HISMGround = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMGround"));
	HISMGround->SetupAttachment(GetRootComponent());
	HISMGround->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HISMGround->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMGround->SetCollisionResponseToChannels(response);
	HISMGround->NumCustomDataFloats = 8;

	HISMFlatGround = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMFlatGround"));
	HISMFlatGround->SetupAttachment(GetRootComponent());
	HISMFlatGround->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HISMFlatGround->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMFlatGround->SetCollisionResponseToChannels(response);
	HISMFlatGround->SetCastShadow(false);
	HISMFlatGround->NumCustomDataFloats = 8;

	HISMRampGround = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMRampGround"));
	HISMRampGround->SetupAttachment(GetRootComponent());
	HISMRampGround->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HISMRampGround->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMRampGround->SetCollisionResponseToChannels(response);
	HISMRampGround->NumCustomDataFloats = 8;

	HISMRiver = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMRiver"));
	HISMRiver->SetupAttachment(GetRootComponent());
	HISMRiver->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HISMRiver->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMRiver->SetCollisionResponseToChannels(response);
	HISMRiver->SetCanEverAffectNavigation(false);
	HISMRiver->NumCustomDataFloats = 4;

	LavaComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("LavaComponent"));
	LavaComponent->SetupAttachment(GetRootComponent());
	LavaComponent->bAutoActivate = false;

	AtmosphereComponent = CreateDefaultSubobject<UAtmosphereComponent>(TEXT("AtmosphereComponent"));
	AtmosphereComponent->WindComponent->SetupAttachment(RootComponent);

	CloudComponent = CreateDefaultSubobject<UCloudComponent>(TEXT("CloudComponent"));

	CrystalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrystalMesh"));
	CrystalMesh->SetCollisionProfileName("NoCollision", false);
	CrystalMesh->SetComponentTickEnabled(false);
	CrystalMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -1000.0f));
	CrystalMesh->SetupAttachment(RootComponent);

	Size = 22500;
	Peaks = 2;
	Rivers = 2;
	Type = EType::Round;
	bLava = true;
	MaxLevel = 7;

	PercentageGround = 30;

	Seed = "";
}

void AGrid::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Camera = PController->GetPawn<ACamera>();

	MapUIInstance = CreateWidget<UUserWidget>(PController, MapUI);

	LoadUIInstance = CreateWidget<UUserWidget>(PController, LoadUI);

	Camera->Grid = this;

	for (FResourceHISMStruct &ResourceStruct : TreeStruct)
		ResourceStruct.Resource = GetWorld()->SpawnActor<AResource>(ResourceStruct.ResourceClass, FVector::Zero(), FRotator(0.0f));

	for (FResourceHISMStruct& ResourceStruct : FlowerStruct)
		ResourceStruct.Resource = GetWorld()->SpawnActor<AResource>(ResourceStruct.ResourceClass, FVector::Zero(), FRotator(0.0f));

	for (FResourceHISMStruct &ResourceStruct : MineralStruct)
		ResourceStruct.Resource = GetWorld()->SpawnActor<AResource>(ResourceStruct.ResourceClass, FVector::Zero(), FRotator(0.0f));
}

void AGrid::Load()
{
	// Add loading screen
	LoadUIInstance->AddToViewport();

	Stream.Initialize(*Seed);

	FString s = Seed;

	if (s == "")
		s = FString::FromInt(Stream.GetCurrentSeed());

	Camera->UpdateMapSeed(s);

	Seed = "";

	FTimerHandle RenderTimer;
	GetWorld()->GetTimerManager().SetTimer(RenderTimer, this, &AGrid::Render, 0.001, false);
}

void AGrid::Render()
{
	// Set map limtis
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

	// Set adjacent tile information
	for (int32 x = 0; x < bound; x++) {
		for (int32 y = 0; y < bound; y++) {
			FTileStruct* tile = &Storage[x][y];

			TMap<FString, FTileStruct*> map;

			if (x > 0)
				map.Add("Left", &Storage[x - 1][y]);

			if (x < (bound - 1))
				map.Add("Right", &Storage[x + 1][y]);

			if (y > 0)
				map.Add("Below", &Storage[x][y - 1]);

			if (y < (bound - 1))
				map.Add("Above", &Storage[x][y + 1]);

			for (auto& element : map)
				tile->AdjacentTiles.Add(element.Key, element.Value);
		}
	}

	TArray<FTileStruct*> peaksList;

	TArray<FTileStruct*> chooseableTiles = {};

	int32 levelTotal = Size * (PercentageGround / 100.0f);

	int32 level = MaxLevel + 1;

	int32 levelCount = 0;

	float pow = 0.5f;

	// Set tile information based on adjacent tile types until all tile struct choices are set
	while (true) {
		// Set current level
		if (levelCount <= 0) {
			level--;

			if (level < 0)
				break;

			float percentage = (1.85f / FMath::Sqrt(2.0f * PI * FMath::Square(MaxLevel / 2.5f)) * FMath::Exp(-1.0f * (FMath::Square(level) / (1.85f * FMath::Square(MaxLevel / 2.5f)))));
			levelCount = FMath::Clamp(percentage * levelTotal, 0, levelTotal);
		}

		// Get Tile and adjacent tiles
		FTileStruct* chosenTile = nullptr;

		if (peaksList.Num() < Peaks) {
			int32 x = Storage.Num();
			int32 y = Storage[0].Num();

			int32 chosenX = Stream.RandRange(bound / 4, x - bound / 4 - 1);
			int32 chosenY = Stream.RandRange(bound / 4, y - bound / 4 - 1);

			chosenTile = &Storage[chosenX][chosenY];

			peaksList.Add(chosenTile);

			if (peaksList.Num() == 2 && Type == EType::Mountain) {
				TArray<FTileStruct*> tiles = CalculatePath(peaksList[0], peaksList[1]);

				float distance = FMath::Sqrt((FMath::Square(peaksList[0]->X - peaksList[1]->X) + FMath::Square(peaksList[0]->Y - peaksList[1]->Y)) * 1.0f);

				pow = FMath::Max(0.5f, FMath::Min(distance / levelCount, 0.7f));

				for (FTileStruct* tile : tiles) {
					if (tile == peaksList[0] || tile == peaksList[1])
						continue;

					float d0 = FMath::Sqrt((FMath::Square(peaksList[0]->X - tile->X) + FMath::Square(peaksList[0]->Y - tile->Y)) * 1.0f);
					float d1 = FMath::Sqrt((FMath::Square(peaksList[1]->X - tile->X) + FMath::Square(peaksList[1]->Y - tile->Y)) * 1.0f);

					if (FMath::Abs(d0 - d1) < 1.0f && peaksList.Num() < 3)
						peaksList.Add(tile);

					tile->Level = level;

					// Set adjacent tiles to choose from
					for (auto& element : tile->AdjacentTiles)
						if (!chooseableTiles.Contains(element.Value) && element.Value->Level < 0)
							chooseableTiles.Add(element.Value);

					chooseableTiles.Remove(tile);

					levelCount--;
				}
			}
		}
		else {
			if (chooseableTiles.IsEmpty())
				break;

			int32 chosenNum = Stream.RandRange(0, chooseableTiles.Num() - 1);
			chosenTile = chooseableTiles[chosenNum];

			for (FTileStruct* tile : chooseableTiles) {
				int32 distance = 1000000000;

				for (FTileStruct* peak : peaksList) {
					int32 d = FVector2D::Distance(FVector2D(peak->X, peak->Y), FVector2D(tile->X, tile->Y));

					if (distance > d)
						distance = d;
				}

				int32 chosenDistance = 1000000000;

				for (FTileStruct* peak : peaksList) {
					int32 d = FVector2D::Distance(FVector2D(peak->X, peak->Y), FVector2D(chosenTile->X, chosenTile->Y));

					if (chosenDistance > d)
						chosenDistance = d;
				}

				if (chosenDistance > distance + 20)
					chosenTile = tile;
			}
		}

		// Set level
		chosenTile->Level = level;

		// Set adjacent tiles to choose from
		for (auto& element : chosenTile->AdjacentTiles)
			if (!chooseableTiles.Contains(element.Value) && element.Value->Level < 0)
				chooseableTiles.Add(element.Value);

		chooseableTiles.Remove(chosenTile);

		levelCount--;
	}

	for (TArray<FTileStruct> &row : Storage)
		for (FTileStruct &tile : row)
			FillHoles(&tile);

	for (TArray<FTileStruct>& row : Storage)
		for (FTileStruct& tile : row)
			SetTileDetails(&tile);

	// Spawn Rivers
	TArray<FTileStruct*> riverStartTiles;

	for (TArray<FTileStruct>& row : Storage) {
		for (FTileStruct& tile : row) {
			int32 lowPoint = FMath::Floor(MaxLevel / 2.0f);
			int32 highPoint = MaxLevel;

			if (bLava)
				highPoint -= 2;

			if (tile.Level < lowPoint || tile.Level > highPoint || tile.bRamp || tile.bEdge)
				continue;

			bool bNextToLava = false;

			if (bLava) {
				for (auto& element : tile.AdjacentTiles) {
					FTileStruct* t = element.Value;

					if (t->Level != 7)
						continue;

					bNextToLava = true;
				}
			}

			if (!bNextToLava)
				riverStartTiles.Add(&tile);
		}
	}

	for (int32 i = 0; i < Rivers; i++) {
		if (riverStartTiles.IsEmpty())
			break;

		int32 chosenNum = Stream.RandRange(0, riverStartTiles.Num() - 1);
		FTileStruct* chosenTile = riverStartTiles[chosenNum];

		FTileStruct* closestPeak = nullptr;

		for (FTileStruct* peak : peaksList) {
			if (closestPeak == nullptr) {
				closestPeak = peak;

				continue;
			}

			float d1 = FVector2D::Distance(FVector2D(peak->X, peak->Y), FVector2D(chosenTile->X, chosenTile->Y));
			float d2 = FVector2D::Distance(FVector2D(closestPeak->X, closestPeak->Y), FVector2D(chosenTile->X, chosenTile->Y));

			if (d2 > d1)
				closestPeak = peak;
		}

		TArray<FTileStruct*> tiles = GenerateRiver(chosenTile, closestPeak);

		for (FTileStruct* tile : tiles) {
			for (auto& element : tile->AdjacentTiles) {
				FTileStruct* t = element.Value;

				if (t->bRiver)
					continue;

				if (tile->Level > t->Level)
					t->Level = tile->Level;

				t->bEdge = true;
			}
		}	
	}

	// Spawn Actor
	for (TArray<FTileStruct> &row : Storage)
		for (FTileStruct &tile : row)
			GenerateTile(&tile);

	// Spawn resources
	ResourceTiles.Empty();

	for (TArray<FTileStruct> &row : Storage) {
		for (FTileStruct &tile : row) {
			if (tile.Level < 0 || tile.Level > 4 || tile.bRiver || tile.bRamp)
				continue;

			ResourceTiles.Add(&tile);
		}
	}

	int32 num = ResourceTiles.Num() / 2000;

	TArray<TArray<FTileStruct*>> ValidMineralTiles;

	for (FTileStruct* tile : ResourceTiles) {
		bool valid = true;
		TArray<FTileStruct*> tiles;

		GetValidSpawnLocations(tile, tile, 1, valid, tiles);

		if (!valid)
			continue;

		ValidMineralTiles.Add(tiles);
	}

	for (FResourceHISMStruct &ResourceStruct : MineralStruct) {
		for (int32 i = 0; i < (num * ResourceStruct.Multiplier); i++) {
			int32 chosenNum = Stream.RandRange(0, ValidMineralTiles.Num() - 1);
			TArray<FTileStruct*> chosenTiles = ValidMineralTiles[chosenNum];

			GenerateMinerals(chosenTiles[0], ResourceStruct.Resource);

			for (FTileStruct* tile : chosenTiles)
				for (int32 j = ValidMineralTiles.Num() - 1; j > -1; j--)
					if (ValidMineralTiles[j].Contains(tile))
						ValidMineralTiles.RemoveAt(j);
		}
	}

	TMap<int32, TArray<FResourceHISMStruct>> vegetation;
	vegetation.Add(2, FlowerStruct);
	vegetation.Add(5, TreeStruct);

	for (auto& element : vegetation) {
		int32 iterations = num;

		if (element.Key == 2)
			iterations = FMath::Max(1.0f, iterations / 3.0f);

		for (int32 i = 0; i < iterations; i++) {
			for (FResourceHISMStruct ResourceStruct : element.Value)
				ResourceStruct.Resource->ResourceHISM->bAutoRebuildTreeOnInstanceChanges = false;

			int32 chosenNum = Stream.RandRange(0, ResourceTiles.Num() - 1);
			FTileStruct* chosenTile = ResourceTiles[chosenNum];

			int32 amount = Stream.RandRange(element.Key / 2, element.Key);

			bool bTree = false;

			int32 scale = 1.5f;

			if (element.Key == 5) {
				bTree = true;

				scale = 1.1f;
			}

			VegetationLimitCounter = 0;
			VegetationLimit = amount * (element.Key * 25);

			VegetationTiles.Empty();

			GenerateVegetation(element.Value, chosenTile, chosenTile, amount, scale, bTree);

			if (bTree)
				for (FTileStruct* tile : VegetationTiles)
					ResourceTiles.Remove(tile);

			for (FResourceHISMStruct ResourceStruct : element.Value) {
				ResourceStruct.Resource->ResourceHISM->bAutoRebuildTreeOnInstanceChanges = true;

				ResourceStruct.Resource->ResourceHISM->BuildTreeIfOutdated(true, true);
			}
		}
	}

	// Set Atmosphere Affects
	SetSeasonAffect(AtmosphereComponent->Calendar.Period, 1.0f);

	AtmosphereComponent->SetWindDimensions(bound);

	// Spawn clouds
	CloudComponent->ActivateCloud();

	// Set Camera Bounds
	FVector c1 = FVector(bound * 100, bound * 100, 0);
	FVector c2 = FVector(-bound * 100, -bound * 100, 0);

	Camera->MovementComponent->SetBounds(c1, c2);

	// Spawn egg basket
	GetWorld()->GetTimerManager().SetTimer(EggBasketTimer, this, &AGrid::SpawnEggBasket, 300.0f, true, 0.0f);

	// Lava Component
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(LavaComponent, TEXT("SpawnLocations"), LavaSpawnLocations);
	LavaComponent->SetVariableFloat(TEXT("SpawnRate"), LavaSpawnLocations.Num() / 10.0f);
	LavaComponent->Activate();

	// Remove loading screen
	LoadUIInstance->RemoveFromParent();

	if (Camera->PauseUIInstance->IsInViewport())
		Camera->SetPause(true, true);
}

TArray<FTileStruct*> AGrid::CalculatePath(FTileStruct* Tile, FTileStruct* Target)
{
	TArray<FTileStruct*> tiles;

	tiles.Add(Tile);

	FTileStruct* closestTile = Tile;

	for (auto& element : Tile->AdjacentTiles) {
		float currentDistance = FMath::Sqrt((FMath::Square(closestTile->X - Target->X) + FMath::Square(closestTile->Y - Target->Y)) * 1.0f);
		float newDistance = FMath::Sqrt((FMath::Square(element.Value->X - Target->X) + FMath::Square(element.Value->Y - Target->Y)) * 1.0f);

		if (!tiles.Contains(element.Value))
			tiles.Add(element.Value);

		if (currentDistance > newDistance)
			closestTile = element.Value;
	}

	if (closestTile != Tile)
		tiles.Append(CalculatePath(closestTile, Target));

	return tiles;
}

void AGrid::FillHoles(FTileStruct* Tile)
{
	TArray<int32> levels;
	levels.Add(Tile->Level);

	for (auto& element : Tile->AdjacentTiles)
		levels.Add(element.Value->Level);

	float result = Tile->Level;

	if (!levels.IsEmpty()) {
		for (int32 i = 0; i < levels.Num() - 1; i++)
			for (int32 j = 0; j < levels.Num() - i - 1; j++)
				if (levels[j] > levels[j + 1])
					levels.Swap(j, j + 1);

		if (levels.Num() % 2 == 0)
			result = FMath::RoundHalfFromZero((levels[levels.Num() / 2] + levels[levels.Num() / 2 - 1]) / 2.0f);
		else
			result = levels[levels.Num() / 2];
	}

	if (bLava && levels.Contains(MaxLevel) && Tile->Level != MaxLevel && result < MaxLevel - 1)
		result = MaxLevel - 1;

	if (result == Tile->Level)
		return;

	Tile->Level = result;
}

void AGrid::SetTileDetails(FTileStruct* Tile)
{
	// Set Rotation
	int32 rand = Stream.RandRange(0, 3);

	Tile->Rotation = (FRotator(0.0f, 90.0f, 0.0f) * rand).Quaternion();

	// Set fertility
	if ((bLava && Tile->Level == MaxLevel) || Tile->Level < 0)
		return;

	int32 value = Stream.RandRange(-1, 1);

	int32 fTile = 0;
	int32 count = 0;

	for (auto& element : Tile->AdjacentTiles) {
		FTileStruct* t = element.Value;

		if (bLava && t->Level == MaxLevel) {
			Tile->Fertility = 0;

			return;
		}

		int32 f = t->Fertility;

		if (f > -1) {
			fTile += f;
			count++;
		}
	}

	if (count > 0)
		fTile = (fTile + 1) / count;

	Tile->Fertility = FMath::Clamp(fTile + value, 1, 5);
}

TArray<FTileStruct*> AGrid::GenerateRiver(FTileStruct* Tile, FTileStruct* Peak)
{
	TArray<FTileStruct*> tiles;

	if (Tile->Level < 0)
		return tiles;

	tiles.Add(Tile);
	
	TArray<FTileStruct*> twoMostOuterTiles;

	for (auto& element : Tile->AdjacentTiles) {
		FTileStruct* t = element.Value;

		if (t->bRamp || t->Level > Tile->Level || t->bRiver)
			continue;

		if (twoMostOuterTiles.Num() < 2) {
			twoMostOuterTiles.Add(t);

			continue;
		}

		FTileStruct* chosenTile = t;

		for (FTileStruct* tile : twoMostOuterTiles) {
			float d1 = FVector2D::Distance(FVector2D(Peak->X, Peak->Y), FVector2D(chosenTile->X, chosenTile->Y));
			float d2 = FVector2D::Distance(FVector2D(Peak->X, Peak->Y), FVector2D(tile->X, tile->Y));

			if (d2 < d1)
				chosenTile = tile;
		}

		twoMostOuterTiles.Add(t);
		twoMostOuterTiles.Remove(chosenTile);
	}

	if (!twoMostOuterTiles.IsEmpty()) {
		int32 index = Stream.RandRange(0, twoMostOuterTiles.Num() - 1);
		FTileStruct* chosenTile = twoMostOuterTiles[index];

		chosenTile->bRiver = true;

		tiles.Append(GenerateRiver(chosenTile, Peak));
	}

	return tiles;
}

void AGrid::GenerateTile(FTileStruct* Tile)
{
	if (Tile->Level < 0)
		return;
	
	FTransform transform;
	FVector loc = FVector(Tile->X * 100.0f, Tile->Y * 100.0f, 0.0f);
	transform.SetLocation(loc);
	transform.SetRotation(Tile->Rotation);

	int32 inst;

	if (Tile->AdjacentTiles.Num() < 4) {
		FTransform t = transform;
		t.SetLocation(t.GetLocation() + FVector(0.0f, 0.0f, -200.0f));

		if (!Tile->AdjacentTiles.Find("Left")) {
			t.SetLocation(t.GetLocation() + FVector(-100.0f, 0.0f, 0.0f));
			HISMFlatGround->AddInstance(t);
		}

		if (!Tile->AdjacentTiles.Find("Right")) {
			t.SetLocation(t.GetLocation() + FVector(100.0f, 0.0f, 0.0f));
			HISMFlatGround->AddInstance(t);
		}

		if (!Tile->AdjacentTiles.Find("Below")) {
			t.SetLocation(t.GetLocation() + FVector(0.0f, -100.0f, 0.0f));
			HISMFlatGround->AddInstance(t);
		}

		if (!Tile->AdjacentTiles.Find("Above")) {
			t.SetLocation(t.GetLocation() + FVector(0.0f, 100.0f, 0.0f));
			HISMFlatGround->AddInstance(t);
		}
	}

	for (auto& element : Tile->AdjacentTiles) {
		if (element.Value->Level > -1)
			continue;

		FTransform t;
		t.SetLocation(FVector(element.Value->X * 100.0f, element.Value->Y * 100.0f, -200.0f));
		HISMFlatGround->AddInstance(t);
	}

	if (bLava && Tile->Level == MaxLevel) {
		transform.SetLocation(loc + FVector(0.0f, 0.0f, 75.0f * (MaxLevel - 2)));

		inst = HISMLava->AddInstance(transform);

		LavaSpawnLocations.Add(transform.GetLocation());
	}
	else if (Tile->bRiver) {
		transform.SetRotation(FRotator(0.0f).Quaternion());
		transform.SetLocation(loc + FVector(0.0f, 0.0f, 75.0f * Tile->Level));

		float r = 0.0f + (76.0f / 5.0f * (5.0f - Tile->Level));
		float g = 99.0f + (80.0f / 5.0f * (5.0f - Tile->Level));
		float b = 255.0f;

		r /= 255.0f;
		g /= 255.0f;
		b /= 255.0f;

		inst = HISMRiver->AddInstance(transform);
		HISMRiver->SetCustomDataValue(inst, 0, 1.0f);
		HISMRiver->SetCustomDataValue(inst, 1, r);
		HISMRiver->SetCustomDataValue(inst, 2, g);
		HISMRiver->SetCustomDataValue(inst, 3, b);

		FTransform tf;
		int32 num = Tile->Level - -1;
		int32 sign = 1;
		bool bOnYAxis = true;

		if (Tile->AdjacentTiles.Num() < 4) {
			TArray<FString> directionList;

			if (!Tile->AdjacentTiles.Contains("Above"))
				directionList.Add("Above");

			if (!Tile->AdjacentTiles.Contains("Below"))
				directionList.Add("Below");

			if (!Tile->AdjacentTiles.Contains("Left"))
				directionList.Add("Left");

			if (!Tile->AdjacentTiles.Contains("Right"))
				directionList.Add("Right");

			for (FString direction : directionList) {
				if (direction == "Below" || direction == "Right")
					sign = -1;

				if (direction == "Left" || direction == "Right")
					bOnYAxis = false;

				CreateWaterfall(transform.GetLocation(), num, sign, bOnYAxis, r, g, b);
			}
		}

		for (auto& element : Tile->AdjacentTiles) {
			FTileStruct* t = element.Value;

			if ((!t->bRiver && t->Level > 0) || t->Level >= Tile->Level)
				continue;

			num = Tile->Level - t->Level;
			sign = 1;
			bOnYAxis = true;

			if (t->Y < Tile->Y || t->X > Tile->X)
				sign = -1;

			if (t->Y == Tile->Y)
				bOnYAxis = false;

			CreateWaterfall(transform.GetLocation(), num, sign, bOnYAxis, r, g, b);
		}
	}
	else {
		transform.SetLocation(loc + FVector(0.0f, 0.0f, 75.0f * Tile->Level));

		float r = 0.0f;
		float g = 0.0f;
		float b = 0.0f;

		if (Tile->Fertility == 0) {
			r = 30.0f;
			g = 20.0f;
			b = 13.0f;
		}
		else if (Tile->Fertility == 1) {
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

		if (Tile->bEdge) {
			inst = HISMGround->AddInstance(transform);
			HISMGround->SetCustomDataValue(inst, 0, 0.0f);
			HISMGround->SetCustomDataValue(inst, 1, 1.0f);
			HISMGround->SetCustomDataValue(inst, 2, r);
			HISMGround->SetCustomDataValue(inst, 3, g);
			HISMGround->SetCustomDataValue(inst, 4, b);
		}
		else {
			int32 chance = Stream.RandRange(1, 100);

			for (auto& element : Tile->AdjacentTiles) {
				if (element.Value->Level > Tile->Level && element.Value->Level != MaxLevel && chance > 99) {
					Tile->bRamp = true;

					transform.SetRotation((FVector(Tile->X * 100.0f, Tile->Y * 100.0f, 0) - FVector(element.Value->X * 100.0f, element.Value->Y * 100.0f, 0)).ToOrientationQuat());
					transform.SetLocation(transform.GetLocation() + FVector(0.0f, 0.0f, 100.0f));

					break;
				}
			}

			if (Tile->bRamp) {
				inst = HISMRampGround->AddInstance(transform);
				HISMRampGround->SetCustomDataValue(inst, 0, 0.0f);
				HISMRampGround->SetCustomDataValue(inst, 1, 1.0f);
				HISMRampGround->SetCustomDataValue(inst, 2, r);
				HISMRampGround->SetCustomDataValue(inst, 3, g);
				HISMRampGround->SetCustomDataValue(inst, 4, b);
			}
			else {
				bool bFlat = true;

				if (Tile->AdjacentTiles.Num() < 4)
					bFlat = false;
				else
					for (auto& element : Tile->AdjacentTiles)
						if (element.Value->Level < Tile->Level || (element.Value->Level == MaxLevel && Tile->Level == (MaxLevel - 1)))
							bFlat = false;

				if (bFlat) {
					inst = HISMFlatGround->AddInstance(transform);
					HISMFlatGround->SetCustomDataValue(inst, 0, 0.0f);
					HISMFlatGround->SetCustomDataValue(inst, 1, 1.0f);
					HISMFlatGround->SetCustomDataValue(inst, 2, r);
					HISMFlatGround->SetCustomDataValue(inst, 3, g);
					HISMFlatGround->SetCustomDataValue(inst, 4, b);
				}
				else {
					inst = HISMGround->AddInstance(transform);
					HISMGround->SetCustomDataValue(inst, 0, 0.0f);
					HISMGround->SetCustomDataValue(inst, 1, 1.0f);
					HISMGround->SetCustomDataValue(inst, 2, r);
					HISMGround->SetCustomDataValue(inst, 3, g);
					HISMGround->SetCustomDataValue(inst, 4, b);

					Tile->bEdge = true;
				}
			}
		}
	}

	Tile->Instance = inst;
}

void AGrid::CreateWaterfall(FVector Location, int32 Num, int32 Sign, bool bOnYAxis, float R, float G, float B)
{
	for (int32 i = 0; i < Num; i++) {
		FTransform transform;

		if (bOnYAxis) {
			transform.SetRotation(FRotator(0.0f, 0.0f, 90.0f * Sign).Quaternion());
			transform.SetLocation(Location + FVector(0.0f, 30.0f * Sign * -1, 30.0f - (100.0f * i)));
		}
		else {
			transform.SetRotation(FRotator(90.0f * Sign, 0.0f, 0.0f).Quaternion());
			transform.SetLocation(Location + FVector(30.0f * Sign, 0.0f, 30.0f - (100.0f * i)));
		}

		int32 wfInst = HISMRiver->AddInstance(transform);
		HISMRiver->SetCustomDataValue(wfInst, 0, 1.0f);
		HISMRiver->SetCustomDataValue(wfInst, 1, R);
		HISMRiver->SetCustomDataValue(wfInst, 2, G);
		HISMRiver->SetCustomDataValue(wfInst, 3, B);
	}
}

void AGrid::GenerateMinerals(FTileStruct* Tile, AResource* Resource)
{
	FTransform transform = GetTransform(Tile);
	
	int32 inst = Resource->ResourceHISM->AddInstance(transform);
	Resource->ResourceHISM->SetCustomDataValue(inst, 0, 0.0f);

	FString socketName = "InfoSocket";
	socketName.AppendInt(inst);

	UStaticMeshSocket* socket = NewObject<UStaticMeshSocket>(Resource);
	socket->SocketName = *socketName;
	socket->RelativeLocation = transform.GetLocation() + FVector(0.0f, 0.0f, Resource->ResourceHISM->GetStaticMesh()->GetBounds().GetBox().GetSize().Z);

	Resource->ResourceHISM->GetStaticMesh()->AddSocket(socket);

	ResourceTiles.Remove(Tile);
}

void AGrid::GetValidSpawnLocations(FTileStruct* SpawnTile, FTileStruct* CheckTile, int32 Range, bool& Valid, TArray<FTileStruct*>& Tiles)
{
	if (CheckTile->X > SpawnTile->X + Range || CheckTile->X < SpawnTile->X - Range || CheckTile->Y > SpawnTile->Y + Range || CheckTile->Y < SpawnTile->Y - Range)
		return;

	if (CheckTile->bRamp || CheckTile->bRiver || CheckTile->Level != SpawnTile->Level) {
		Valid = false;

		return;
	}

	Tiles.Add(CheckTile);

	for (auto& element : CheckTile->AdjacentTiles)
		if (!Tiles.Contains(element.Value))
			GetValidSpawnLocations(SpawnTile, element.Value, Range, Valid, Tiles);
}

void AGrid::GenerateVegetation(TArray<FResourceHISMStruct> Vegetation, FTileStruct* StartingTile, FTileStruct* Tile, int32 Amount, float Scale, bool bTree)
{
	if (FMath::Abs(StartingTile->X - Tile->X) + FMath::Abs(StartingTile->Y - Tile->Y) > 5)
		Amount = Stream.RandRange(Amount - 1, Amount);

	if (Amount == 0 || VegetationLimitCounter == VegetationLimit || Tile->Fertility == 0 || !ResourceTiles.Contains(Tile) || VegetationTiles.Contains(Tile) || Tile->bRiver || Tile->Level < 0 || GetTransform(Tile).GetLocation().Z < 0.0f)
		return;

	TArray<int32> usedX;
	TArray<int32> usedY;

	int32 num = Amount;

	if (!bTree)
		num *= Stream.RandRange(10, 20);

	for (int32 i = 0; i < num; i++) {
		bool validXY = false;
		int32 x = 0;
		int32 y = 0;

		while (!validXY) {
			x = Stream.RandRange(-40, 40);
			y = Stream.RandRange(-40, 40);

			if (!usedX.Contains(x) && !usedY.Contains(y))
				validXY = true;
		}

		FTransform transform;
		transform.SetLocation(GetTransform(Tile).GetLocation() + FVector(x, y, 0.0f));

		float size = Stream.FRandRange(1.0f / Scale, Scale);
		transform.SetScale3D(FVector(size));

		transform.SetRotation(FRotator(0.0f, Stream.RandRange(0, 360), 0.0f).Quaternion());

		int32 index = Stream.RandRange(0, Vegetation.Num() - 1);

		AVegetation* resource = Cast<AVegetation>(Vegetation[index].Resource);

		int32 inst = resource->ResourceHISM->AddInstance(transform);

		resource->ResourceHISM->SetCustomDataValue(inst, 9, size);

		if (bTree)
			GenerateTree(resource, inst);
		else
			GenerateFlower(Tile, resource, inst);
	}

	VegetationTiles.Add(Tile);

	VegetationLimitCounter++;

	for (auto& element : Tile->AdjacentTiles)
		GenerateVegetation(Vegetation, StartingTile, element.Value, Amount, Scale, bTree);
}

void AGrid::GenerateTree(AVegetation* Resource, int32 Instance)
{
	float r = 0.0f;
	float g = 0.0f;
	float b = 0.0f;

	int32 colour = Stream.RandRange(0, 3);

	if (colour == 0) {
		r = 53.0f;
		g = 90.0f;
		b = 32.0f;
	}
	else if (colour == 1) {
		r = 54.0f;
		g = 79.0f;
		b = 38.0f;
	}
	else if (colour == 2) {
		r = 32.0f;
		g = 90.0f;
		b = 40.0f;
	}
	else {
		r = 38.0f;
		g = 79.0f;
		b = 43.0f;
	}

	int32 dyingChance = Stream.RandRange(0, 100);

	if (dyingChance >= 97) {
		r = 82.0f;
		g = 90.0f;
		b = 32.0f;
	}

	if (dyingChance == 100)
		Resource->ResourceHISM->SetCustomDataValue(Instance, 10, 1.0f);
	else
		Resource->ResourceHISM->SetCustomDataValue(Instance, 10, 0.0f);

	r /= 255.0f;
	g /= 255.0f;
	b /= 255.0f;

	Resource->ResourceHISM->SetCustomDataValue(Instance, 0, 0.0f);
	Resource->ResourceHISM->SetCustomDataValue(Instance, 1, 1.0f);
	Resource->ResourceHISM->SetCustomDataValue(Instance, 2, r);
	Resource->ResourceHISM->SetCustomDataValue(Instance, 3, g);
	Resource->ResourceHISM->SetCustomDataValue(Instance, 4, b);
}

void AGrid::GenerateFlower(FTileStruct* Tile, AVegetation* Resource, int32 Instance)
{
	UHierarchicalInstancedStaticMeshComponent* HISM = HISMFlatGround;

	if (Tile->bEdge)
		HISM = HISMGround;

	float r = Stream.FRandRange(0.0f, 1.0f);;
	float g = Stream.FRandRange(0.0f, 1.0f);
	float b = Stream.FRandRange(0.0f, 1.0f);

	Resource->ResourceHISM->SetCustomDataValue(Instance, 0, 0.0f);
	Resource->ResourceHISM->SetCustomDataValue(Instance, 1, 1.0f);
	Resource->ResourceHISM->SetCustomDataValue(Instance, 2, r);
	Resource->ResourceHISM->SetCustomDataValue(Instance, 3, g);
	Resource->ResourceHISM->SetCustomDataValue(Instance, 4, b);
}

void AGrid::RemoveTree(AResource* Resource, int32 Instance)
{
	int32 lastInstance = Resource->ResourceHISM->GetInstanceCount() - 1;

	FWorkerStruct workerStruct;
	workerStruct.Instance = lastInstance;

	int32 index = Resource->WorkerStruct.Find(workerStruct);

	if (index != INDEX_NONE) {
		Resource->WorkerStruct[index].Instance = Instance;

		for (ACitizen* citizen : Resource->WorkerStruct[index].Citizens) {
			citizen->AIController->MoveRequest.SetGoalInstance(Instance);

			break;
		}
	}
	
	Resource->ResourceHISM->RemoveInstance(Instance);
}

void AGrid::SpawnEggBasket()
{
	int32 index = Stream.RandRange(0, ResourceTiles.Num() - 1);

	if (index == INDEX_NONE)
		return;

	FTransform transform = GetTransform(ResourceTiles[index]);

	AEggBasket* eggBasket = GetWorld()->SpawnActor<AEggBasket>(EggBasketClass, transform.GetLocation(), transform.Rotator());
	eggBasket->Grid = this;
	eggBasket->Tile = ResourceTiles[index];

	ResourceTiles.RemoveAt(index);
}

FTransform AGrid::GetTransform(FTileStruct* Tile)
{
	FTransform transform;
	
	if (Tile->bRiver)
		HISMRiver->GetInstanceTransform(Tile->Instance, transform);
	else if (Tile->bRamp)
		HISMRampGround->GetInstanceTransform(Tile->Instance, transform);
	else if(Tile->bEdge)
		HISMGround->GetInstanceTransform(Tile->Instance, transform);
	else
		HISMFlatGround->GetInstanceTransform(Tile->Instance, transform);

	transform.SetLocation(transform.GetLocation() + FVector(0.0f, 0.0f, 100.0f));

	return transform;
}

void AGrid::Clear()
{
	TArray<FResourceHISMStruct> resourceList;
	resourceList.Append(TreeStruct);
	resourceList.Append(FlowerStruct);
	resourceList.Append(MineralStruct);
	
	for (FResourceHISMStruct& ResourceStruct : resourceList) {
		ResourceStruct.Resource->ResourceHISM->ClearInstances();

		ResourceStruct.Resource->ResourceHISM->GetStaticMesh()->Sockets.Empty();
	}

	Storage.Empty();

	LavaComponent->Deactivate();
	LavaSpawnLocations.Empty();

	CloudComponent->Clear();

	GetWorld()->GetTimerManager().ClearTimer(EggBasketTimer);

	TArray<AActor*> baskets;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEggBasket::StaticClass(), baskets);

	for (AActor* actor : baskets)
		actor->Destroy();

	HISMLava->ClearInstances();
	HISMGround->ClearInstances();
	HISMFlatGround->ClearInstances();
	HISMRampGround->ClearInstances();
	HISMRiver->ClearInstances();

	if (Camera->PauseUIInstance->IsInViewport())
		Camera->SetPause(false, false);
}

void AGrid::SetSeasonAffect(FString Period, float Increment)
{
	if (Period == "Winter")
		CloudComponent->bSnow = true;
	else
		CloudComponent->bSnow = false;

	AlterSeasonAffectGradually(Period, Increment);

	CloudComponent->UpdateSpawnedClouds();
}

void AGrid::AlterSeasonAffectGradually(FString Period, float Increment)
{
	Async(EAsyncExecution::Thread, [this, Period, Increment]() {
		TArray<float> Values;
		Values.Add(HISMGround->PerInstanceSMCustomData[5]);
		Values.Add(HISMGround->PerInstanceSMCustomData[6]);
		Values.Add(HISMGround->PerInstanceSMCustomData[7]);

		if (Period == "Spring")
			Values[0] = FMath::Clamp(Values[0] + Increment, 0.0f, 1.0f);
		else
			Values[0] = FMath::Clamp(Values[0] - Increment, 0.0f, 1.0f);

		if (Period == "Autumn")
			Values[1] = FMath::Clamp(Values[1] + Increment, 0.0f, 1.0f);
		else
			Values[1] = FMath::Clamp(Values[1] - Increment, 0.0f, 1.0f);

		if (Period == "Winter")
			Values[2] = FMath::Clamp(Values[2] + Increment, 0.0f, 1.0f);
		else
			Values[2] = FMath::Clamp(Values[2] - Increment, 0.0f, 1.0f);

		SetSeasonAffect(Values);

		Values.Remove(0.0f);

		AsyncTask(ENamedThreads::GameThread, [this, Values, Period, Increment]() {
			TArray<FResourceHISMStruct> resourceList;
			resourceList.Append(TreeStruct);
			resourceList.Append(FlowerStruct);

			for (FResourceHISMStruct& ResourceStruct : resourceList)
				ResourceStruct.Resource->ResourceHISM->BuildTreeIfOutdated(true, true);

			HISMGround->BuildTreeIfOutdated(true, true);
			HISMFlatGround->BuildTreeIfOutdated(true, true);
			HISMRampGround->BuildTreeIfOutdated(true, true);

			if (Values.Contains(1.0f) || Values.IsEmpty())
				return;

			FTimerHandle seasonChangeTimer;
			GetWorldTimerManager().SetTimer(seasonChangeTimer, FTimerDelegate::CreateUObject(this, &AGrid::AlterSeasonAffectGradually, Period, Increment), 0.02f, false);
		});
	});
}

void AGrid::SetSeasonAffect(TArray<float> Values)
{
	TArray<FResourceHISMStruct> resourceList;
	resourceList.Append(TreeStruct);
	resourceList.Append(FlowerStruct);

	for (FResourceHISMStruct& ResourceStruct : resourceList) {
		for (int32 inst = 0; inst < ResourceStruct.Resource->ResourceHISM->GetInstanceCount(); inst++) {
			ResourceStruct.Resource->ResourceHISM->PerInstanceSMCustomData[inst * 11 + 5] = Values[0];
			ResourceStruct.Resource->ResourceHISM->PerInstanceSMCustomData[inst * 11 + 6] = Values[1];
			ResourceStruct.Resource->ResourceHISM->PerInstanceSMCustomData[inst * 11 + 7] = Values[2];
		}
	}

	for (int32 inst = 0; inst < HISMGround->GetInstanceCount(); inst++) {
		FTransform transform;
		HISMGround->GetInstanceTransform(inst, transform);

		auto bound = FMath::FloorToInt32(FMath::Sqrt((double)Size));

		int32 x = transform.GetLocation().X / 100.0f + (bound / 2);
		int32 y = transform.GetLocation().Y / 100.0f + (bound / 2);

		if (Storage[x][y].Fertility == 0.0f)
			continue;

		HISMGround->PerInstanceSMCustomData[inst * 8 + 5] = Values[0];
		HISMGround->PerInstanceSMCustomData[inst * 8 + 6] = Values[1];
		HISMGround->PerInstanceSMCustomData[inst * 8 + 7] = Values[2];
	}

	for (int32 inst = 0; inst < HISMFlatGround->GetInstanceCount(); inst++) {
		FTransform transform;
		HISMFlatGround->GetInstanceTransform(inst, transform);

		auto bound = FMath::FloorToInt32(FMath::Sqrt((double)Size));

		int32 x = transform.GetLocation().X / 100.0f + (bound / 2);
		int32 y = transform.GetLocation().Y / 100.0f + (bound / 2);

		if (x < 0 || x > (bound - 1) || y < 0 || y > (bound - 1) || Storage[x][y].Fertility == 0.0f)
			continue;

		HISMFlatGround->PerInstanceSMCustomData[inst * 8 + 5] = Values[0];
		HISMFlatGround->PerInstanceSMCustomData[inst * 8 + 6] = Values[1];
		HISMFlatGround->PerInstanceSMCustomData[inst * 8 + 7] = Values[2];
	}

	for (int32 inst = 0; inst < HISMRampGround->GetInstanceCount(); inst++) {
		HISMRampGround->PerInstanceSMCustomData[inst * 8 + 5] = Values[0];
		HISMRampGround->PerInstanceSMCustomData[inst * 8 + 6] = Values[1];
		HISMRampGround->PerInstanceSMCustomData[inst * 8 + 7] = Values[2];
	}
}