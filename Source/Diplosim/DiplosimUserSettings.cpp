#include "DiplosimUserSettings.h"

#include "NiagaraComponent.h"

#include "Map/Clouds.h"
#include "DiplosimGameModeBase.h"

UDiplosimUserSettings::UDiplosimUserSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bEnemies = true;

	bRenderClouds = true;

	Clouds = nullptr;

	GameMode = nullptr;
}

void UDiplosimUserSettings::SetSpawnEnemies(bool Value)
{
	bEnemies = Value;

	if (GameMode == nullptr)
		return;

	if (!GameMode->GetWorldTimerManager().IsTimerActive(GameMode->WaveTimer) && GameMode->CheckEnemiesStatus())
		GameMode->SetWaveTimer();
}

bool UDiplosimUserSettings::GetSpawnEnemies() const
{
	return bEnemies;
}

void UDiplosimUserSettings::SetRenderClouds(bool Value)
{
	bRenderClouds = Value;

	if (Clouds == nullptr)
		return;

	for (FCloudStruct cloudStruct : Clouds->Clouds)
		cloudStruct.Cloud->SetHiddenInGame(!Value);	
}

bool UDiplosimUserSettings::GetRenderClouds() const
{
	return bRenderClouds;
}

UDiplosimUserSettings* UDiplosimUserSettings::GetDiplosimUserSettings()
{
	return Cast<UDiplosimUserSettings>(UGameUserSettings::GetGameUserSettings());
}