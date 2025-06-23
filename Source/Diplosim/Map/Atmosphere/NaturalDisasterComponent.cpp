#include "Map/Atmosphere/NaturalDisasterComponent.h"

#include "Components/DirectionalLightComponent.h"

#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"

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
		GenerateMeteor(magnitude);
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

}

void UNaturalDisasterComponent::CancelEarthquake()
{

}

void UNaturalDisasterComponent::GenerateMeteor(float Magnitude)
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