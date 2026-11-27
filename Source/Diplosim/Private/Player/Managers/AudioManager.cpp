#include "Player/Managers/AudioManager.h"

#include "Components/AudioComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Camera/CameraComponent.h"

#include "AI/AI.h"
#include "AI/AIMovementComponent.h"
#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Map/Atmosphere/Clouds.h"
#include "Player/Camera.h"
#include "Player/Components/CameraMovementComponent.h"
#include "Universal/DiplosimUserSettings.h"

UAudioManager::UAudioManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	TMap<UAudioComponent**, FName> components;
	components.Add(&InteractAudioComponent, TEXT("InteractAudioComponent"));
	components.Add(&AmbientWindAudioComponent, TEXT("AmbientWindAudioComponent"));
	components.Add(&AmbientTreesAudioComponent, TEXT("AmbientTreesAudioComponent"));
	components.Add(&AmbientSeaAudioComponent, TEXT("AmbientSeaAudioComponent"));
	components.Add(&AmbientRainAudioComponent, TEXT("AmbientRainAudioComponent"));
	components.Add(&AmbientLavaAudioComponent, TEXT("AmbientLavaAudioComponent"));
	components.Add(&AmbientWaterfallAudioComponent, TEXT("AmbientWaterfallAudioComponent"));
	components.Add(&MusicAudioComponent, TEXT("MusicAudioComponent"));

	for (auto element : components) {
		auto component = CreateDefaultSubobject<UAudioComponent>(element.Value);
		*element.Key = component;
		component->SetAutoActivate(false);
		
		if (component == InteractAudioComponent || component == MusicAudioComponent)
			component->SetUISound(true);

		if (component == InteractAudioComponent)
			component->bCanPlayMultipleInstances = true;
		else
			component->SetVolumeMultiplier(0.0f);
	}

	EventSound = InteractSound = nullptr;
	Camera = nullptr;

	LastTransform = FTransform(FQuat::Identity, FVector::Zero());
	Counter = 0;

	WindSpeedPercentage = -1.0f;
}

void UAudioManager::SetupAttachment(USceneComponent* SceneComponent)
{
	InteractAudioComponent->SetupAttachment(SceneComponent);
	AmbientWindAudioComponent->SetupAttachment(SceneComponent);
	AmbientTreesAudioComponent->SetupAttachment(SceneComponent);
	AmbientSeaAudioComponent->SetupAttachment(SceneComponent);
	AmbientRainAudioComponent->SetupAttachment(SceneComponent);
	AmbientLavaAudioComponent->SetupAttachment(SceneComponent);
	AmbientWaterfallAudioComponent->SetupAttachment(SceneComponent);
	MusicAudioComponent->SetupAttachment(SceneComponent);
}

void UAudioManager::PlayAmbientSound(UAudioComponent* AudioComponent, USoundBase* Sound, float Pitch)
{
	if (Sound == nullptr || Camera->Grid->Storage.IsEmpty())
		return;

	Async(EAsyncExecution::TaskGraphMainTick, [this, AudioComponent, Sound, Pitch]() {
		float pitch = Pitch;

		if (pitch == -1.0f)
			pitch = Camera->Stream.FRandRange(0.8f, 1.2f);

		if (IsValid(AudioComponent->GetOwner()) && AudioComponent->GetOwner()->IsA<AAI>())
			AudioComponent->SetRelativeLocation(Cast<AAI>(AudioComponent->GetOwner())->MovementComponent->GetMovementTransform().GetLocation());

		AudioComponent->SetSound(Sound);
		AudioComponent->SetPitchMultiplier(pitch);
		AudioComponent->SetVolumeMultiplier(Camera->Settings->GetAmbientVolume() * Camera->Settings->GetMasterVolume());
		AudioComponent->Play();
	});
}

