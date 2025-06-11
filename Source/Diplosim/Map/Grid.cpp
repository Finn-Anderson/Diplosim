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

#include "Atmosphere/Clouds.h"
#include "Resources/Mineral.h"
#include "Resources/Vegetation.h"
#include "Atmosphere/AtmosphereComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Components/CameraMovementComponent.h"
#include "Universal/EggBasket.h"
#include "Universal/DiplosimUserSettings.h"
#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Buildings/Misc/Portal.h"

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
	HISMLava->SetEvaluateWorldPositionOffset(false);
	HISMLava->SetGenerateOverlapEvents(false);
	HISMLava->bWorldPositionOffsetWritesVelocity = false;
	HISMLava->bAutoRebuildTreeOnInstanceChanges = false;

	HISMGround = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMGround"));
	HISMGround->SetupAttachment(GetRootComponent());
	HISMGround->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HISMGround->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMGround->SetCollisionResponseToChannels(response);
	HISMGround->SetEvaluateWorldPositionOffset(false);
	HISMGround->SetGenerateOverlapEvents(false);
	HISMGround->bWorldPositionOffsetWritesVelocity = false;
	HISMGround->NumCustomDataFloats = 8;
	HISMGround->bAutoRebuildTreeOnInstanceChanges = false;
	HISMGround->ShadowCacheInvalidationBehavior = EShadowCacheInvalidationBehavior::Always;

	HISMFlatGround = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMFlatGround"));
	HISMFlatGround->SetupAttachment(GetRootComponent());
	HISMFlatGround->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HISMFlatGround->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMFlatGround->SetCollisionResponseToChannels(response);
	HISMFlatGround->SetCastShadow(false);
	HISMFlatGround->SetEvaluateWorldPositionOffset(false);
	HISMFlatGround->SetGenerateOverlapEvents(false);
	HISMFlatGround->bWorldPositionOffsetWritesVelocity = false;
	HISMFlatGround->NumCustomDataFloats = 8;
	HISMFlatGround->bAutoRebuildTreeOnInstanceChanges = false;

	HISMRampGround = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMRampGround"));
	HISMRampGround->SetupAttachment(GetRootComponent());
	HISMRampGround->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HISMRampGround->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMRampGround->SetCollisionResponseToChannels(response);
	HISMRampGround->SetEvaluateWorldPositionOffset(false);
	HISMRampGround->SetGenerateOverlapEvents(false);
	HISMRampGround->bWorldPositionOffsetWritesVelocity = false;
	HISMRampGround->NumCustomDataFloats = 8;
	HISMRampGround->bAutoRebuildTreeOnInstanceChanges = false;

	HISMRiver = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMRiver"));
	HISMRiver->SetupAttachment(GetRootComponent());
	HISMRiver->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HISMRiver->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMRiver->SetCollisionResponseToChannels(response);
	HISMRiver->SetCanEverAffectNavigation(false);
	HISMRiver->SetGenerateOverlapEvents(false);
	HISMRiver->SetWorldPositionOffsetDisableDistance(5000);
	HISMRiver->bWorldPositionOffsetWritesVelocity = false;
	HISMRiver->NumCustomDataFloats = 4;
	HISMRiver->bAutoRebuildTreeOnInstanceChanges = false;

	HISMWall = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMWall"));
	HISMWall->SetupAttachment(GetRootComponent());
	HISMWall->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HISMWall->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMWall->SetCollisionResponseToChannels(response);
	HISMWall->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Ignore);
	HISMWall->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Ignore);
	HISMWall->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	HISMWall->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
	HISMWall->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECollisionResponse::ECR_Block);
	HISMWall->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel3, ECollisionResponse::ECR_Block);
	HISMWall->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel4, ECollisionResponse::ECR_Block);
	HISMWall->SetCollisionResponseToChannel(ECollisionChannel::ECC_Destructible, ECollisionResponse::ECR_Block);
	HISMWall->SetCanEverAffectNavigation(true);
	HISMWall->SetHiddenInGame(true);
	HISMWall->SetCastShadow(false);
	HISMWall->SetEvaluateWorldPositionOffset(false);
	HISMWall->SetGenerateOverlapEvents(false);
	HISMWall->bWorldPositionOffsetWritesVelocity = false;
	HISMWall->bAutoRebuildTreeOnInstanceChanges = false;

	LavaComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("LavaComponent"));
	LavaComponent->SetupAttachment(GetRootComponent());
	LavaComponent->SetAutoActivate(false);

	AtmosphereComponent = CreateDefaultSubobject<UAtmosphereComponent>(TEXT("AtmosphereComponent"));
	AtmosphereComponent->WindComponent->SetupAttachment(RootComponent);

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

	VegetationMinDensity = 1;
	VegetationMaxDensity = 3;
	VegetationMultiplier = 1;
	VegetationSizeMultiplier = 5;
}

