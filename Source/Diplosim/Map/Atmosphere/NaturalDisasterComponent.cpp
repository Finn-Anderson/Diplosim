#include "Map/Atmosphere/NaturalDisasterComponent.h"

#include "Components/DirectionalLightComponent.h"
#include "Kismet/GameplayStatics.h"

#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Buildings/Building.h"
#include "Universal/Resource.h"
#include "Universal/HealthComponent.h"

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

	int32 iX = FMath::RandRange(0, bounds);
	int32 iY = FMath::RandRange(0, bounds);

	FTileStruct start = Grid->Storage[iX][iY];

	TArray<FTileStruct> endLocations;

	for (TArray<FTileStruct>& row : Grid->Storage) {
		for (FTileStruct& tile : row) {
			if (FMath::Abs(tile.X - start.X) + FMath::Abs(tile.Y - start.Y) != distance)
				continue;

			endLocations.Add(tile);
		}
	}

	int32 chosenEnd = FMath::RandRange(0, endLocations.Num() - 1);

	FTileStruct end = endLocations[chosenEnd];

	TArray<FTileStruct> points;

	auto bound = FMath::FloorToInt32(FMath::Sqrt((double)Grid->Size));

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

	p1 = FVector2D(FMath::RandRange(p0.X, pHalf.X) + FMath::RandRange(-variance, variance), FMath::RandRange(p0.Y, pHalf.Y) + FMath::RandRange(-variance, variance));
	p2 = FVector2D(FMath::RandRange(pHalf.X, p3.X) + FMath::RandRange(-variance, variance), FMath::RandRange(pHalf.Y, p3.Y) + FMath::RandRange(-variance, variance));

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

	TArray<FEarthquakeStruct> EarthquakeStructs;

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
			if (!actor->IsA<ABuilding>())
				continue;

			float d = FVector::Dist(estruct.Point, actor->GetActorLocation());

			estruct.BuildingsInRange.Add(Cast<ABuilding>(actor), d);
		}

		EarthquakeStructs.Add(estruct);
	}

	Grid->Camera->CitizenManager->CreateTimer("Earthquake", Grid, 4.0f * Magnitude, FTimerDelegate::CreateUObject(this, &UNaturalDisasterComponent::CalculateEarthquakeDamage, EarthquakeStructs, range), true);
}

void UNaturalDisasterComponent::CalculateEarthquakeDamage(TArray<FEarthquakeStruct> EarthquakeStructs, float Range)
{
	for (FEarthquakeStruct estruct : EarthquakeStructs) {
		for (auto& element : estruct.BuildingsInRange) {
			float damageChance = element.Value / Range;

			int32 chance = FMath::RandRange(0, 100);

			if (damageChance > chance)
				element.Key->HealthComponent->TakeHealth(25.0f, Grid);
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

}

void UNaturalDisasterComponent::GenerateRedSun(float Magnitude)
{
	Grid->AtmosphereComponent->bRedSun = true;

	AlterSunGradually(0.15f, -0.02f);

	Grid->Camera->CitizenManager->CreateTimer("Red Sun", Grid, 120.0f * Magnitude, FTimerDelegate::CreateUObject(this, &UNaturalDisasterComponent::CancelRedSun), false);
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