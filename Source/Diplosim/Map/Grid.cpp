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
#include "NavigationSystem.h"

#include "AIVisualiser.h"
#include "Atmosphere/Clouds.h"
#include "Resources/Mineral.h"
#include "Resources/Vegetation.h"
#include "Atmosphere/AtmosphereComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Components/CameraMovementComponent.h"
#include "Player/Components/BuildComponent.h"
#include "Universal/EggBasket.h"
#include "Universal/DiplosimUserSettings.h"
#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Buildings/Misc/Special/Special.h"

AGrid::AGrid()
{
	PrimaryActorTick.bCanEverTick = false;

	FCollisionResponseContainer response;
	response.GameTraceChannel1 = ECR_Block;
	response.WorldStatic = ECR_Ignore;
	response.WorldDynamic = ECR_Overlap;
	response.Pawn = ECR_Ignore;
	response.PhysicsBody = ECR_Ignore;
	response.Vehicle = ECR_Block;
	response.Destructible = ECR_Ignore;
	response.GameTraceChannel2 = ECR_Ignore;
	response.GameTraceChannel3 = ECR_Ignore;
	response.GameTraceChannel4 = ECR_Ignore;

	WorldContainer = CreateDefaultSubobject<USceneComponent>(TEXT("WorldContainer"));
	WorldContainer->SetMobility(EComponentMobility::Static);
	RootComponent = WorldContainer;

	TMap<UHierarchicalInstancedStaticMeshComponent**, FName> hisms;
	hisms.Add(&HISMLava, TEXT("HISMLava"));
	hisms.Add(&HISMSea, TEXT("HISMSea"));
	hisms.Add(&HISMGround, TEXT("HISMGround"));
	hisms.Add(&HISMFlatGround, TEXT("HISMFlatGround"));
	hisms.Add(&HISMRampGround, TEXT("HISMRampGround"));
	hisms.Add(&HISMRiver, TEXT("HISMRiver"));

	for (auto& element : hisms) {
		auto hism = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(element.Value);
		*element.Key = hism;
		hism->SetupAttachment(GetRootComponent());
		hism->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		hism->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
		hism->SetCollisionResponseToChannels(response);
		hism->SetGenerateOverlapEvents(false);
		hism->bAutoRebuildTreeOnInstanceChanges = false;
		hism->bWorldPositionOffsetWritesVelocity = false;

		bool bwpo = false;
		if (hism == HISMSea || hism == HISMRiver)
			bwpo = true;

		hism->SetEvaluateWorldPositionOffset(bwpo);

		if (hism == HISMLava || hism == HISMSea || hism == HISMRiver)
			hism->SetCanEverAffectNavigation(false);

		if (hism == HISMLava || hism == HISMFlatGround)
			hism->SetCastShadow(false);

		if (hism == HISMGround || hism == HISMFlatGround || hism == HISMRampGround)
			hism->NumCustomDataFloats = 8;
		else if (hism == HISMRiver)
			hism->NumCustomDataFloats = 4;
	}

	HISMGround->ShadowCacheInvalidationBehavior = EShadowCacheInvalidationBehavior::Always;

	HISMRiver->SetWorldPositionOffsetDisableDistance(5000);

	LavaComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("LavaComponent"));
	LavaComponent->SetupAttachment(GetRootComponent());
	LavaComponent->SetAutoActivate(false);

	AtmosphereComponent = CreateDefaultSubobject<UAtmosphereComponent>(TEXT("AtmosphereComponent"));
	AtmosphereComponent->WindComponent->SetupAttachment(GetRootComponent());

	AIVisualiser = CreateDefaultSubobject<UAIVisualiser>(TEXT("AIVisualiser"));
	AIVisualiser->AIContainer->SetupAttachment(GetRootComponent());

	CrystalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrystalMesh"));
	CrystalMesh->SetCollisionProfileName("NoCollision", false);
	CrystalMesh->SetComponentTickEnabled(false);
	CrystalMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -1000.0f));
	CrystalMesh->SetupAttachment(GetRootComponent());

	Size = 22500;
	Chunks = 1;
	Peaks = 2;
	Rivers = 2;
	Type = EType::Round;
	bLava = true;
	MaxLevel = 7;

	PercentageGround = 30;

	Seed = "";

	VegetationMinDensity = 1;
	VegetationMaxDensity = 3;
	VegetationMultiplier = 1;
	VegetationSizeMultiplier = 5;

	bRandSpecialBuildings = true;

	TreeColours = { FLinearColor(53.0f, 90.0f, 32.0f), FLinearColor(54.0f, 79.0f, 38.0f), FLinearColor(32.0f, 90.0f, 40.0f), FLinearColor(38.0f, 79.0f, 43.0f), FLinearColor(82.0f, 90.0f, 32.0f) };
	GroundColours = { FLinearColor(30.0f, 20.0f, 13.0f), FLinearColor(255.0f, 225.0f, 45.0f), FLinearColor(152.0f, 192.0f, 100.0f), FLinearColor(86.0f, 228.0f, 68.0f), FLinearColor(52.0f, 213.0f, 31.0f), FLinearColor(36.0f, 146.0f, 21.0f) };
}

