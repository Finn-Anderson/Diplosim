#include "Map/Atmosphere/NaturalDisasterComponent.h"

#include "Camera/CameraShakeSourceComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"

#include "Buildings/Building.h"
#include "Buildings/Work/Work.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/DiplosimTimerManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Universal/Resource.h"
#include "Universal/HealthComponent.h"
#include "Universal/Projectile.h"

UNaturalDisasterComponent::UNaturalDisasterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	Grid = nullptr;

	ResetDisasterChance();

	Frequency = 1.0f;
	Intensity = 1.0f;
}

bool UNaturalDisasterComponent::ShouldCreateDisaster()
{
	float chance = Grid->Camera->Stream.FRandRange(0.0f, 100.0f);

	if (DisasterChance <= chance)
		return false;

	return true;
}

void UNaturalDisasterComponent::IncrementDisasterChance()
{
	DisasterChance += 0.02f * Frequency;

	if (!ShouldCreateDisaster())
		return;

	float magnitude = Grid->Camera->Stream.FRandRange(1.0f, 5.0f) * Intensity;

	int32 type = Grid->Camera->Stream.RandRange(0, 2);
	FString text = "";

	if (type == 0) {
		GenerateEarthquake(magnitude);
		text = "Earthquake";
	}
	else if (type == 1) {
		GeneratePurifier(magnitude);
		text = "Purifier";
	}
	else {
		GenerateRedSun(magnitude);
		text = "Red Sun";
	}

	Grid->Camera->ShowEvent("Natural Disaster", text);

	ResetDisasterChance();
}

void UNaturalDisasterComponent::ResetDisasterChance()
{
	DisasterChance = 0.0f;
}

void UNaturalDisasterComponent::SetEarthquakeSounds(float Dilation)
{
	TArray<UAudioComponent*> components;
	Grid->GetComponents<UAudioComponent>(components);

	for (UAudioComponent* component : components) {
		if (!component->GetName().Contains("nd-audio"))
			continue;

		component->SetPaused(Dilation < 1.0f);
	}
}

void UNaturalDisasterComponent::GenerateEarthquake(float Magnitude)
{
	float range = 500.0f * Magnitude;

	FOverlapsStruct overlaps;
	overlaps.bBuildings = true;

	int32 count = 0;

	for (const FVector& point : GetEarthquakePoints(Magnitude)) {
		TArray<AActor*> actors = Grid->AIVisualiser->GetOverlaps(Grid->Camera, Grid, range, overlaps, EFactionType::Both, nullptr, point);

		for (AActor* actor : actors) {
			if (!actor->IsA<ABuilding>() || IsProtected(actor->GetActorLocation()))
				continue;

			float d = FVector::Dist(point, actor->GetActorLocation());
			int32 damage = FMath::Max(1.0f, range / (d / 10.0f));

			Cast<ABuilding>(actor)->HealthComponent->TakeHealth(damage, Grid);
		}

		UAudioComponent* audio = NewObject<UAudioComponent>(this, UAudioComponent::StaticClass(), FName("nd-audio" + FString::FromInt(count)));
		audio->SetupAttachment(Grid->GetRootComponent());
		audio->SetRelativeLocation(point);
		Grid->Camera->PlayAmbientSound(audio, Sound, 1.0f);
		if (Grid->Camera->CustomTimeDilation > 1.0f)
			audio->SetPaused(true);

		UCameraShakeSourceComponent* shake = NewObject<UCameraShakeSourceComponent>(this, UCameraShakeSourceComponent::StaticClass(), FName("nd-shake" + FString::FromInt(count)));
		shake->SetupAttachment(Grid->GetRootComponent());
		shake->SetRelativeLocation(point);
		shake->InnerAttenuationRadius = range;
		shake->OuterAttenuationRadius = range * 6;
		shake->StartCameraShake(Shake, 1.0f, ECameraShakePlaySpace::World);

		DrawDebugLine(GetWorld(), point + FVector(0.0f, 0.0f, 1000.0f), point, FColor::Red, false, 10.0f);

		count++;
	}

	Grid->Camera->TimerManager->CreateTimer("Clear Earthquake", Grid, 10.0f, "ClearEarthquakeShakesAndSounds", {}, false, true);
}

void UNaturalDisasterComponent::ClearEarthquakeShakesAndSounds()
{
	TArray<UActorComponent*> components;
	Grid->GetComponents(components);

	for (UActorComponent* component : components) {
		if (!component->GetName().Contains("nd-"))
			continue;

		if (component->IsA<UAudioComponent>())
			Cast<UAudioComponent>(component)->SetPaused(true);

		component->DestroyComponent();
	}
}