void AGrid::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Camera = PController->GetPawn<ACamera>();

	MapUIInstance = CreateWidget<UUserWidget>(PController, MapUI);

	LoadUIInstance = CreateWidget<UUserWidget>(PController, LoadUI);

	Camera->Grid = this;

	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();
	HISMRiver->SetWorldPositionOffsetDisableDistance(settings->GetWPODistance());

	for (FResourceHISMStruct &ResourceStruct : TreeStruct) {
		ResourceStruct.Resource = GetWorld()->SpawnActor<AResource>(ResourceStruct.ResourceClass, FVector::Zero(), FRotator(0.0f));
		ResourceStruct.Resource->ResourceHISM->SetWorldPositionOffsetDisableDistance(settings->GetWPODistance());
		ResourceStruct.Resource->ResourceHISM->bWorldPositionOffsetWritesVelocity = false;
		ResourceStruct.Resource->ResourceHISM->bAutoRebuildTreeOnInstanceChanges = false;
	}

	for (FResourceHISMStruct& ResourceStruct : FlowerStruct) {
		ResourceStruct.Resource = GetWorld()->SpawnActor<AResource>(ResourceStruct.ResourceClass, FVector::Zero(), FRotator(0.0f));
		ResourceStruct.Resource->ResourceHISM->SetWorldPositionOffsetDisableDistance(settings->GetWPODistance());
		ResourceStruct.Resource->ResourceHISM->bWorldPositionOffsetWritesVelocity = false;
		ResourceStruct.Resource->ResourceHISM->bAutoRebuildTreeOnInstanceChanges = false;
	}

	for (FResourceHISMStruct& ResourceStruct : MineralStruct) {
		ResourceStruct.Resource = GetWorld()->SpawnActor<AResource>(ResourceStruct.ResourceClass, FVector::Zero(), FRotator(0.0f));
		ResourceStruct.Resource->ResourceHISM->bAutoRebuildTreeOnInstanceChanges = false;
	}

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	FScriptDelegate delegate;
	delegate.BindUFunction(this, TEXT("OnNavMeshGenerated"));

	nav->OnNavigationGenerationFinishedDelegate.Add(delegate);
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

	// Set tile information based on adjacent tile types until all tile struct choices are set
	while (true) {
		// Set current level
		if (levelCount <= 0) {
			level--;

			if (level < 0)
				break;

			int32 height = level;

			if (Type == EType::Mountain && level != MaxLevel)
				height = (MaxLevel / FMath::Min(FMath::Max(1, level) * 2.0f, MaxLevel));

			float percentage = (1.85f / FMath::Sqrt(2.0f * PI * FMath::Square(MaxLevel / 2.5f)) * FMath::Exp(-1.0f * (FMath::Square(height) / (1.85f * FMath::Square(MaxLevel / 2.5f)))));
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
				float distance = FVector2D::Distance(FVector2D(peaksList[0]->X, peaksList[0]->Y), FVector2D(peaksList[1]->X, peaksList[1]->Y));

				FVector2D p0 = FVector2D::Zero();
				FVector2D p1 = FVector2D::Zero();
				FVector2D p2 = FVector2D::Zero();
				FVector2D p3 = FVector2D::Zero();

				if (peaksList[0]->Y > peaksList[1]->Y) {
					p0 = FVector2D(peaksList[1]->X, peaksList[1]->Y);
					p3 = FVector2D(peaksList[0]->X, peaksList[0]->Y);
				}
				else {
					p0 = FVector2D(peaksList[0]->X, peaksList[0]->Y);
					p3 = FVector2D(peaksList[1]->X, peaksList[1]->Y);
				}

				FVector2D pHalf = (p0 + p3) / 2.0f;

				float variance = distance * 0.66f;

				p1 = FVector2D(FMath::RandRange(p0.X, pHalf.X) + FMath::RandRange(-variance, variance), FMath::RandRange(p0.Y, pHalf.Y) + FMath::RandRange(-variance, variance));
				p2 = FVector2D(FMath::RandRange(pHalf.X, p3.X) + FMath::RandRange(-variance, variance), FMath::RandRange(pHalf.Y, p3.Y) + FMath::RandRange(-variance, variance));

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

						peaksList.Add(element.Value);

						for (auto& e : element.Value->AdjacentTiles)
							if (!chooseableTiles.Contains(e.Value) && e.Value->Level < 0)
								chooseableTiles.Add(e.Value);
					}

					tile->Level = level;

					levelCount--;

					chooseableTiles.Remove(tile);

					peaksList.Add(tile);

					t += 0.01f;
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

	ResourceTiles.Empty();

	// Spawn Tiles
	for (TArray<FTileStruct>& row : Storage) {
		for (FTileStruct& tile : row) {
			GenerateTile(&tile);

			if (tile.Level < 0 || tile.Level > 4 || tile.bRiver || tile.bRamp)
				continue;

			ResourceTiles.Add(&tile);
		}
	}

	for (TArray<FTileStruct>& row : Storage)
		for (FTileStruct& tile : row)
			CreateEdgeWalls(&tile);

	HISMLava->BuildTreeIfOutdated(true, false);
	HISMGround->BuildTreeIfOutdated(true, false);
	HISMFlatGround->BuildTreeIfOutdated(true, false);
	HISMRampGround->BuildTreeIfOutdated(true, false);
	HISMRiver->BuildTreeIfOutdated(true, false);
	HISMWall->BuildTreeIfOutdated(true, false);

	// Spawn resources
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

			ResourceStruct.Resource->ResourceHISM->BuildTreeIfOutdated(true, false);
		}
	}

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
			int32 chosenNum = Stream.RandRange(0, ResourceTiles.Num() - 1);
			FTileStruct* chosenTile = ResourceTiles[chosenNum];

			int32 amount = Stream.RandRange(min, max);

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

		for (FResourceHISMStruct ResourceStruct : element.Value) {
			ResourceStruct.Resource->ResourceHISM->BuildTreeIfOutdated(true, true);

			ResourceStruct.Resource->ResourceHISM->bAutoRebuildTreeOnInstanceChanges = true;
		}
	}

	// Set Atmosphere Affects
	SetSeasonAffect(AtmosphereComponent->Calendar.Period, 1.0f);

	AtmosphereComponent->SetWindDimensions(bound);

	// Spawn clouds
	AtmosphereComponent->Clouds->ActivateCloud();
	AtmosphereComponent->Clouds->StartCloudTimer();

	// Set Camera Bounds
	FVector c1 = FVector(bound * 100, bound * 100, 0);
	FVector c2 = FVector(-bound * 100, -bound * 100, 0);

	Camera->MovementComponent->SetBounds(c1, c2);

	// Spawn egg basket
	SpawnEggBasket();

	// Lava Component
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(LavaComponent, TEXT("SpawnLocations"), LavaSpawnLocations);
	LavaComponent->SetVariableFloat(TEXT("SpawnRate"), LavaSpawnLocations.Num() / 10.0f);
	LavaComponent->Activate();

	// Unique Buildings
	int32 index = Stream.RandRange(0, ResourceTiles.Num() - 1);
	FVector location = FVector(ResourceTiles[index]->X * 100.0f, ResourceTiles[index]->Y * 100.0f, ResourceTiles[index]->Level * 75.0f + 100.0f);

	APortal* portal = GetWorld()->SpawnActor<APortal>(PortalClass, location, ResourceTiles[index]->Rotation.Rotator());
	Camera->ConquestManager->Portal = portal;

	// Conquest Map
	Camera->ConquestManager->GenerateWorld();
	Camera->UpdateWorldMap();
	Camera->UpdateInteractUI();
	Camera->UpdateFactionIcons();

	if (Camera->PauseUIInstance->IsInViewport())
		Camera->SetPause(true, true);
}

