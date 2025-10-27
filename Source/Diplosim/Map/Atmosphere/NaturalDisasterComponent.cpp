#include "Map/Atmosphere/NaturalDisasterComponent.h"

#include "Components/DirectionalLightComponent.h"
#include "Kismet/GameplayStatics.h"

#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Buildings/Building.h"
#include "Buildings/Work/Work.h"
#include "Universal/Resource.h"
#include "Universal/HealthComponent.h"
#include "AI/Projectile.h"

UNaturalDisasterComponent::UNaturalDisasterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	ResetDisasterChance();

	Frequency = 1.0f;
	Intensity = 1.0f;
}

bool UNaturalDisasterComponent::ShouldCreateDisaster()
{
	float chance = Grid->Stream.FRandRange(0.0f, 100.0f);

	if (bDisasterChance <= chance)
		return false;

	return true;
}

void UNaturalDisasterComponent::IncrementDisasterChance()
{
	bDisasterChance += 0.02f * Frequency;

	if (!ShouldCreateDisaster())
		return;

	float magnitude = Grid->Stream.FRandRange(1.0f, 5.0f) * Intensity;

	int32 type = Grid->Stream.RandRange(0, 2);

	if (type == 0)
		GenerateEarthquake(magnitude);
	else if (type == 1)
		GeneratePurifier(magnitude);
	else
		GenerateRedSun(magnitude);

	ResetDisasterChance();
}

void UNaturalDisasterComponent::ResetDisasterChance()
{
	bDisasterChance = 0.0f;
}

void UNaturalDisasterComponent::GenerateEarthquake(float Magnitude)
{
	int32 bounds = Grid->Storage.Num() - 1;
	int32 distance = Magnitude * 5;
	float range = 500.0f * Magnitude;

	int32 iX = Grid->Stream.RandRange(0, bounds);
	int32 iY = Grid->Stream.RandRange(0, bounds);

	FTileStruct start = Grid->Storage[iX][iY];

	TArray<FTileStruct> endLocations;

	for (TArray<FTileStruct>& row : Grid->Storage) {
		for (FTileStruct& tile : row) {
			if (FMath::Abs(tile.X - start.X) + FMath::Abs(tile.Y - start.Y) != distance)
				continue;

			endLocations.Add(tile);
		}
	}

	int32 chosenEnd = Grid->Stream.RandRange(0, endLocations.Num() - 1);

	FTileStruct end = endLocations[chosenEnd];

	TArray<FTileStruct> points;

	auto bound = Grid->GetMapBounds();

	float dist = FVector2D::Distance(FVector2D(start.X, start.Y), FVector2D(end.X, end.Y));

	FVector2D p0 = FVector2D::Zero();
	FVector2D p1 = FVector2D::Zero();
	FVector2D p2 = FVector2D::Zero();
	FVector2D p3 = FVector2D::Zero();

	if (start.Y > end.Y) {
		p0 = FVector2D(end.X, end.Y);
		p3 = FVector2D(start.X, start.Y);
	}
	else {
		p0 = FVector2D(start.X, start.Y);
		p3 = FVector2D(end.X, end.Y);
	}

	FVector2D pHalf = (p0 + p3) / 2.0f;

	float variance = dist * 0.66f;

	p1 = FVector2D(Grid->Stream.RandRange(p0.X, pHalf.X) + Grid->Stream.RandRange(-variance, variance), Grid->Stream.RandRange(p0.Y, pHalf.Y) + Grid->Stream.RandRange(-variance, variance));
	p2 = FVector2D(Grid->Stream.RandRange(pHalf.X, p3.X) + Grid->Stream.RandRange(-variance, variance), Grid->Stream.RandRange(pHalf.Y, p3.Y) + Grid->Stream.RandRange(-variance, variance));

	double t = 0.0f;

	while (t <= 1.0f) {
		FVector2D point = FMath::Pow((1 - t), 3) * p0 + 3 * FMath::Pow((1 - t), 2) * t * p1 + 3 * (1 - t) * FMath::Pow(t, 2) * p2 + FMath::Pow(t, 3) * p3;

		int32 px = point.X + (bound / 2);
		int32 py = point.Y + (bound / 2);

		FTileStruct tile = Grid->Storage[px][py];

		if (!points.Contains(tile))
			points.Add(tile);

		t += 0.02f;
	}

	TArray<FEarthquakeStruct> earthquakeStructs;

	TArray<TEnumAsByte<EObjectTypeQuery>> objects;

	TArray<AActor*> ignore;
	ignore.Add(Cast<ACamera>(GetOwner())->Grid);

	for (FResourceHISMStruct resourceStruct : Grid->MineralStruct)
		ignore.Add(resourceStruct.Resource);

	for (FResourceHISMStruct resourceStruct : Grid->FlowerStruct)
		ignore.Add(resourceStruct.Resource);

	TArray<AActor*> actors;

	for (FTileStruct tile : points) {
		FEarthquakeStruct estruct;
		estruct.Point = Grid->GetTransform(&tile).GetLocation();

		UKismetSystemLibrary::SphereOverlapActors(GetWorld(), estruct.Point, range, objects, nullptr, ignore, actors);

		for (AActor* actor : actors) {
			if (!actor->IsA<ABuilding>() || IsProtected(actor->GetActorLocation()))
				continue;

			float d = FVector::Dist(estruct.Point, actor->GetActorLocation());

			estruct.BuildingsInRange.Add(Cast<ABuilding>(actor), d);
		}

		earthquakeStructs.Add(estruct);
	}

	TArray<FTimerParameterStruct> params;
	Grid->Camera->CitizenManager->SetParameter(earthquakeStructs, params);
	Grid->Camera->CitizenManager->SetParameter(range, params);
	Grid->Camera->CitizenManager->SetParameter(Magnitude, params);
	Grid->Camera->CitizenManager->CreateTimer("Earthquake", Grid, 4.0f * Magnitude, "CalculateEarthquakeDamage", params, true);
}

