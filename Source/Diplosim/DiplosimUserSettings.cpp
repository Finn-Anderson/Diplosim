#include "DiplosimUserSettings.h"

#include "NiagaraComponent.h"

#include "Map/Clouds.h"

UDiplosimUserSettings::UDiplosimUserSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	MaxEnemies = 333;
	MaxCitizens = 1000;

	bRenderClouds = true;

	Clouds = nullptr;
}

void UDiplosimUserSettings::SetMaxEnemies(int32 Value)
{
	MaxEnemies = Value;
}

int32 UDiplosimUserSettings::GetMaxEnemies() const
{
	return MaxEnemies;
}

void UDiplosimUserSettings::SetMaxCitizens(int32 Value)
{
	MaxCitizens = Value;
}

int32 UDiplosimUserSettings::GetMaxCitizens() const
{
	return MaxCitizens;
}

void UDiplosimUserSettings::SetRenderClouds(bool Value)
{
	bRenderClouds = Value;

	if (Clouds == nullptr)
		return;

	for (FCloudStruct cloud : Clouds->Clouds)
		cloud.CloudComponent->SetHiddenInGame(!Value);	
}

bool UDiplosimUserSettings::GetRenderClouds() const
{
	return bRenderClouds;
}

UDiplosimUserSettings* UDiplosimUserSettings::GetDiplosimUserSettings()
{
	return Cast<UDiplosimUserSettings>(UGameUserSettings::GetGameUserSettings());
}