void UAudioManager::PlayInteractSound(USoundBase* Sound, float Pitch)
{
	InteractAudioComponent->SetSound(Sound);
	InteractAudioComponent->SetPitchMultiplier(Pitch);
	InteractAudioComponent->SetVolumeMultiplier(Camera->Settings->GetMasterVolume() * Camera->Settings->GetSFXVolume());

	InteractAudioComponent->Play();
}

void UAudioManager::CalculateAmbientEnvironmentSound(bool bForce)
{
	if (!AmbientWindAudioComponent->IsActive())
		AmbientWindAudioComponent->Activate();

	if (!AmbientTreesAudioComponent->IsActive())
		AmbientTreesAudioComponent->Activate();

	if (!AmbientSeaAudioComponent->IsActive())
		AmbientSeaAudioComponent->Activate();

	if (!AmbientRainAudioComponent->IsActive())
		AmbientRainAudioComponent->Activate();

	if (!Camera->Grid->bLava) {
		if (AmbientLavaAudioComponent->IsActive())
			AmbientLavaAudioComponent->Deactivate();
	}
	else if (!AmbientLavaAudioComponent->IsActive())
		AmbientLavaAudioComponent->Activate();

	if (!AmbientWaterfallAudioComponent->IsActive())
		AmbientWaterfallAudioComponent->Activate();

	Async(EAsyncExecution::TaskGraph, [this, bForce]() {
		FScopeTryLock lock(&AmbientLock);
		if (!lock.IsLocked())
			return;

		FTransform transform = Camera->CameraComponent->GetComponentTransform();
		if ((transform.GetLocation() - LastTransform.GetLocation()).IsNearlyZero(1.0f) && !bForce)
			return;

		if (Counter > 0 && !bForce) {
			Counter--;

			return;
		}

		LastTransform = transform;
		Counter = 20;

		float volume = Camera->Settings->GetAmbientVolume() * Camera->Settings->GetMasterVolume();

		// Wind
		AmbientWindAudioComponent->SetVolumeMultiplier((LastTransform.GetLocation().Z / Camera->MovementComponent->MaxLength) * volume * GetWindSpeedVolume());
		
		// Trees
		FVector closestPoint = FVector(100000000000.0f);
		for (const FResourceHISMStruct& treeStruct : Camera->Grid->TreeStruct) {
			if (!treeStruct.Resource->ResourceHISM)
				continue;

			for (int32 i = 0; i < treeStruct.Resource->ResourceHISM->GetInstanceCount(); i++) {
				if (!treeStruct.Resource->ResourceHISM->IsValidInstance(i))
					continue;

				FTransform t;
				treeStruct.Resource->ResourceHISM->GetInstanceTransform(i, t, true);

				if (FVector::Dist(t.GetLocation(), LastTransform.GetLocation()) > FVector::Dist(closestPoint, LastTransform.GetLocation()))
					continue;

				closestPoint = t.GetLocation();
			}
		}

		CalculateAmbientVolume(AmbientTreesAudioComponent, volume * GetWindSpeedVolume(), closestPoint, false);

		// Sea
		closestPoint = FVector(100000000000.0f);
		for (TArray<FTileStruct>& row : Camera->Grid->Storage) {
			for (FTileStruct& tile : row) {
				if (tile.Level != 0)
					continue;

				bool bSeaEdgeTile = tile.AdjacentTiles.Num() < 4;
				if (!bSeaEdgeTile) {
					for (auto& element : tile.AdjacentTiles) {
						if (element.Value->Level > -1)
							continue;

						bSeaEdgeTile = true;
						break;
					}
				}

				if (!bSeaEdgeTile)
					continue;

				FTransform t = Camera->Grid->GetTransform(&tile);

				if (FVector::Dist(t.GetLocation(), LastTransform.GetLocation()) > FVector::Dist(closestPoint, LastTransform.GetLocation()))
					continue;

				closestPoint = t.GetLocation();
			}
		}

		CalculateAmbientVolume(AmbientSeaAudioComponent, volume, closestPoint, false);

		// Rain
		closestPoint = FVector(100000000000.0f);
		for (const FCloudStruct& cloudStruct : Camera->Grid->AtmosphereComponent->Clouds->Clouds) {
			if (!cloudStruct.Precipitation)
				continue;

			for (int32 i = 0; i < cloudStruct.HISMCloud->GetInstanceCount(); i++) {
				if (!cloudStruct.HISMCloud->IsValidInstance(i))
					continue;

				FTransform t;
				cloudStruct.HISMCloud->GetInstanceTransform(i, t, true);

				if (FVector::DistXY(t.GetLocation(), LastTransform.GetLocation()) > FVector::DistXY(closestPoint, LastTransform.GetLocation()))
					continue;

				closestPoint = t.GetLocation();
			}
		}

		CalculateAmbientVolume(AmbientRainAudioComponent, volume, closestPoint, true);

		// Lava
		if (!Camera->Grid->bLava)
			return;

		closestPoint = FVector(100000000000.0f);
		for (FTileStruct* tile : Camera->Grid->LavaTiles) {
			FTransform t = Camera->Grid->GetTransform(tile);

			if (FVector::Dist(t.GetLocation(), LastTransform.GetLocation()) > FVector::Dist(closestPoint, LastTransform.GetLocation()))
				continue;

			closestPoint = t.GetLocation();
		}

		CalculateAmbientVolume(AmbientLavaAudioComponent, volume, closestPoint, false);

		// Waterfall
		if (!Camera->Grid->bLava)
			return;

		closestPoint = FVector(100000000000.0f);
		for (const FVector& location : Camera->Grid->WaterfallSpawnLocations) {
			if (FVector::Dist(location, LastTransform.GetLocation()) > FVector::Dist(closestPoint, LastTransform.GetLocation()))
				continue;

			closestPoint = location;
		}

		CalculateAmbientVolume(AmbientWaterfallAudioComponent, volume, closestPoint, false);
	});
}

