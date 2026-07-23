#include "Universal/Projectile.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/SphereComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

#include "AI/AI.h"
#include "AI/AIMovementComponent.h"
#include "Buildings/Work/Defence/Tower.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Map/AIInstancedStaticMeshComponent.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Map/Atmosphere/NaturalDisasterComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/ConquestManager.h"
#include "Universal/AttackComponent.h"
#include "Universal/HealthComponent.h"
#include "Universal/DiplosimUserSettings.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickInterval(1/60.0f);

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AIMesh"));
	ProjectileMesh->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	ProjectileMesh->bCastDynamicShadow = true;
	ProjectileMesh->CastShadow = true;

	RootComponent = ProjectileMesh;

    ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
    ProjectileMovementComponent->SetUpdatedComponent(ProjectileMesh);
    ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->InitialSpeed = 600.0f;
    ProjectileMovementComponent->MaxSpeed = 600.0f;

	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	AudioComponent->SetupAttachment(ProjectileMesh);
	AudioComponent->PitchModulationMin = 0.9f;
	AudioComponent->PitchModulationMax = 1.1f;
	AudioComponent->bAutoDestroy = false;

	Damage = 10;
	Radius = 150.0f;

	bExplode = false;
	bDamageFallOff = true;

	FactionName = "";
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (DeltaTime < 0.001f || DeltaTime > 1.0f)
		return;

	FHitResult hit;
	FVector size = ProjectileMesh->GetStaticMesh()->GetBounds().GetBox().GetSize() / 2.0f * ProjectileMesh->GetRelativeScale3D();

	if (GetWorld()->SweepSingleByChannel(hit, GetActorLocation(), GetActorLocation(), (ProjectileMesh->GetRelativeRotation() + FRotator(90.0f, 0.0f, 0.0f)).Quaternion(), ECC_Pawn, FCollisionShape::MakeCapsule(size.Z, size.X)))
		OnHit(hit.GetActor(), hit.GetComponent(), hit.Item); 

	if (IsValid(MovementShake))
		UGameplayStatics::PlayWorldCameraShake(GetWorld(), MovementShake, GetActorLocation(), 0.0f, Radius, 1.0f);
}

void AProjectile::SpawnNiagaraSystems(AActor* Launcher)
{
	SetOwner(Launcher);

	UNiagaraComponent* trail = nullptr;

	if (TrailSystem)
		trail = UNiagaraFunctionLibrary::SpawnSystemAttached(TrailSystem, ProjectileMesh, FName("Trail1"), FVector(0.0f), FRotator(0.0f), EAttachLocation::Type::KeepRelativeOffset, true);

	if (!Launcher->IsA<ATower>())
		return;

	FLinearColor colour = Cast<ATower>(Launcher)->ChosenColour;

	ProjectileMesh->SetCustomPrimitiveDataFloat(0, colour.R);
	ProjectileMesh->SetCustomPrimitiveDataFloat(1, colour.G);
	ProjectileMesh->SetCustomPrimitiveDataFloat(2, colour.B);

	if (TrailSystem)
		trail->SetVariableLinearColor(TEXT("Colour"), colour);
}

void AProjectile::OnHit(AActor* Actor, UActorComponent* Component, int32 Instance)
{
	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	ACamera* camera = PController->GetPawn<ACamera>();

	AAI* ai = camera->Grid->AIVisualiser->GetHISMAI(camera, Cast<UAIInstancedStaticMeshComponent>(Component), Instance);

	if (Actor->GetClass() == GetOwner()->GetClass() || (IsValid(ai) && ai->GetClass() == GetOwner()->GetClass()))
		return;

	if (bExplode)
		Explode(camera);
	else if (Component->IsA<UAIInstancedStaticMeshComponent>())
		HitActor(camera, ai);

	Destroy();
}

void AProjectile::Explode(ACamera* Camera)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", GetOwner());
	EFactionType type = EFactionType::Both;
	if (faction != nullptr)
		type = EFactionType::Different;

	FOverlapsStruct overlaps;
	overlaps.GetEverythingWithHealth();

	TArray<AActor*> actors = Camera->Grid->AIVisualiser->GetOverlaps(Camera, GetOwner(), Radius, overlaps, type, faction, GetActorLocation());

	for (AActor* actor : actors) {
		if (GetOwner()->IsA<ACamera>() && Camera->Grid->AtmosphereComponent->NaturalDisasterComponent->IsProtected(Camera->GetTargetActorLocation(actor)))
			continue;

		HitActor(Camera, actor);
	}

	UNiagaraComponent* explosion = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ExplosionSystem, GetActorLocation());
	if (GetOwner()->IsA<ATower>())
		explosion->SetVariableLinearColor(TEXT("Colour"), Cast<ATower>(GetOwner())->ChosenColour);

	explosion->SetVariableFloat(TEXT("Scale"), ProjectileMesh->GetRelativeScale3D().X);

	UGameplayStatics::PlaySoundAtLocation(GetWorld(), AudioComponent->GetSound(), GetActorLocation(), Camera->Settings->GetMasterVolume() * Camera->Settings->GetAmbientVolume());

	UGameplayStatics::PlayWorldCameraShake(GetWorld(), Shake, GetActorLocation(), 0.0f, Radius * 6, 1.0f);
}

void AProjectile::HitActor(ACamera* Camera, AActor* Actor)
{
	if (!IsValid(Actor) || Camera->ConquestManager->GetFactionFromActor(Actor).Name == FactionName)
		return;

	UHealthComponent* healthComponent = Actor->GetComponentByClass<UHealthComponent>();

	if (!healthComponent || healthComponent->GetHealth() == 0)
		return;

	float multiplier = 1.0f;
	float dmg = Damage;
	USoundBase* sound = nullptr;

	UAttackComponent* attackComponent = GetOwner()->FindComponentByClass<UAttackComponent>();

	if (attackComponent) {
		multiplier = attackComponent->DamageMultiplier;
		sound = attackComponent->OnHitSound;
	}

	if (bDamageFallOff) {
		float distance = FVector::Dist(GetActorLocation(), Actor->GetActorLocation());

		dmg /= FMath::Pow(FMath::LogX(50.0f, distance), 5.0f);
	}

	healthComponent->TakeHealth(dmg * multiplier, GetOwner(), sound);
}