#include "AI/Projectile.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"

#include "Universal/HealthComponent.h"
#include "Enemy.h"
#include "Buildings/Work/Defence/Tower.h"
#include "AttackComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/ConquestManager.h"
#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Map/Atmosphere/NaturalDisasterComponent.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AIMesh"));
	ProjectileMesh->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	ProjectileMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	ProjectileMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody, ECollisionResponse::ECR_Overlap);
	ProjectileMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->bCastDynamicShadow = true;
	ProjectileMesh->CastShadow = true;

    ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
    ProjectileMovementComponent->SetUpdatedComponent(ProjectileMesh);
    ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->InitialSpeed = 600.0f;
    ProjectileMovementComponent->MaxSpeed = 600.0f;

	Damage = 10;
	Radius = 150.0f;

	bExplode = false;
	bDamageFallOff = true;
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	ProjectileMesh->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
}

void AProjectile::SpawnNiagaraSystems(AActor* Launcher)
{
	SetOwner(Launcher);

	UNiagaraComponent* trail = nullptr;

	if (TrailSystem)
		trail = UNiagaraFunctionLibrary::SpawnSystemAttached(TrailSystem, ProjectileMesh, FName("Trail1"), FVector(0.0f), FRotator(0.0f), EAttachLocation::Type::KeepRelativeOffset, true);

	if (Launcher->IsA<ATower>()) {
		FLinearColor colour = Cast<ATower>(Launcher)->Colour;

		ProjectileMesh->SetCustomPrimitiveDataFloat(0, colour.R);
		ProjectileMesh->SetCustomPrimitiveDataFloat(1, colour.G);
		ProjectileMesh->SetCustomPrimitiveDataFloat(2, colour.B);

		if (TrailSystem)
			trail->SetVariableLinearColor(TEXT("Colour"), colour);
	}
}

void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) 
{
	if (OtherActor == GetOwner())
		return;

	int32 multiplier = 1.0f; 
	
	if (GetOwner()->IsA<AAI>())
		multiplier = Cast<AAI>(GetOwner())->AttackComponent->DamageMultiplier;

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	ACamera* camera = PController->GetPawn<ACamera>();

	TArray<TEnumAsByte<EObjectTypeQuery>> objects;

	TArray<AActor*> ignore;
	ignore.Add(camera->Grid);

	for (FResourceHISMStruct resourceStruct : camera->Grid->MineralStruct)
		ignore.Add(resourceStruct.Resource);

	for (FResourceHISMStruct resourceStruct : camera->Grid->FlowerStruct)
		ignore.Add(resourceStruct.Resource);

	FFactionStruct* faction = camera->ConquestManager->GetFaction("", GetOwner());

	if (faction != nullptr)
		ignore.Append(faction->Buildings);

	TArray<AActor*> actors;

	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), GetActorLocation(), Radius, objects, nullptr, ignore, actors);

	UHealthComponent* healthComp = nullptr;

	if (bExplode) {
		for (AActor* actor : actors) {
			if (actor->IsA<ABuilding>())
				continue;

			healthComp = actor->GetComponentByClass<UHealthComponent>();

			if (!healthComp || healthComp->GetHealth() == 0)
				continue;

			int32 dmg = Damage * multiplier;

			if (bDamageFallOff) {
				float distance = FVector::Dist(GetActorLocation(), actor->GetActorLocation());

				dmg /= FMath::Pow(FMath::LogX(50.0f, distance), 5.0f);
			}
			else if (GetOwner()->IsA<UNaturalDisasterComponent>() && camera->Grid->AtmosphereComponent->NaturalDisasterComponent->IsProtected(actor->GetActorLocation()))
				continue;

			healthComp->TakeHealth(dmg, this);
		}

		UNiagaraComponent* explosion = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ExplosionSystem, GetActorLocation());

		if (GetOwner()->IsA<ATower>())
			explosion->SetVariableLinearColor(TEXT("Colour"), Cast<ATower>(GetOwner())->Colour);

		UGameplayStatics::PlayWorldCameraShake(GetWorld(), Shake, GetActorLocation(), 0.0f, Radius * 6, 1.0f);
	}
	else {
		healthComp = OtherActor->GetComponentByClass<UHealthComponent>();

		if (healthComp)
			healthComp->TakeHealth(Damage * multiplier, this);
	}

	Destroy();
}