void UAudioManager::AlterWindPitch(float WindSpeedPerc)
{
	bool recalc = WindSpeedPercentage != -1.0f ? true : false;

	WindSpeedPercentage = WindSpeedPerc;

	AmbientWindAudioComponent->SetPitchMultiplier(WindSpeedPercentage);
	AmbientTreesAudioComponent->SetPitchMultiplier(WindSpeedPercentage);

	if (recalc)
		CalculateAmbientEnvironmentSound(true);
}

void UAudioManager::ClearAmbientSound()
{
	AmbientWindAudioComponent->SetVolumeMultiplier(0.0f);
	AmbientTreesAudioComponent->SetVolumeMultiplier(0.0f);
	AmbientSeaAudioComponent->SetVolumeMultiplier(0.0f);
	AmbientRainAudioComponent->SetVolumeMultiplier(0.0f);
}

void UAudioManager::CalculateAmbientVolume(UAudioComponent* AudioComponent, float Volume, FVector ClosestPoint, bool bIgnoreZ)
{
	float d = 100000000.0f;
	if (bIgnoreZ)
		d = FVector::DistXY(LastTransform.GetLocation(), ClosestPoint);
	else
		d = FVector::Dist(LastTransform.GetLocation(), ClosestPoint);

	float distance = FMath::Min(1.0f / FMath::Pow(FMath::LogX(300, d), 5), 1.0f);
	AudioComponent->SetVolumeMultiplier(distance * Volume);
}

TArray<USoundBase*> UAudioManager::GetResourceSounds(AResource* Resource)
{
	for (const FCitizenResourceStruct& crs : CitizenResourceStruct) {
		if (!crs.Resources.Contains(Resource->GetClass()))
			continue;

		return crs.Sounds;
	}

	return TArray<USoundBase*>();
}

float UAudioManager::GetWindSpeedVolume()
{
	return WindSpeedPercentage * 1.25f;
}