void AGrid::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Camera = PController->GetPawn<ACamera>();

	MapUIInstance = CreateWidget<UUserWidget>(PController, MapUI);

	LoadUIInstance = CreateWidget<UUserWidget>(PController, LoadUI);

	Camera->Grid = this;

	AtmosphereComponent->ChangeWindDirection();

	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();
	HISMRiver->SetWorldPositionOffsetDisableDistance(settings->GetWPODistance());

	for (FResourceHISMStruct& ResourceStruct : TreeStruct) {
		ResourceStruct.Resource = GetWorld()->SpawnActor<AResource>(ResourceStruct.ResourceClass, FVector::Zero(), FRotator(0.0f));
		ResourceStruct.Resource->ResourceHISM->SetWorldPositionOffsetDisableDistance(settings->GetWPODistance());
	}

	for (FResourceHISMStruct& ResourceStruct : FlowerStruct) {
		ResourceStruct.Resource = GetWorld()->SpawnActor<AResource>(ResourceStruct.ResourceClass, FVector::Zero(), FRotator(0.0f));
		ResourceStruct.Resource->ResourceHISM->SetWorldPositionOffsetDisableDistance(settings->GetWPODistance());
	}

	for (FResourceHISMStruct& ResourceStruct : MineralStruct)
		ResourceStruct.Resource = GetWorld()->SpawnActor<AResource>(ResourceStruct.ResourceClass, FVector::Zero(), FRotator(0.0f));

	for (TSubclassOf<ASpecial> specialClass : SpecialBuildingClasses)
		SpecialBuildings.Add(GetWorld()->SpawnActor<ASpecial>(specialClass, FVector::Zero(), FRotator(0.0f)));

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	FScriptDelegate delegate;
	delegate.BindUFunction(this, TEXT("OnNavMeshGenerated"));

	nav->OnNavigationGenerationFinishedDelegate.Add(delegate);
}

int32 AGrid::GetMapBounds()
{
	return FMath::FloorToInt32(FMath::Sqrt((double)Size)) * Chunks;
}

void AGrid::Load()
{
	LoadUIInstance->AddToViewport();

	Camera->Stream.Initialize(*Seed);

	FString s = Seed;

	if (s == "")
		s = FString::FromInt(Camera->Stream.GetCurrentSeed());

	Camera->UpdateMapSeed(s);

	Seed = "";

	Camera->UpdateLoadingText("Initialising Map");

	FTimerHandle RenderTimer;
	GetWorld()->GetTimerManager().SetTimer(RenderTimer, this, &AGrid::SetupMap, 0.001, false);
}

void AGrid::InitialiseStorage()
{
	auto bound = GetMapBounds();

	Storage.Empty();

	for (int32 x = 0; x < bound; x++) {
		auto& row = Storage.Emplace_GetRef();

		for (int32 y = 0; y < bound; y++) {
			FTileStruct tile;

			tile.X = x - (bound / 2);
			tile.Y = y - (bound / 2);

			row.Add(tile);
		}
	}

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
}

