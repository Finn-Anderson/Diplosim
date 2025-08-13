#include "Buildings/Work/Defence/Trap.h"

#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/DecalComponent.h"

#include "AI/Enemy.h"
#include "Universal/HealthComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"

ATrap::ATrap()
{
	Range = 150.0f;
	ActivateRange = 100.0f;

	DecalComponent->SetVisibility(true);

	Damage = 300;

	FuseTime = 1.0f;

	bExplode = false;
}

void ATrap::ShouldStartTrapTimer(AActor* Actor)
{
	if (Camera->CitizenManager->DoesTimerExist("Trap", this))
		return;

	float distance = FVector::Dist(GetActorLocation(), Actor->GetActorLocation());

	if (distance > ActivateRange)
		return;

	if (FuseTime == 0.0f)
		ActivateTrap();
	else
		Camera->CitizenManager->CreateTimer("Trap", this, FuseTime, FTimerDelegate::CreateUObject(this, &ATrap::ActivateTrap), false, true);
}

void ATrap::ActivateTrap()
{
	TArray<AActor*> actorsInRange = Camera->Grid->AIVisualiser->GetOverlaps(Camera, this, Range);

	for (AActor* actor : actorsInRange) {
		UHealthComponent* healthComp = actor->GetComponentByClass<UHealthComponent>();

		if (!healthComp)
			continue;

		int32 dmg = Damage;

		if (bExplode) {
			FVector location = actor->GetActorLocation();

			if (actor->IsA<ABuilding>())
				Cast<ABuilding>(actor)->BuildingMesh->GetClosestPointOnCollision(GetActorLocation(), location);

			float distance = FVector::Dist(GetActorLocation(), actor->GetActorLocation());

			 dmg /= FMath::Pow(FMath::LogX(50.0f, distance), 5.0f);
		}

		healthComp->TakeHealth(dmg, this);
	}

	UGameplayStatics::PlayWorldCameraShake(GetWorld(), Shake, GetActorLocation(), 0.0f, 1000.0f, 1.0f);

	ParticleComponent->Activate();

	float height = BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize().Z + 1.0f;

	SetActorLocation(GetActorLocation() - FVector(0.0f, 0.0f, height));
	DestructionComponent->SetRelativeLocation(DestructionComponent->GetRelativeLocation() + FVector(0.0f, 0.0f, height));
	GroundDecalComponent->SetRelativeLocation(GroundDecalComponent->GetRelativeLocation() + FVector(0.0f, 0.0f, height));

	HealthComponent->Health = 0;
	HealthComponent->Clear(this);
}