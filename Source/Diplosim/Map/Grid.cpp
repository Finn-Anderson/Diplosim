#include "Grid.h"

#include "Math/UnrealMathUtility.h"
#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Blueprint/UserWidget.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Engine/ExponentialHeightFog.h"

#include "Atmosphere/Clouds.h"
#include "Resources/Mineral.h"
#include "Resources/Vegetation.h"
#include "Atmosphere/AtmosphereComponent.h"
#include "Player/Camera.h"
#include "Player/Components/CameraMovementComponent.h"
#include "Universal/EggBasket.h"
#include "Universal/DiplosimUserSettings.h"

AGrid::AGrid()
{
	PrimaryActorTick.bCanEverTick = false;

	HISMLava = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMLava"));
	HISMLava->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMLava->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);
	HISMLava->SetCanEverAffectNavigation(false);
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

	HISMRampGround = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMRampGround"));
	HISMRampGround->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMRampGround->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);
	HISMRampGround->NumCustomDataFloats = 4;

	AtmosphereComponent = CreateDefaultSubobject<UAtmosphereComponent>(TEXT("AtmosphereComponent"));

	Size = 22500;
	Peaks = 2;
	Type = EType::Round;

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

	Clouds = GetWorld()->SpawnActor<AClouds>(CloudsClass, FVector(0.0f, 0.0f, 0.0f), FRotator(0.0f, 0.0f, 0.0f));
	Clouds->Grid = this;

	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();
	settings->Fog = Fog;
	settings->Atmosphere = AtmosphereComponent;

	if (!settings->GetRenderFog())
		Fog->SetHidden(true);

	AtmosphereComponent->SetSunStatus(settings->GetSunPosition());
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

	int32 level = 8;

	int32 levelCount = 0;

	float pow = 0.5f;

	// Set tile information based on adjacent tile types until all tile struct choices are set
	while (levelTotal > 0) {
		// Set current level
		if (levelCount <= 0) {
			level--;

			if (level == 0)
				levelCount = levelTotal;
			else
				levelCount = FMath::Pow(pow, FMath::Abs(level)) * levelTotal;
		}

		// Get Tile and adjacent tiles
		FTileStruct* chosenTile = nullptr;

		if (peaksList.Num() < Peaks) {
			int32 x = Storage.Num();
			int32 y = Storage[0].Num();

			int32 chosenX = FMath::RandRange(bound / 4, x - bound / 4 - 1);
			int32 chosenY = FMath::RandRange(bound / 4, y - bound / 4 - 1);

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

					levelTotal--;
					levelCount--;
				}
			}
		}
		else {
			int32 chosenNum = FMath::RandRange(0, chooseableTiles.Num() - 1);
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

		levelTotal--;
		levelCount--;
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

	// Spawn resources
	ResourceTiles.Empty();

	for (TArray<FTileStruct> &row : Storage) {
		for (FTileStruct &tile : row) {
			if (tile.Level < 0 || tile.Level > 4)
				continue;

			ResourceTiles.Add(&tile);
		}
	}

	int32 num = ResourceTiles.Num() / 1000;

	TArray<AResource*>* minerals = Resources.Find("Mineral");

	for (AResource* mineral : *minerals) {
		for (int32 i = 0; i < num; i++) {
			int32 chosenNum = FMath::RandRange(0, ResourceTiles.Num() - 1);
			FTileStruct* chosenTile = ResourceTiles[chosenNum];

			GenerateMinerals(chosenTile, mineral);
		}
	}
	

	for (int32 i = 0; i < num; i++) {
		int32 chosenNum = FMath::RandRange(0, ResourceTiles.Num() - 1);
		FTileStruct* chosenTile = ResourceTiles[chosenNum];

		int32 amount = FMath::RandRange(1, 5);

		GenerateTrees(chosenTile, amount);
	}

	// Spawn clouds
	Clouds->ActivateCloud();

	// Set Camera Bounds
	FVector c1 = FVector(bound * 100, bound * 100, 0);
	FVector c2 = FVector(-bound * 100, -bound * 100, 0);

	Camera->MovementComponent->SetBounds(c1, c2);

	// Spawn egg basket
	GetWorld()->GetTimerManager().SetTimer(EggBasketTimer, this, &AGrid::SpawnEggBasket, 300.0f, true, 0.0f);

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

	for (int32 i = 0; i < levels.Num() - 1; i++)
		for (int32 j = 0; j < levels.Num() - i - 1; j++)
			if (levels[j] > levels[j + 1])
				levels.Swap(j, j + 1);

	float result;

	if (levels.Num() % 2 == 0)
		result = FMath::RoundHalfFromZero((levels[levels.Num() / 2] + levels[levels.Num() / 2 - 1]) / 2.0f);
	else
		result = levels[levels.Num() / 2];

	if (result == Tile->Level)
		return;

	Tile->Level = result;

	for (auto& element : Tile->AdjacentTiles)
		FillHoles(element.Value);
}

