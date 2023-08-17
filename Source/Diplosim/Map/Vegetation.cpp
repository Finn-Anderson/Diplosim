#include "Vegetation.h"

AVegetation::AVegetation()
{
	bIsGettingChopped = false;
}

void AVegetation::BeginPlay()
{
	Super::BeginPlay();

	if (MeshList.Num() > 0) {
		int32 i = FMath::RandRange(0, (MeshList.Num() - 1));

		ResourceMesh->SetMobility(EComponentMobility::Movable);

		ResourceMesh->SetStaticMesh(MeshList[i]);

		ResourceMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Overlap);

		ResourceMesh->SetMobility(EComponentMobility::Static);
	}
}

void AVegetation::YieldStatus()
{
	bIsGettingChopped = false;

	ResourceMesh->SetRelativeScale3D(FVector(0.1f, 0.1f, 0.1f));

	FTimerHandle growTimer;
	GetWorldTimerManager().SetTimer(growTimer, this, &AVegetation::Grow, 30.0f, false);
}

void AVegetation::Grow()
{
	ResourceMesh->SetRelativeScale3D(ResourceMesh->GetRelativeScale3D() + FVector(0.1f, 0.1f, 0.1f));

	if (!IsChoppable()) {
		FTimerHandle growTimer;
		GetWorldTimerManager().SetTimer(growTimer, this, &AVegetation::Grow, 30.0f, false);
	}
}

bool AVegetation::IsChoppable()
{
	if (bIsGettingChopped || ResourceMesh->GetRelativeScale3D() != FVector(1.0f, 1.0f, 1.0f))
		return false;

	return true;
}