void AGrid::SetupMap()
{
	FTransform seaTransform;
	seaTransform.SetLocation(FVector(0.0f, 0.0f, 50.0f));
	HISMSea->AddInstance(seaTransform);
	HISMSea->BuildTreeIfOutdated(true, true);

	auto bound = GetMapBounds();

	InitialiseStorage();

	Camera->ConquestManager->AINum = Storage.Num() * PercentageGround / 5000;
	Camera->UpdateMapAIUI();

	PeaksList.Empty();

	TArray<FTileStruct*> chooseableTiles = {};

	int32 levelTotal = Size * (PercentageGround / 100.0f) * Chunks;

	int32 level = MaxLevel + 1;

	int32 levelCount = 0;

	float totalPercentage = 0.0f;

	for (int32 i = MaxLevel; i > -1; i--) {
		int32 height = i;

		if (Type == EType::Mountain)
			height = (MaxLevel / FMath::Min(FMath::Max(1, i) * 2.0f, MaxLevel));

		float percentage = (1.85f / FMath::Sqrt(2.0f * PI * FMath::Square(MaxLevel / 2.5f)) * FMath::Exp(-1.0f * (FMath::Square(height) / (1.85f * FMath::Square(MaxLevel / 2.5f)))));

		totalPercentage += percentage;
	}

	while (true) {
		if (levelCount <= 0) {
			level--;

			if (level < 0)
				break;

			int32 height = level;

			if (Type == EType::Mountain)
				height = (MaxLevel / FMath::Min(FMath::Max(1, level) * 2.0f, MaxLevel));

			float percentage = (1.85f / FMath::Sqrt(2.0f * PI * FMath::Square(MaxLevel / 2.5f)) * FMath::Exp(-1.0f * (FMath::Square(height) / (1.85f * FMath::Square(MaxLevel / 2.5f))))) / totalPercentage;
			levelCount = FMath::Clamp(percentage * levelTotal, 0, levelTotal);
		}

		FTileStruct* chosenTile = nullptr;

		if (PeaksList.Num() < Peaks * Chunks) {
			int32 range = bound / 4;

			int32 chosenX = Camera->Stream.RandRange(range, bound - 1 - range);
			int32 chosenY = Camera->Stream.RandRange(range, bound - 1 - range);

			chosenTile = &Storage[chosenX][chosenY];

			PeaksList.Add(chosenTile);

			if (PeaksList.Num() == 2 && Type == EType::Mountain) {
				float distance = FVector2D::Distance(FVector2D(PeaksList[0]->X, PeaksList[0]->Y), FVector2D(PeaksList[1]->X, PeaksList[1]->Y));

				FVector2D p0 = FVector2D::Zero();
				FVector2D p1 = FVector2D::Zero();
				FVector2D p2 = FVector2D::Zero();
				FVector2D p3 = FVector2D::Zero();

				if (PeaksList[0]->Y > PeaksList[1]->Y) {
					p0 = FVector2D(PeaksList[1]->X, PeaksList[1]->Y);
					p3 = FVector2D(PeaksList[0]->X, PeaksList[0]->Y);
				}
				else {
					p0 = FVector2D(PeaksList[0]->X, PeaksList[0]->Y);
					p3 = FVector2D(PeaksList[1]->X, PeaksList[1]->Y);
				}

				FVector2D pHalf = (p0 + p3) / 2.0f;

				float variance = distance * 0.66f;

				p1 = FVector2D(Camera->Stream.RandRange(p0.X, pHalf.X) + Camera->Stream.RandRange(-variance, variance), Camera->Stream.RandRange(p0.Y, pHalf.Y) + Camera->Stream.RandRange(-variance, variance));
				p2 = FVector2D(Camera->Stream.RandRange(pHalf.X, p3.X) + Camera->Stream.RandRange(-variance, variance), Camera->Stream.RandRange(pHalf.Y, p3.Y) + Camera->Stream.RandRange(-variance, variance));

				double t = 0.0f;

				while (t <= 1.0f) {
					FVector2D point = FMath::Pow((1 - t), 3) * p0 + 3 * FMath::Pow((1 - t), 2) * t * p1 + 3 * (1 - t) * FMath::Pow(t, 2) * p2 + FMath::Pow(t, 3) * p3;

					int32 px = point.X + (bound / 2);
					int32 py = point.Y + (bound / 2);

					FTileStruct* tile = &Storage[px][py];

					if (tile->Level > -1) {
						t += 0.01f;

						continue;
					}

					for (auto& element : tile->AdjacentTiles) {
						element.Value->Level = level;

						levelCount--;

						chooseableTiles.Remove(element.Value);

						PeaksList.Add(element.Value);

						for (auto& e : element.Value->AdjacentTiles)
							if (!chooseableTiles.Contains(e.Value) && e.Value->Level < 0)
								chooseableTiles.Add(e.Value);
					}

					tile->Level = level;

					levelCount--;

					chooseableTiles.Remove(tile);

					PeaksList.Add(tile);

					t += 0.01f;
				}
			}
		}
		else {
			if (chooseableTiles.IsEmpty())
				break;

			int32 chosenNum = Camera->Stream.RandRange(0, chooseableTiles.Num() - 1);
			chosenTile = chooseableTiles[chosenNum];

			for (FTileStruct* tile : chooseableTiles) {
				int32 distance = 1000000000;

				for (FTileStruct* peak : PeaksList) {
					int32 d = FVector2D::Distance(FVector2D(peak->X, peak->Y), FVector2D(tile->X, tile->Y));

					if (distance > d)
						distance = d;
				}

				int32 chosenDistance = 1000000000;

				for (FTileStruct* peak : PeaksList) {
					int32 d = FVector2D::Distance(FVector2D(peak->X, peak->Y), FVector2D(chosenTile->X, chosenTile->Y));

					if (chosenDistance > d)
						chosenDistance = d;
				}

				if (chosenDistance > distance + 20)
					chosenTile = tile;
			}
		}

		chosenTile->Level = level;

		for (auto& element : chosenTile->AdjacentTiles)
			if (!chooseableTiles.Contains(element.Value) && element.Value->Level < 0)
				chooseableTiles.Add(element.Value);

		chooseableTiles.Remove(chosenTile);

		levelCount--;
	}

	Camera->UpdateLoadingText("Cleaning Up Holes");

	FTimerHandle RenderTimer;
	GetWorld()->GetTimerManager().SetTimer(RenderTimer, this, &AGrid::CleanupHoles, 0.001, false);
}

