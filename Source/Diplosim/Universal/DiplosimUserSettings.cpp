#include "DiplosimUserSettings.h"

#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/AudioComponent.h"

#include "Map/Atmosphere/Clouds.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "DiplosimGameModeBase.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "AI/Citizen.h"
#include "Buildings/Building.h"

UDiplosimUserSettings::UDiplosimUserSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bEnemies = true;

	bRenderClouds = true;
	bRenderFog = true;

	AAName = "None";

	GIName = "None";

	bRayTracing = false;

	bMotionBlur = false;

	ScreenPercentage = 100;

	MasterVolume = 1.0f;
	SFXVolume = 1.0f;
	AmbientVolume = 1.0f;

	Atmosphere = nullptr;
	Clouds = nullptr;

	GameMode = nullptr;
}

void UDiplosimUserSettings::SetVisualSettings()
{
	SetRenderClouds(GetRenderClouds());
	SetRenderFog(GetRenderFog());
	SetAA(GetAA());
	SetGI(GetGI());
	SetRayTracing(GetRayTracing());
	SetMotionBlur(GetMotionBlur());
	SetScreenPercentage(GetScreenPercentage());
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
	if (bRenderClouds == Value)
		return;

	bRenderClouds = Value;

	if (Clouds == nullptr)
		return;
	
	if (bRenderClouds) {
		Clouds->SetComponentTickEnabled(true);

		Clouds->ActivateCloud();
	}
	else {
		Clouds->Clear();

		Clouds->SetComponentTickEnabled(false);
	}
}

bool UDiplosimUserSettings::GetRenderClouds() const
{
	return bRenderClouds;
}

void UDiplosimUserSettings::SetRenderFog(bool Value)
{
	bRenderFog = Value;

	if (Atmosphere == nullptr)
		return;

	if (bRenderFog)
		Atmosphere->Fog->SetHiddenInGame(false);
	else
		Atmosphere->Fog->SetHiddenInGame(true);
}

bool UDiplosimUserSettings::GetRenderFog() const
{
	return bRenderFog;
}

void UDiplosimUserSettings::SetAA(FString Value)
{
	AAName = Value;

	if (AAName == "None")
		GEngine->Exec(GetWorld(), TEXT("r.AntiAliasingMethod 0"));
	else if (AAName == "TAA")
		GEngine->Exec(GetWorld(), TEXT("r.AntiAliasingMethod 2"));
	else
		GEngine->Exec(GetWorld(), TEXT("r.AntiAliasingMethod 4"));
}

FString UDiplosimUserSettings::GetAA() const
{
	return AAName;
}

void UDiplosimUserSettings::SetGI(FString Value)
{
	GIName = Value;

	if (GIName == "None") {
		GEngine->Exec(GetWorld(), TEXT("r.DynamicGlobalIlluminationMethod 0"));
		GEngine->Exec(GetWorld(), TEXT("r.ReflectionMethod 0"));
	}
	else if (GIName == "Lumen") {
		GEngine->Exec(GetWorld(), TEXT("r.DynamicGlobalIlluminationMethod 1"));
		GEngine->Exec(GetWorld(), TEXT("r.ReflectionMethod 1"));
	}
	else {
		GEngine->Exec(GetWorld(), TEXT("r.DynamicGlobalIlluminationMethod 2"));
		GEngine->Exec(GetWorld(), TEXT("r.ReflectionMethod 2"));
	}
}

FString UDiplosimUserSettings::GetGI() const
{
	return GIName;
}

void UDiplosimUserSettings::SetRayTracing(bool Value)
{
	bRayTracing = Value;

	if (bRayTracing)
		GEngine->Exec(GetWorld(), TEXT("r.RayTracing.Enable 1"));
	else
		GEngine->Exec(GetWorld(), TEXT("r.RayTracing.Enable 0"));
}

bool UDiplosimUserSettings::GetRayTracing() const
{
	return bRayTracing;
}

void UDiplosimUserSettings::SetMotionBlur(bool Value)
{
	bMotionBlur = Value;

	if (bMotionBlur)
		GEngine->Exec(GetWorld(), TEXT("r.MotionBlur.Amount 0.39"));
	else
		GEngine->Exec(GetWorld(), TEXT("r.MotionBlur.Amount 0"));
}

bool UDiplosimUserSettings::GetMotionBlur() const
{
	return bMotionBlur;
}

void UDiplosimUserSettings::SetScreenPercentage(int32 Value)
{
	ScreenPercentage = Value;

	FString cmd = "r.ScreenPercentage " + FString::FromInt(ScreenPercentage);

	GEngine->Exec(GetWorld(), *cmd);
}

int32 UDiplosimUserSettings::GetScreenPercentage() const
{
	return ScreenPercentage;
}

void UDiplosimUserSettings::SetMasterVolume(float Value)
{
	if (MasterVolume == Value)
		return;

	MasterVolume = Value;

	UpdateAmbientVolume();
}

float UDiplosimUserSettings::GetMasterVolume() const
{
	return MasterVolume;
}

void UDiplosimUserSettings::SetSFXVolume(float Value)
{
	if (SFXVolume == Value)
		return;

	SFXVolume = Value;
}

float UDiplosimUserSettings::GetSFXVolume() const
{
	return SFXVolume;
}

void UDiplosimUserSettings::SetAmbientVolume(float Value)
{
	if (AmbientVolume == Value)
		return;

	AmbientVolume = Value;

	UpdateAmbientVolume();
}

float UDiplosimUserSettings::GetAmbientVolume() const
{
	return AmbientVolume;
}

void UDiplosimUserSettings::UpdateAmbientVolume()
{
	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	if (!PController)
		return;

	ACamera* camera = PController->GetPawn<ACamera>();

	for (ACitizen* citizen : camera->CitizenManager->Citizens)
		citizen->AmbientAudioComponent->SetVolumeMultiplier(GetAmbientVolume() * GetMasterVolume());

	for (ABuilding* building : camera->CitizenManager->Buildings)
		building->AmbientAudioComponent->SetVolumeMultiplier(GetAmbientVolume() * GetMasterVolume());
}

UDiplosimUserSettings* UDiplosimUserSettings::GetDiplosimUserSettings()
{
	return Cast<UDiplosimUserSettings>(UGameUserSettings::GetGameUserSettings());
}