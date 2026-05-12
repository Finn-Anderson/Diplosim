#include "Universal/Projectile.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/SphereComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

#include "AI/AI.h"
#include "Buildings/Work/Defence/Tower.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Map/Atmosphere/NaturalDisasterComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/ConquestManager.h"
#include "Universal/AttackComponent.h"
#include "Universal/HealthComponent.h"

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
	FVector size = ProjectileMesh->GetStaticMesh()->GetBounds().GetBox().GetSize() / 2.0f;

	if (GetWorld()->SweepSingleByChannel(hit, GetActorLocation(), GetActorLocation(), (ProjectileMesh->GetRelativeRotation() + FRotator(90.0f, 0.0f, 0.0f)).Quaternion(), ECC_Pawn, FCollisionShape::MakeCapsule(size.Z, size.X)))
		OnHit(hit.GetActor(), hit.GetComponent(), hit.Item);
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
	if (Actor == GetOwner())
		return;

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	ACamera* camera = PController->GetPawn<ACamera>();

	if (bExplode)
		Explode(camera);
	else if (Component->IsA<UInstancedStaticMeshComponent>())
		HitActor(camera, camera->Grid->AIVisualiser->GetHISMAI(camera, Cast<UInstancedStaticMeshComponent>(Component), Instance));

	Destroy();
}

void AProjectile::Explode(ACamera* Camera)
{
	TArray<TEnumAsByte<EObjectTypeQuery>> objects;

	TArray<AActor*> ignore;
	ignore.Add(Camera->Grid);

	for (FResourceHISMStruct resourceStruct : Camera->Grid->MineralStruct)
		ignore.Add(resourceStruct.Resource);

	for (FResourceHISMStruct resourceStruct : Camera->Grid->FlowerStruct)
		ignore.Add(resourceStruct.Resource);

	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", GetOwner());

	if (faction != nullptr)
		ignore.Append(faction->Buildings);

	TArray<AActor*> actors;
	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), GetActorLocation(), Radius, objects, nullptr, ignore, actors);

	for (AActor* actor : actors) {
		if (GetOwner()->IsA<UNaturalDisasterComponent>() && Camera->Grid->AtmosphereComponent->NaturalDisasterComponent->IsProtected(actor->GetActorLocation()))
			continue;

		HitActor(Camera, actor);
	}

	UNiagaraComponent* explosion = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ExplosionSystem, GetActorLocation());
	if (GetOwner()->IsA<ATower>())
		explosion->SetVariableLinearColor(TEXT("Colour"), Cast<ATower>(GetOwner())->ChosenColour);

	AudioComponent->Play();

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

	healthComponent->TakeHealth(dmg * multiplier, this, sound);
}