void AGrid::CleanupHoles()
{
	for (TArray<FTileStruct>& row : Storage)
		for (FTileStruct& tile : row)
			FillHoles(&tile);

	Camera->UpdateLoadingText("Removing Pocket Seas");

	FTimerHandle RenderTimer;
	GetWorld()->GetTimerManager().SetTimer(RenderTimer, this, &AGrid::RemovePocketSeas, 0.001, false);
}

void AGrid::RemovePocketSeas()
{
	for (TArray<FTileStruct>& row : Storage) {
		for (FTileStruct& tile : row) {
			SeaTiles.Empty();

			GetSeaTiles(&tile);

			if (SeaTiles.Num() > 32)
				continue;

			for (FTileStruct* t : SeaTiles)
				t->Level = 0;
		}
	}

	Camera->UpdateLoadingText("Populating Tile Information");

	FTimerHandle RenderTimer;
	GetWorld()->GetTimerManager().SetTimer(RenderTimer, this, &AGrid::SetupTileInformation, 0.001, false);
}

void AGrid::SetupTileInformation()
{
	for (TArray<FTileStruct>& row : Storage)
		for (FTileStruct& tile : row)
			SetTileDetails(&tile);

	Camera->UpdateLoadingText("Paving Rivers");

	FTimerHandle RenderTimer;
	GetWorld()->GetTimerManager().SetTimer(RenderTimer, this, &AGrid::PaveRivers, 0.001, false);
}