TArray<FVector> UNaturalDisasterComponent::GetEarthquakePoints(float Magnitude)
{
	auto bound = Grid->GetMapBounds();
	int32 distance = Magnitude * 5;

	int32 iX = Grid->Camera->Stream.RandRange(0, bound - 1);
	int32 iY = Grid->Camera->Stream.RandRange(0, bound - 1);

	FTileStruct start = Grid->Storage[iX][iY];

	TArray<FTileStruct> endLocations;

	for (TArray<FTileStruct>& row : Grid->Storage) {
		for (FTileStruct& tile : row) {
			if (FMath::Abs(tile.X - start.X) + FMath::Abs(tile.Y - start.Y) != distance)
				continue;

			endLocations.Add(tile);
		}
	}

	int32 chosenEnd = Grid->Camera->Stream.RandRange(0, endLocations.Num() - 1);

	FTileStruct end = endLocations[chosenEnd];

	TArray<FVector> points;

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

	p1 = FVector2D(Grid->Camera->Stream.RandRange(p0.X, pHalf.X) + Grid->Camera->Stream.RandRange(-variance, variance), Grid->Camera->Stream.RandRange(p0.Y, pHalf.Y) + Grid->Camera->Stream.RandRange(-variance, variance));
	p2 = FVector2D(Grid->Camera->Stream.RandRange(pHalf.X, p3.X) + Grid->Camera->Stream.RandRange(-variance, variance), Grid->Camera->Stream.RandRange(pHalf.Y, p3.Y) + Grid->Camera->Stream.RandRange(-variance, variance));

	double t = 0.0f;

	while (t <= 1.0f) {
		FVector2D point = FMath::Pow((1 - t), 3) * p0 + 3 * FMath::Pow((1 - t), 2) * t * p1 + 3 * (1 - t) * FMath::Pow(t, 2) * p2 + FMath::Pow(t, 3) * p3;

		int32 px = FMath::Clamp(point.X + (bound / 2), 0, bound - 1);
		int32 py = FMath::Clamp(point.Y + (bound / 2), 0, bound - 1);

		FVector pt = Grid->GetTransform(&Grid->Storage[px][py]).GetLocation();

		if (!points.Contains(pt))
			points.Add(pt);

		t += 0.02f;
	}

	return points;
}

void UNaturalDisasterComponent::GeneratePurifier(float Magnitude)
{
	int32 x = Grid->Camera->Stream.RandRange(0, Grid->Storage.Num() - 1);
	int32 y = Grid->Camera->Stream.RandRange(0, Grid->Storage[x].Num() - 1);

	FRotator rotation;
	rotation.Yaw = Grid->Camera->Stream.RandRange(0, 359);
	rotation.Pitch = Grid->Camera->Stream.RandRange(-90, -30);

	FVector location = Grid->GetTransform(&Grid->Storage[x][y]).GetLocation() - (rotation.Vector() * 10000.0f);

	AProjectile* projectile = GetWorld()->SpawnActor<AProjectile>(PurifierClass, location, rotation);
	projectile->ProjectileMesh->SetRelativeScale3D(FVector(Magnitude));
	projectile->SpawnNiagaraSystems(Grid->Camera);
	projectile->Radius *= Magnitude;
}

void UNaturalDisasterComponent::GenerateRedSun(float Magnitude)
{
	Grid->AtmosphereComponent->bRedSun = true;

	AlterSunGradually(0.15f, -0.02f);

	Grid->Camera->TimerManager->CreateTimer("Red Sun", Grid, 120.0f * Magnitude, "CancelRedSun", {}, false, true);
}

void UNaturalDisasterComponent::AlterSunGradually(float Target, float Increment)
{
	FLinearColor colour = Grid->AtmosphereComponent->Sun->GetLightColor();

	colour.G = FMath::Clamp(colour.G + Increment, 0.15f, 1.0f);
	colour.B = FMath::Clamp(colour.B + Increment, 0.15f, 1.0f);

	Grid->AtmosphereComponent->Sun->SetLightColor(colour);
	Grid->AtmosphereComponent->Moon->SetLightColor(colour);

	if (colour.G == Target)
		return;
	
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
			if (!building->IsA<AWork>() || Cast<AWork>(building)->ForcefieldRange == 0 || !building->ParticleComponent->IsActive())
				continue;

			double distance = FVector::Dist(building->GetActorLocation(), Location);

			if (distance <= Cast<AWork>(building)->ForcefieldRange)
				return true;
		}
	}

	return false;
}