void UNaturalDisasterComponent::CalculateEarthquakeDamage(TArray<FEarthquakeStruct> EarthquakeStructs, float Range, float Magnitude)
{
	for (FEarthquakeStruct estruct : EarthquakeStructs) {
		for (auto& element : estruct.BuildingsInRange) {
			int32 damage = FMath::Max(1.0f, element.Value / Range / 10.0f * Magnitude);

			element.Key->HealthComponent->TakeHealth(damage, Grid);
		}

		UGameplayStatics::PlayWorldCameraShake(GetWorld(), Shake, estruct.Point, 0.0f, Range, 1.0f);
	}
}

void UNaturalDisasterComponent::CancelEarthquake()
{
	Grid->Camera->CitizenManager->RemoveTimer("Earthquake", Grid);
}

void UNaturalDisasterComponent::GeneratePurifier(float Magnitude)
{
	auto bound = Grid->GetMapBounds();

	int32 x = Grid->Stream.RandRange(0, bound);
	int32 y = Grid->Stream.RandRange(0, bound);

	FRotator rotation;
	rotation.Yaw = Grid->Stream.RandRange(0, 359);
	rotation.Pitch = Grid->Stream.RandRange(-90, -30);

	AProjectile* projectile = GetWorld()->SpawnActor<AProjectile>(PurifierClass, Grid->GetTransform(&Grid->Storage[x][y]).GetLocation() - (rotation.Vector() * 1000.0f), rotation);
	projectile->SpawnNiagaraSystems(GetOwner());
	projectile->Radius *= Magnitude;
}

void UNaturalDisasterComponent::GenerateRedSun(float Magnitude)
{
	Grid->AtmosphereComponent->bRedSun = true;

	AlterSunGradually(0.15f, -0.02f);

	Grid->Camera->CitizenManager->CreateTimer("Red Sun", Grid, 120.0f * Magnitude, "CancelRedSun", {}, false);
}

void UNaturalDisasterComponent::AlterSunGradually(float Target, float Increment)
{
	FLinearColor colour = Grid->AtmosphereComponent->Sun->GetLightColor();

	if (colour.G == Target)
		return;

	colour.G = FMath::Clamp(colour.G + Increment, 0.15f, 1.0f);
	colour.B = FMath::Clamp(colour.B + Increment, 0.15f, 1.0f);

	Grid->AtmosphereComponent->Sun->SetLightColor(colour);
	Grid->AtmosphereComponent->Moon->SetLightColor(colour);
	
	FTimerHandle sunChangeTimer;
	GetWorld()->GetTimerManager().SetTimer(sunChangeTimer, FTimerDelegate::CreateUObject(this, &UNaturalDisasterComponent::AlterSunGradually, Target, Increment), 0.02f, false);
}

void UNaturalDisasterComponent::CancelRedSun()
{
	Grid->AtmosphereComponent->bRedSun = false;

	AlterSunGradually(1.0f, 0.02f);
}

bool UNaturalDisasterComponent::IsProtected(FVector Location)
{
	for (FFactionStruct& faction : Grid->Camera->ConquestManager->Factions) {
		for (ABuilding* building : faction.Buildings) {
			if (!building->IsA<AWork>() || Cast<AWork>(building)->ForcefieldRange == 0 || building->GetCitizensAtBuilding().IsEmpty())
				continue;

			double distance = FVector::Dist(building->GetActorLocation(), Location);

			if (distance <= Cast<AWork>(building)->ForcefieldRange)
				return true;
		}
	}

	return false;
}