void AGrid::PaveRivers()
{
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

		int32 chosenNum = Camera->Stream.RandRange(0, riverStartTiles.Num() - 1);
		FTileStruct* chosenTile = riverStartTiles[chosenNum];

		FTileStruct* closestPeak = nullptr;

		for (FTileStruct* peak : PeaksList) {
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

	Camera->UpdateLoadingText("Spawning Tiles");

	FTimerHandle RenderTimer;
	GetWorld()->GetTimerManager().SetTimer(RenderTimer, this, &AGrid::SpawnTiles, 0.001, false);
}

void AGrid::SpawnTiles()
{
	ResourceTiles.Empty();
	CalculatedTiles.Empty();

	for (TArray<FTileStruct>& row : Storage) {
		for (FTileStruct& tile : row) {
			CalculateTile(&tile);

			if (tile.Level < 0 || tile.Level > 4 || tile.bRiver || tile.bRamp)
				continue;

			ResourceTiles.Add(&tile);
		}
	}

	GenerateTiles();

	Camera->UpdateLoadingText("Spawning Minerals");

	FTimerHandle RenderTimer;
	GetWorld()->GetTimerManager().SetTimer(RenderTimer, this, &AGrid::SpawnMinerals, 0.001, false);
}

void AGrid::SpawnMinerals()
{
	int32 num = ResourceTiles.Num() / 2000;

	ValidMineralTiles.Empty();

	for (FTileStruct* tile : ResourceTiles) {
		bool valid = true;
		TArray<FTileStruct*> tiles;

		GetValidSpawnLocations(tile, tile, 1, valid, tiles);

		if (!valid)
			continue;

		ValidMineralTiles.Add(tiles);
	}

	for (FResourceHISMStruct& ResourceStruct : MineralStruct) {
		for (int32 i = 0; i < (num * ResourceStruct.Multiplier); i++) {
			int32 chosenNum = Camera->Stream.RandRange(0, ValidMineralTiles.Num() - 1);
			TArray<FTileStruct*> chosenTiles = ValidMineralTiles[chosenNum];

			GenerateMinerals(chosenTiles[0], ResourceStruct.Resource);

			for (FTileStruct* tile : chosenTiles) {
				tile->bMineral = true;

				for (int32 j = ValidMineralTiles.Num() - 1; j > -1; j--)
					if (ValidMineralTiles[j].Contains(tile))
						ValidMineralTiles.RemoveAt(j);
			}

			ResourceStruct.Resource->ResourceHISM->BuildTreeIfOutdated(true, true);
		}
	}

	GenerateTiles();

	Camera->UpdateLoadingText("Spawning Vegetation");

	FTimerHandle RenderTimer;
	GetWorld()->GetTimerManager().SetTimer(RenderTimer, this, &AGrid::SpawnVegetation, 0.001, false);
}

void AGrid::SpawnVegetation()
{
	int32 num = ResourceTiles.Num() / 2000;

	TMap<int32, TArray<FResourceHISMStruct>> vegetation;
	vegetation.Add(2, FlowerStruct);
	vegetation.Add(5, TreeStruct);

	for (auto& element : vegetation) {
		int32 iterations = num * VegetationMultiplier;
		int32 min = VegetationMinDensity;
		int32 max = VegetationMaxDensity;

		if (element.Key == 2) {
			iterations = FMath::Max(1.0f, iterations / 3.0f);
			min = FMath::Max(1.0f, VegetationMinDensity / 3.0f);
			max = FMath::Max(1.0f, VegetationMinDensity / 3.0f);
		}

		for (int32 i = 0; i < iterations; i++) {
			int32 chosenNum = Camera->Stream.RandRange(0, ResourceTiles.Num() - 1);
			FTileStruct* chosenTile = ResourceTiles[chosenNum];

			int32 amount = Camera->Stream.RandRange(min, max);

			bool bTree = false;

			int32 scale = 1.5f;

			if (element.Key == 5) {
				bTree = true;

				scale = 1.1f;
			}

			VegetationTiles.Empty();

			GenerateVegetation(element.Value, chosenTile, chosenTile, amount, scale, bTree);

			if (bTree)
				for (FTileStruct* tile : VegetationTiles)
					ResourceTiles.Remove(tile);
		}

		for (FResourceHISMStruct ResourceStruct : element.Value)
			ResourceStruct.Resource->ResourceHISM->BuildTreeIfOutdated(true, true);
	}

	GenerateTiles();

	Camera->UpdateLoadingText("Setting Up Environment");

	FTimerHandle RenderTimer;
	GetWorld()->GetTimerManager().SetTimer(RenderTimer, FTimerDelegate::CreateUObject(this, &AGrid::SetupEnvironment, false), 0.001, false);
}

void AGrid::SetupEnvironment(bool bLoad)
{
	auto bound = GetMapBounds();

	AtmosphereComponent->SetSeasonAffect(AtmosphereComponent->Calendar.Period, 1.0f);
	AtmosphereComponent->SetWindDimensions(bound);

	FVector c1 = FVector(bound * 100.0f, bound * 100.0f, 0);
	FVector c2 = FVector(-bound * 100.0f, -bound * 100.0f, 0);

	Camera->MovementComponent->SetBounds(c1, c2);

	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(LavaComponent, TEXT("SpawnLocations"), LavaSpawnLocations);
	LavaComponent->SetVariableFloat(TEXT("SpawnRate"), LavaSpawnLocations.Num() / 10.0f);
	LavaComponent->Activate();

	if (bLoad)
		return;

	AtmosphereComponent->Clouds->ActivateCloud();

	SpawnEggBasket();

	SetSpecialBuildings(ValidMineralTiles);

	if (Camera->PauseUIInstance->IsInViewport())
		Camera->SetPause(true, false);
}

void AGrid::OnNavMeshGenerated()
{
	LoadUIInstance->RemoveFromParent();
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

	Tile->Level = result;
}

void AGrid::SetTileDetails(FTileStruct* Tile)
{
	int32 rand = Camera->Stream.RandRange(0, 3);

	Tile->Rotation = (FRotator(0.0f, 90.0f, 0.0f) * rand).Quaternion();

	if ((bLava && Tile->Level == MaxLevel) || Tile->Level < 0)
		return;

	int32 value = Camera->Stream.RandRange(-1, 1);

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

void AGrid::GetSeaTiles(FTileStruct* Tile) 
{
	if (SeaTiles.Contains(Tile) || Tile->Level > -1 || SeaTiles.Num() > 32)
		return;

	SeaTiles.Add(Tile);

	for (auto& element : Tile->AdjacentTiles)
		GetSeaTiles(element.Value);
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
		int32 index = Camera->Stream.RandRange(0, twoMostOuterTiles.Num() - 1);
		FTileStruct* chosenTile = twoMostOuterTiles[index];

		chosenTile->bRiver = true;

		tiles.Append(GenerateRiver(chosenTile, Peak));
	}

	return tiles;
}

void AGrid::CalculateTile(FTileStruct* Tile)
{
	if (Tile->Level < 0)
		return;

	FTransform transform;
	FVector loc = FVector(Tile->X * 100.0f, Tile->Y * 100.0f, 0.0f);
	transform.SetLocation(loc);
	transform.SetRotation(Tile->Rotation);

	if (bLava && Tile->Level == MaxLevel) {
		transform.SetLocation(loc + FVector(0.0f, 0.0f, 75.0f * (MaxLevel - 2)));

		AddCalculatedTile(HISMLava, transform);

		LavaSpawnLocations.Add(transform.GetLocation());
	}
	else if (Tile->bRiver) {
		transform.SetRotation(FRotator(0.0f).Quaternion());
		transform.SetLocation(loc + FVector(0.0f, 0.0f, 75.0f * Tile->Level));

		AddCalculatedTile(HISMRiver, transform);

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

				CreateWaterfall(transform.GetLocation(), num, sign, bOnYAxis);
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

			CreateWaterfall(transform.GetLocation(), num, sign, bOnYAxis);
		}
	}
	else {
		if (Tile->Fertility == 0)
			Tile->Level = MaxLevel - 1;

		transform.SetLocation(loc + FVector(0.0f, 0.0f, 75.0f * Tile->Level));

		if (Tile->bEdge) {
			AddCalculatedTile(HISMGround, transform);
		}
		else {
			int32 chance = Camera->Stream.RandRange(1, 100);

			bool canUseRamp = true;

			if (Tile->AdjacentTiles.Num() < 4)
				canUseRamp = false;
			else
				for (auto& element : Tile->AdjacentTiles)
					if (element.Value->Level < 0)
						canUseRamp = false;

			if (canUseRamp) {
				for (auto& element : Tile->AdjacentTiles) {
					if (element.Value->Level > Tile->Level && element.Value->Level != MaxLevel && chance > 99) {
						Tile->bRamp = true;

						transform.SetRotation((FVector(Tile->X * 100.0f, Tile->Y * 100.0f, 0) - FVector(element.Value->X * 100.0f, element.Value->Y * 100.0f, 0)).ToOrientationQuat());

						break;
					}
				}
			}

			if (Tile->bRamp) {
				transform.SetLocation(transform.GetLocation() + FVector(0.0f, 0.0f, 100.0f));

				AddCalculatedTile(HISMRampGround, transform);
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
					AddCalculatedTile(HISMFlatGround, transform);
				}
				else {
					AddCalculatedTile(HISMGround, transform);

					Tile->bEdge = true;
				}
			}
		}
	}

	Tile->Rotation = transform.GetRotation();
}