void AGrid::SetTileDetails(FTileStruct* Tile)
{
	// Set Rotation
	int32 rand = FMath::RandRange(0, 3);

	Tile->Rotation = (FRotator(0.0f, 90.0f, 0.0f) * rand).Quaternion();

	// Set fertility
	if (Tile->Level == 7 || Tile->Level < 0)
		return;

	int32 value = FMath::RandRange(-1, 1);

	int32 fTile = 0;
	int32 count = 0;

	for (auto& element : Tile->AdjacentTiles) {
		FTileStruct* t = element.Value;

		if (t->Level == 7) {
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

void AGrid::GenerateTile(FTileStruct* Tile)
{
	if (Tile->Level < 0)
		return;

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

		bool bRamp = false;

		int32 chance = FMath::RandRange(1, 100);

		for (auto& element : Tile->AdjacentTiles) {
			if (element.Value->Level > Tile->Level && element.Value->Level != 7 && chance > 99) {
				bRamp = true;

				transform.SetRotation((FVector(Tile->X * 100.0f, Tile->Y * 100.0f, 0) - FVector(element.Value->X * 100.0f, element.Value->Y * 100.0f, 0)).ToOrientationQuat());
				transform.SetLocation(transform.GetLocation() + FVector(0.0f, 0.0f, 100.0f));

				break;
			}
		}
			

		if (bRamp) {
			inst = HISMRampGround->AddInstance(transform);
			HISMRampGround->SetCustomDataValue(inst, 0, 1.0f);
			HISMRampGround->SetCustomDataValue(inst, 1, r);
			HISMRampGround->SetCustomDataValue(inst, 2, g);
			HISMRampGround->SetCustomDataValue(inst, 3, b);
		}
		else {
			bool bFlat = true;

			if (Tile->AdjacentTiles.Num() < 4)
				bFlat = false;
			else
				for (auto& element : Tile->AdjacentTiles)
					if (element.Value->Level < Tile->Level || (element.Value->Level == 7 && Tile->Level != 5))
						bFlat = false;

			if (bFlat) {
				inst = HISMFlatGround->AddInstance(transform);
				HISMFlatGround->SetCustomDataValue(inst, 0, 1.0f);
				HISMFlatGround->SetCustomDataValue(inst, 1, r);
				HISMFlatGround->SetCustomDataValue(inst, 2, g);
				HISMFlatGround->SetCustomDataValue(inst, 3, b);
			}
			else {
				inst = HISMGround->AddInstance(transform);
				HISMGround->SetCustomDataValue(inst, 0, 1.0f);
				HISMGround->SetCustomDataValue(inst, 1, r);
				HISMGround->SetCustomDataValue(inst, 2, g);
				HISMGround->SetCustomDataValue(inst, 3, b);
			}
		}
	}

	Tile->Instance = inst;
}

void AGrid::GenerateMinerals(FTileStruct* Tile, AResource* Resource)
{
	AMineral* mineral = Cast<AMineral>(Resource);

	int32 inst = mineral->ResourceHISM->AddInstance(GetTransform(Tile));

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

		resource->ResourceHISM->SetCustomDataValue(inst, 0, 1.0f);
	}

	ResourceTiles.Remove(Tile);

	for (auto& element : Tile->AdjacentTiles)
		GenerateTrees(element.Value, value);
}

void AGrid::SpawnEggBasket()
{
	int32 index = FMath::RandRange(0, ResourceTiles.Num() - 1);

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
	bool bFlat = true;

	if (Tile->AdjacentTiles.Num() < 4)
		bFlat = false;
	else
		for (auto& element : Tile->AdjacentTiles)
			if (element.Value->Level < Tile->Level || element.Value->Level == 7)
				bFlat = false;

	FTransform transform;
	int32 z = 100.0f;

	if (bFlat)
		HISMFlatGround->GetInstanceTransform(Tile->Instance, transform);
	else
		HISMGround->GetInstanceTransform(Tile->Instance, transform);

	transform.SetLocation(transform.GetLocation() + FVector(0.0f, 0.0f, z));

	return transform;
}

void AGrid::Clear()
{
	for (const TPair<FString, TArray<AResource*>>& pair : Resources)
		for (AResource* r : pair.Value)
			r->ResourceHISM->ClearInstances();

	Storage.Empty();

	for (UNiagaraComponent* comp : LavaComponents)
		comp->DestroyComponent();

	LavaComponents.Empty();

	Clouds->Clear();

	GetWorld()->GetTimerManager().ClearTimer(EggBasketTimer);

	TArray<AActor*> baskets;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEggBasket::StaticClass(), baskets);

	for (AActor* actor : baskets)
		actor->Destroy();

	HISMLava->ClearInstances();
	HISMGround->ClearInstances();
	HISMFlatGround->ClearInstances();
	HISMRampGround->ClearInstances();
}