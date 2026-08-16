#include "AI/Bird.h"

#include "GameFramework/ProjectileMovementComponent.h"

#include "AI/AIMovementComponent.h"
#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/AudioManager.h"
#include "Player/Managers/DiplosimTimerManager.h"
#include "Universal/AttackComponent.h"
#include "Universal/Projectile.h"
#include "Universal/HealthComponent.h"

ABird::ABird()
{

}

void ABird::SetupBird()
{
	Camera->TimerManager->CreateTimer("Poo", this, GetPooTimer(), "Poo", {}, false, true);

	Camera->TimerManager->CreateTimer("Chirp", this, GetChirpTimer(), "Chirp", {}, false, true);
}

void ABird::Poo()
{
	FVector startLoc = MovementComponent->Transform.GetLocation();
	FVector velocity = MovementComponent->Velocity + FVector(0.0f, 0.0f, -100.0f);

	AProjectile* projectile = GetWorld()->SpawnActor<AProjectile>(AttackComponent->ProjectileClass, startLoc, velocity.Rotation());
	projectile->SpawnNiagaraSystems(this);
	projectile->ProjectileMovementComponent->Velocity = velocity;

	Camera->TimerManager->CreateTimer("Poo", this, GetPooTimer(), "Poo", {}, false, true);
}

void ABird::Chirp()
{
	USoundBase* sound = Chirps[Camera->Stream.RandRange(0, Chirps.Num() - 1)];

	Camera->AudioManager->PlayAmbientSound(HealthComponent->HitAudioComponent, sound, VoicePitch);

	Camera->TimerManager->CreateTimer("Chirp", this, GetChirpTimer(), "Chirp", {}, false, true);
}

int32 ABird::GetPooTimer()
{
	int32 timeToCompleteDay = Camera->Grid->AtmosphereComponent->GetTimeToCompleteDay();

	return Camera->Stream.RandRange(timeToCompleteDay / 8, timeToCompleteDay / 4);
}

int32 ABird::GetChirpTimer()
{
	return Camera->Stream.RandRange(1, 20);
}

// Avoidance (tie to conquest AI fight calculation)