void AGrid::AddCalculatedTile(UHierarchicalInstancedStaticMeshComponent* HISM, FTransform Transform)
{
	if (CalculatedTiles.Contains(HISM)) {
		TArray<FTransform>* t = CalculatedTiles.Find(HISM);

		t->Add(Transform);
	}
	else {
		CalculatedTiles.Add(HISM, { Transform });
	}
}

void AGrid::GenerateTiles()
{
	for (auto& element : CalculatedTiles) {
		TArray<int32> instances = element.Key->AddInstances(element.Value, true, true, false);

		for (int32 inst : instances) {
			if (inst == 0)
				FNavigationSystem::RegisterComponent(*element.Key);

			FTransform transform;
			element.Key->GetInstanceTransform(inst, transform);

			FLinearColor colour = FLinearColor(Camera->Stream.FRandRange(0.0f, 1.0f), Camera->Stream.FRandRange(0.0f, 1.0f), Camera->Stream.FRandRange(0.0f, 1.0f));

			if (element.Key->GetOwner()->IsA<AResource>()) {
				if (element.Key->GetOwner()->IsA<AVegetation>()) {
					bool bTree = false;

					for (FResourceHISMStruct resourceStruct : TreeStruct) {
						if (resourceStruct.Resource->ResourceHISM != element.Key)
							continue;

						bTree = true;

						break;
					}

					if (bTree) {
						int32 colourIndex = FMath::Floor(Camera->Stream.RandRange(0, 20) / 4.0f);

						colour = TreeColours[colourIndex];

						int32 dyingChance = Camera->Stream.RandRange(0, 100);

						if (dyingChance == 100)
							element.Key->PerInstanceSMCustomData[inst * element.Key->NumCustomDataFloats + 10] = 1.0f;
						else
							element.Key->PerInstanceSMCustomData[inst * element.Key->NumCustomDataFloats + 10] = 0.0f;

						element.Key->PerInstanceSMCustomData[inst * element.Key->NumCustomDataFloats + 9] = transform.GetScale3D().Z;
					}

					element.Key->PerInstanceSMCustomData[inst * element.Key->NumCustomDataFloats] = 0.0f;
					element.Key->PerInstanceSMCustomData[inst * element.Key->NumCustomDataFloats + 1] = 1.0f;
					element.Key->PerInstanceSMCustomData[inst * element.Key->NumCustomDataFloats + 2] = colour.R;
					element.Key->PerInstanceSMCustomData[inst * element.Key->NumCustomDataFloats + 3] = colour.G;
					element.Key->PerInstanceSMCustomData[inst * element.Key->NumCustomDataFloats + 4] = colour.B;
				}
				else {
					element.Key->PerInstanceSMCustomData[inst * element.Key->NumCustomDataFloats] = 0.0f;
				}
			}
			else if (element.Key == HISMRiver) {
				int32 level = FMath::RoundHalfFromZero(transform.GetLocation().Z / 75.0f);

				colour.R = 0.0f + (76.0f / 5.0f * (5.0f - level));
				colour.G = 99.0f + (80.0f / 5.0f * (5.0f - level));
				colour.B = 255.0f;

				colour.R /= 255.0f;
				colour.G /= 255.0f;
				colour.B /= 255.0f;

				HISMRiver->PerInstanceSMCustomData[inst * HISMRiver->NumCustomDataFloats] = 1.0f;
				HISMRiver->PerInstanceSMCustomData[inst * HISMRiver->NumCustomDataFloats + 1] = colour.R;
				HISMRiver->PerInstanceSMCustomData[inst * HISMRiver->NumCustomDataFloats + 2] = colour.G;
				HISMRiver->PerInstanceSMCustomData[inst * HISMRiver->NumCustomDataFloats + 3] = colour.B;
			}

			if ((int32)transform.GetLocation().X % 100 != 0 || (int32)transform.GetLocation().Y % 100 != 0)
				continue;

			FTileStruct* tile = GetTileFromLocation(transform.GetLocation());

			if (tile == nullptr)
				continue;

			if (element.Key == HISMFlatGround || element.Key == HISMGround || element.Key == HISMRampGround) {
				colour = GroundColours[tile->Fertility];

				element.Key->PerInstanceSMCustomData[inst * element.Key->NumCustomDataFloats] = 0.0f;
				element.Key->PerInstanceSMCustomData[inst * element.Key->NumCustomDataFloats + 1] = 1.0f;
				element.Key->PerInstanceSMCustomData[inst * element.Key->NumCustomDataFloats + 2] = colour.R;
				element.Key->PerInstanceSMCustomData[inst * element.Key->NumCustomDataFloats + 3] = colour.G;
				element.Key->PerInstanceSMCustomData[inst * element.Key->NumCustomDataFloats + 4] = colour.B;
			}

			tile->Instance = inst;
		}

		element.Key->PartialNavigationUpdates({ element.Value });
		element.Key->BuildTreeIfOutdated(false, true);
	}

	CalculatedTiles.Empty();
}

