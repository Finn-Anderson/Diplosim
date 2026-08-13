#include "Player/Managers/AudioManager.h"

#include "Components/AudioComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Camera/CameraComponent.h"

#include "AI/AI.h"
#include "AI/AIMovementComponent.h"
#include "Map/Grid.h"
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
	components.Add(&MusicAudioComponent, TEXT("MusicAudioComponent"));

	for (auto element : components) {
		auto component = CreateDefaultSubobject<UAudioComponent>(element.Value);
		*element.Key = component;
		
		if (component == InteractAudioComponent || component == MusicAudioComponent)
			component->SetUISound(true);

		if (component == InteractAudioComponent) {
			component->SetAutoActivate(false);
			component->bCanPlayMultipleInstances = true;
		}
		else {
			component->SetVolumeMultiplier(0.0f);
			component->SetAutoActivate(true);
		}
	}
}

void UAudioManager::SetupAttachment(ACamera* Cmra)
{
	Camera = Cmra;

	InteractAudioComponent->SetupAttachment(Camera->CameraComponent);
	AmbientWindAudioComponent->SetupAttachment(Camera->CameraComponent);
	AmbientTreesAudioComponent->SetupAttachment(Camera->CameraComponent);
	AmbientSeaAudioComponent->SetupAttachment(Camera->CameraComponent);
	MusicAudioComponent->SetupAttachment(Camera->CameraComponent);
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
	InteractAudioComponent->SetVolumeMultiplier(Camera->Settings->GetMasterVolume() * Camera->Settings->GetSFXVolume());

	InteractAudioComponent->Play();
}

void UAudioManager::CalculateAmbientEnvironmentSound()
{
	FScopeTryLock lock(&AmbientLock);
	if (!lock.IsLocked())
		return;

	Async(EAsyncExecution::TaskGraph, [this]() {
		FScopeTryLock lock(&AmbientLock);
		if (!lock.IsLocked())
			return;

		// Wind
		FVector location = Camera->CameraComponent->GetComponentLocation();
		float volume = Camera->Settings->GetAmbientVolume() * Camera->Settings->GetMasterVolume();

		AmbientWindAudioComponent->SetVolumeMultiplier((location.Z / Camera->MovementComponent->MaxLength) * volume);
		
		// Trees
		FVector closestPoint = FVector(100000000.0f);
		for (FResourceHISMStruct treeStruct : Camera->Grid->TreeStruct) {
			if (!treeStruct.Resource->ResourceHISM)
				continue;

			for (int32 i = 0; i < treeStruct.Resource->ResourceHISM->GetInstanceCount(); i++) {
				if (!treeStruct.Resource->ResourceHISM->IsValidInstance(i))
					continue;

				FTransform transform;
				treeStruct.Resource->ResourceHISM->GetInstanceTransform(i, transform);

				if (FVector::Dist(transform.GetLocation(), location) > FVector::Dist(closestPoint, location))
					continue;

				closestPoint = transform.GetLocation();
			}
		}

		AmbientTreesAudioComponent->SetWorldLocation(closestPoint);

		// Sea
		Camera->Grid->HISMSea->GetClosestPointOnCollision(location, closestPoint);

		FCollisionQueryParams params;
		params.AddIgnoredComponent(Camera->Grid->HISMSea);
		params.AddIgnoredActor(Camera);

		if (GetWorld()->LineTraceTestByChannel(closestPoint, closestPoint + FVector(0.0f, 0.0f, Camera->Grid->MaxLevel * 75.0f + 100.0f), ECollisionChannel::ECC_GameTraceChannel1, params)) {
			closestPoint = FVector(100000000.0f);
			for (TArray<FTileStruct>& row : Camera->Grid->Storage) {
				for (FTileStruct& tile : row) {
					if (!tile.bEdge || tile.Level != 0)
						continue;

					FTransform transform = Camera->Grid->GetTransform(&tile);

					if (FVector::Dist(transform.GetLocation(), location) > FVector::Dist(closestPoint, location))
						continue;

					closestPoint = transform.GetLocation();
				}
			}
		}

		AmbientSeaAudioComponent->SetWorldLocation(closestPoint);
	});
}

void UAudioManager::AlterWindPitch(float WindSpeedPercentage)
{
	float pitch = 1.0f + (WindSpeedPercentage * 0.5f);
	AmbientWindAudioComponent->SetPitchMultiplier(pitch);
	AmbientTreesAudioComponent->SetPitchMultiplier(pitch);
}