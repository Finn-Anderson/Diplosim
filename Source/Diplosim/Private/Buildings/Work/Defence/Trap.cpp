#include "Buildings/Work/Defence/Trap.h"

#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/DecalComponent.h"

#include "AI/AI.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Player/Camera.h"
#include "Player/Managers/DiplosimTimerManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Universal/HealthComponent.h"

ATrap::ATrap()
{
	Range = 150.0f;
	ActivateRange = 100.0f;

	DecalComponent->DecalSize = FVector(Range);
	DecalComponent->SetVisibility(true);

	Damage = 300;

	FuseTime = 1.0f;

	bExplode = false;
}

void ATrap::StartTrapFuse()
{
	if (FuseTime == 0.0f)
		ActivateTrap();
	else {
		Camera->TimerManager->CreateTimer("Trap", this, FuseTime, "ActivateTrap", {}, false, true);

		if (bExplode)
			BuildingMesh->SetOverlayMaterial(ExplosionTickOverlay);
	}
}

void ATrap::ActivateTrap()
{
	FOverlapsStruct requestedOverlaps;
	requestedOverlaps.GetEverythingWithHealth();

	TArray<AActor*> actorsInRange = Camera->Grid->AIVisualiser->GetOverlaps(Camera, this, Range, requestedOverlaps, EFactionType::Both);

	for (AActor* actor : actorsInRange) {
		UHealthComponent* healthComp = actor->GetComponentByClass<UHealthComponent>();

		if (!healthComp)
			continue;

		float dmg = Damage;

		if (bExplode) {
			FVector location = Camera->GetTargetActorLocation(actor, false);

			if (actor->IsA<ABuilding>())
				Cast<ABuilding>(actor)->BuildingMesh->GetClosestPointOnCollision(GetActorLocation(), location);

			float distance = FVector::Dist(GetActorLocation(), location);

			dmg /= FMath::Pow(FMath::LogX(50.0f, distance), 8.0f);
		}

		healthComp->TakeHealth(dmg, this);
	}

	UGameplayStatics::PlayWorldCameraShake(GetWorld(), Shake, GetActorLocation(), 0.0f, 500.0f, 1.0f);

	ParticleComponent->Activate();

	float z = BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize().Z + 1.0f;

	TArray<UStaticMeshComponent*> meshes;
	GetComponents<UStaticMeshComponent>(meshes);

	for (UStaticMeshComponent* mesh : meshes)
		mesh->SetCustomPrimitiveDataFloat(8, -z);

	HealthComponent->Health = 0;
	HealthComponent->Clear(this);

	BuildingMesh->SetOverlayMaterial(nullptr);
}