void AGrid::CreateWaterfall(FVector Location, int32 Num, int32 Sign, bool bOnYAxis)
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

		AddCalculatedTile(HISMRiver, transform);
	}
}

//
// Resources
//
void AGrid::SetMineralMultiplier(TSubclassOf<class AMineral> MineralClass, int32 Multiplier)
{
	for (FResourceHISMStruct& mineral : MineralStruct) {
		if (mineral.ResourceClass != MineralClass)
			continue;

		mineral.Multiplier = Multiplier;

		break;
	}
}

void AGrid::GetValidSpawnLocations(FTileStruct* SpawnTile, FTileStruct* CheckTile, int32 Range, bool& Valid, TArray<FTileStruct*>& Tiles)
{
	if (CheckTile->X > SpawnTile->X + Range || CheckTile->X < SpawnTile->X - Range || CheckTile->Y > SpawnTile->Y + Range || CheckTile->Y < SpawnTile->Y - Range)
		return;

	if (CheckTile->bRamp || CheckTile->bRiver || CheckTile->bMineral || CheckTile->bUnique || CheckTile->Level != SpawnTile->Level || CheckTile->AdjacentTiles.Num() < 4 || CheckTile->Level < 0) {
		Valid = false;

		return;
	}

	Tiles.Add(CheckTile);

	for (auto& element : CheckTile->AdjacentTiles)
		if (!Tiles.Contains(element.Value))
			GetValidSpawnLocations(SpawnTile, element.Value, Range, Valid, Tiles);
}

void AGrid::GenerateMinerals(FTileStruct* Tile, AResource* Resource)
{
	FTransform transform = GetTransform(Tile);

	AddCalculatedTile(Resource->ResourceHISM, transform);

	ResourceTiles.Remove(Tile);
}

