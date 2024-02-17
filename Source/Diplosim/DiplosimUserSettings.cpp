#include "DiplosimUserSettings.h"

UDiplosimUserSettings::UDiplosimUserSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	MaxEnemies = 333;
	MaxCitizens = 1000;
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

UDiplosimUserSettings* UDiplosimUserSettings::GetDiplosimUserSettings()
{
	return Cast<UDiplosimUserSettings>(UGameUserSettings::GetGameUserSettings());
}