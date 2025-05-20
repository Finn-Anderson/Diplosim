#include "Buildings/Misc/Broch.h"

#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "NavigationSystem.h"

#include "AI/Citizen.h"
#include "Universal/HealthComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Map/Grid.h"
#include "Map/Resources/Mineral.h"

ABroch::ABroch()
{
	HealthComponent->MaxHealth = 300;
	HealthComponent->Health = HealthComponent->MaxHealth;

	BuildingMesh->bReceivesDecals = false;

	DecalComponent = CreateDefaultSubobject<UDecalComponent>("DecalComponent");
	DecalComponent->SetupAttachment(RootComponent);
	DecalComponent->DecalSize = FVector(2000.0f, 2000.0f, 2000.0f);
	DecalComponent->SetRelativeRotation(FRotator(-90, 0, 0));

	StorageCap = 1000000;
}

void ABroch::SpawnCitizens()
{
	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	auto bound = FMath::FloorToInt32(FMath::Sqrt((double)Camera->Grid->Size));
	FTileStruct tile = Camera->Grid->Storage[GetActorLocation().X / 100.0f + bound / 2][GetActorLocation().Y / 100.0f + bound / 2];

	TArray<FVector> locations = GetSpawnLocations(tile, tile, 4);

	for (int32 i = 0; i < Camera->CitizenNum; i++) {
		if (locations.IsEmpty())
			return;

		int32 index = FMath::RandRange(0, locations.Num() - 1);

		FNavLocation navLoc;
		nav->ProjectPointToNavigation(locations[index], navLoc);

		FActorSpawnParameters params;
		params.bNoFail = true;

		ACitizen* citizen = GetWorld()->SpawnActor<ACitizen>(CitizenClass, navLoc.Location, GetActorRotation() - FRotator(0.0f, 90.0f, 0.0f), params);

		citizen->SetSex(Camera->CitizenManager->Citizens);

		for (int32 j = 0; j < 2; j++)
			citizen->GivePersonalityTrait();

		citizen->BioStruct.Age = 17;
		citizen->Birthday();

		citizen->HealthComponent->AddHealth(100);

		citizen->ApplyResearch();

		citizen->SetActorLocation(navLoc.Location + FVector(0.0f, 0.0f, citizen->Capsule->GetScaledCapsuleHalfHeight()));

		citizen->MainIslandSetup();

		locations.RemoveAt(index);
	}
}

TArray<FVector> ABroch::GetSpawnLocations(FTileStruct StartingTile, FTileStruct Tile, int32 Radius, int32 Count)
{
	TArray<FVector> locations;

	FTransform startTransform = Camera->Grid->GetTransform(&StartingTile);
	FTransform transform = Camera->Grid->GetTransform(&Tile);

	FHitResult hit(ForceInit);

	if (Tiles.Contains(TTuple<int32, int32>(Tile.X, Tile.Y)) || GetWorld()->LineTraceSingleByChannel(hit, transform.GetLocation(), transform.GetLocation() + FVector(0.0f, 0.0f, 20.0f), ECollisionChannel::ECC_Visibility) && hit.GetActor()->IsA<AMineral>())
		return locations;

	for (FTileStruct* t : Camera->Grid->CalculatePath(&StartingTile, &Tile))
		if (t->bRiver)
			return locations;
	
	Tiles.Add(TTuple<int32, int32>(Tile.X, Tile.Y));

	Count++;

	for (int32 y = -2; y < 3; y++) {
		for (int32 x = -2; x < 3; x++) {
			FVector location = transform.GetLocation() + FVector(x * 20.0f, y * 20.0f, 0.0f);

			if (FVector::Dist(startTransform.GetLocation(), location) > 110.0f)
				locations.Add(location);
		}
	}

	if (Count == Radius)
		return locations;

	for (auto& element : Tile.AdjacentTiles)
		locations.Append(GetSpawnLocations(StartingTile, *element.Value, Radius, Count));

	return locations;
}