void AGrid::GenerateVegetation(TArray<FResourceHISMStruct> Vegetation, FTileStruct* StartingTile, FTileStruct* Tile, int32 Amount, float Scale, bool bTree)
{
	int32 distanceFromStart = FMath::Abs(StartingTile->X - Tile->X) + FMath::Abs(StartingTile->Y - Tile->Y);

	int32 dist = 3;

	if (!bTree)
		dist = 2;

	if (distanceFromStart > dist * VegetationSizeMultiplier) {
		if (distanceFromStart > dist * 2 * VegetationSizeMultiplier)
			Amount -= 1;
		else
			Amount = Camera->Stream.RandRange(Amount - 1, Amount);
	}

	if (Amount == 0 || Tile->Fertility == 0 || !ResourceTiles.Contains(Tile) || VegetationTiles.Contains(Tile) || Tile->bRiver || Tile->Level < 0 || GetTransform(Tile).GetLocation().Z < 0.0f)
		return;

	TArray<int32> usedX;
	TArray<int32> usedY;

	int32 num = Amount;

	if (!bTree)
		num *= Camera->Stream.RandRange(10, 20);

	for (int32 i = 0; i < num; i++) {
		bool validXY = false;
		int32 x = 0;
		int32 y = 0;

		while (!validXY) {
			x = Camera->Stream.RandRange(-40, 40);
			y = Camera->Stream.RandRange(-40, 40);

			if (!usedX.Contains(x) && !usedY.Contains(y))
				validXY = true;
		}

		FTransform transform;
		transform.SetLocation(GetTransform(Tile).GetLocation() + FVector(x, y, 0.0f));

		float size = Camera->Stream.FRandRange(1.0f / Scale, Scale);
		transform.SetScale3D(FVector(size));

		transform.SetRotation(FRotator(0.0f, Camera->Stream.RandRange(0, 360), 0.0f).Quaternion());

		int32 index = Camera->Stream.RandRange(0, Vegetation.Num() - 1);

		AVegetation* resource = Cast<AVegetation>(Vegetation[index].Resource);

		AddCalculatedTile(resource->ResourceHISM, transform);
	}

	VegetationTiles.Add(Tile);

	for (auto& element : Tile->AdjacentTiles)
		GenerateVegetation(Vegetation, StartingTile, element.Value, Amount, Scale, bTree);
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
	int32 index = Camera->Stream.RandRange(0, ResourceTiles.Num() - 1);

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

	LavaComponent->Deactivate();
	LavaSpawnLocations.Empty();

	AtmosphereComponent->Clouds->Clear();

	TArray<AActor*> baskets;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEggBasket::StaticClass(), baskets);

	for (AActor* actor : baskets)
		actor->Destroy();

	HISMLava->ClearInstances();
	HISMSea->ClearInstances();
	HISMGround->ClearInstances();
	HISMFlatGround->ClearInstances();
	HISMRampGround->ClearInstances();
	HISMRiver->ClearInstances();

	if (Camera->PauseUIInstance->IsInViewport())
		Camera->SetPause(false, false);
}

FTileStruct* AGrid::GetTileFromLocation(FVector WorldLocation)
{
	auto bound = GetMapBounds();

	int32 x = FMath::RoundHalfFromZero(WorldLocation.X / 100.0f) + (bound / 2);
	int32 y = FMath::RoundHalfFromZero(WorldLocation.Y / 100.0f) + (bound / 2);

	if (x >= bound || x < 0 || y >= bound || y < 0)
		return nullptr;

	return &Storage[x][y];
}

//
// Special Buildings
//
void AGrid::SetSpecialBuildings(TArray<TArray<FTileStruct*>> ValidTiles)
{
	for (ASpecial* building : SpecialBuildings) {
		if (bRandSpecialBuildings) {
			int32 value = Camera->Stream.RandRange(0, 1);

			if (value == 0)
				building->SetActorHiddenInGame(true);
			else
				building->SetActorHiddenInGame(false);
		}

		float yaw = Camera->Stream.RandRange(0, 3) * 90.0f;

		building->SetActorRotation(building->GetActorRotation() + FRotator(0.0f, yaw, 0.0f));

		TArray<FTileStruct*> validLocations;

		for (TArray<FTileStruct*> tiles : ValidTiles) {
			FTileStruct* tile = tiles[0];

			building->SetActorLocation(GetTransform(tile).GetLocation());

			if (!Camera->BuildComponent->IsValidLocation(building))
				continue;

			validLocations.Add(tile);
		}

		if (validLocations.IsEmpty()) {
			building->SetActorHiddenInGame(true);
		}
		else {
			int32 chosenLocation = Camera->Stream.RandRange(0, validLocations.Num() - 1);
			building->SetActorLocation(GetTransform(validLocations[chosenLocation]).GetLocation());

			validLocations[chosenLocation]->bUnique = true;
		}

		SetSpecialBuildingStatus(building, !building->IsHidden());
	}

	Camera->UpdateMapSpecialBuildings();
}

void AGrid::SetSpecialBuildingStatus(ASpecial* Building, bool bShow)
{
	Building->SetActorHiddenInGame(!bShow);

	Camera->BuildComponent->SetTreeStatus(Building, false, !bShow);
}

void AGrid::BuildSpecialBuildings()
{
	for (int32 i = SpecialBuildings.Num() - 1; i > -1; i--) {
		ASpecial* building = SpecialBuildings[i];

		if (building->IsHidden()) {
			building->Destroy();

			SpecialBuildings.RemoveAt(i);
		}
		else {
			Camera->BuildComponent->SetTreeStatus(building, true);

			building->BuildingMesh->SetCanEverAffectNavigation(true);
		}
	}
}