void AGrid::OnNavMeshGenerated()
{
	// Remove loading screen
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
		int32 index = Stream.RandRange(0, twoMostOuterTiles.Num() - 1);
		FTileStruct* chosenTile = twoMostOuterTiles[index];

		chosenTile->bRiver = true;

		tiles.Append(GenerateRiver(chosenTile, Peak));
	}

	return tiles;
}

void AGrid::GenerateTile(FTileStruct* Tile)
{
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

	if (Tile->Level < 0) {
		transform.SetLocation(loc + FVector(0.0f, 0.0f, -200.0f));

		inst = HISMFlatGround->AddInstance(transform);
	}
	else if (bLava && Tile->Level == MaxLevel) {
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
		if (Tile->Fertility == 0)
			Tile->Level = MaxLevel - 1;

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

			bool canUseRamp = true;

			if (Tile->AdjacentTiles.Num() < 3)
				canUseRamp = false;
			else
				for (auto& element : Tile->AdjacentTiles)
					if (element.Value->Level < 0)
						canUseRamp = true;

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

	Tile->Rotation = transform.GetRotation();
	Tile->Instance = inst;
}

void AGrid::CreateEdgeWalls(FTileStruct* Tile)
{
	if (Tile->Level < 0 || Tile->bRiver)
		return;

	TArray<FVector2D> xy;

	if (Tile->AdjacentTiles.Num() < 4) {
		if (!Tile->AdjacentTiles.Find("Left"))
			xy.Add(FVector2D(Tile->X - 1, Tile->Y));

		if (!Tile->AdjacentTiles.Find("Right"))
			xy.Add(FVector2D(Tile->X + 1, Tile->Y));

		if (!Tile->AdjacentTiles.Find("Below"))
			xy.Add(FVector2D(Tile->X, Tile->Y - 1));

		if (!Tile->AdjacentTiles.Find("Above"))
			xy.Add(FVector2D(Tile->X, Tile->Y + 1));
	}

	for (auto& element : Tile->AdjacentTiles) {
		if ((!Tile->bRamp && element.Value->Level == Tile->Level && !element.Value->bRiver) || element.Value->bRamp || element.Value->Level > Tile->Level)
			continue;

		xy.Add(FVector2D(element.Value->X, element.Value->Y));
	}

	for (FVector2D coord : xy) {
		float x = Tile->X + (coord.X - Tile->X) / 2.0f;
		float y = Tile->Y + (coord.Y - Tile->Y) / 2.0f;

		int32 level = Tile->Level;

		if (level == 7)
			level = 6;

		FTransform transform;
		transform.SetLocation(FVector(x * 100.0f, y * 100.0f, level * 75.0f + 100.0f));

		if (y != Tile->Y)
			transform.SetRotation(FRotator(0.0f, 90.0f, 0.0f).Quaternion());

		if (Tile->bRamp) {
			if (FMath::Abs(FMath::RoundHalfFromZero(Tile->Rotation.Rotator().Yaw)) == 90.0f || FMath::Abs(FMath::RoundHalfFromZero(Tile->Rotation.Rotator().Yaw)) == 270.0f) {
				if (y != Tile->Y)
					continue;
			}
			else if (x != Tile->X)
				continue;
		}

		bool bExists = false;

		for (int32 i = 0; i < HISMWall->GetInstanceCount(); i++) {
			FTransform tf;
			HISMWall->GetInstanceTransform(i, tf);

			if (tf.GetLocation() != transform.GetLocation())
				continue;

			bExists = true;

			break;
		}

		if (!bExists)
			HISMWall->AddInstance(transform);
	}
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

void AGrid::SetMineralMultiplier(TSubclassOf<class AMineral> MineralClass, int32 Multiplier)
{
	for (FResourceHISMStruct& mineral : MineralStruct) {
		if (mineral.ResourceClass != MineralClass)
			continue;

		mineral.Multiplier = Multiplier;

		break;
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
	int32 distanceFromStart = FMath::Abs(StartingTile->X - Tile->X) + FMath::Abs(StartingTile->Y - Tile->Y);

	int32 dist = 3;

	if (!bTree)
		dist = 2;

	if (distanceFromStart > dist * VegetationSizeMultiplier) {
		if (distanceFromStart > dist * 2 * VegetationSizeMultiplier)
			Amount -= 1;
		else
			Amount = Stream.RandRange(Amount - 1, Amount);
	}

	if (Amount == 0 || Tile->Fertility == 0 || !ResourceTiles.Contains(Tile) || VegetationTiles.Contains(Tile) || Tile->bRiver || Tile->Level < 0 || GetTransform(Tile).GetLocation().Z < 0.0f)
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

	AtmosphereComponent->Clouds->Clear();

	TArray<AActor*> baskets;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEggBasket::StaticClass(), baskets);

	for (AActor* actor : baskets)
		actor->Destroy();

	HISMLava->ClearInstances();
	HISMGround->ClearInstances();
	HISMFlatGround->ClearInstances();
	HISMRampGround->ClearInstances();
	HISMRiver->ClearInstances();
	HISMWall->ClearInstances();

	Camera->ConquestManager->Portal->Destroy();

	if (Camera->PauseUIInstance->IsInViewport())
		Camera->SetPause(false, false);
}

void AGrid::SetSeasonAffect(FString Period, float Increment)
{
	if (Period == "Winter")
		AtmosphereComponent->Clouds->bSnow = true;
	else
		AtmosphereComponent->Clouds->bSnow = false;

	AlterSeasonAffectGradually(Period, Increment);

	AtmosphereComponent->Clouds->UpdateSpawnedClouds();
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