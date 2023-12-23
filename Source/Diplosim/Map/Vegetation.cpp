#include "Vegetation.h"

#include "Buildings/Farm.h"

AVegetation::AVegetation()
{
	ResourceMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody, ECollisionResponse::ECR_Overlap);

	IntialScale = FVector(0.1f, 0.1f, 0.1f);

	MaxScale = FVector(1.0f, 1.0f, 1.0f);

	TimeLength = 30.0f;

	bVaryScale = false;
}

void AVegetation::BeginPlay()
{
	Super::BeginPlay();

	if (MeshList.Num() > 0) {
		int32 i = FMath::RandRange(0, (MeshList.Num() - 1));

		ResourceMesh->SetMobility(EComponentMobility::Movable);

		ResourceMesh->SetStaticMesh(MeshList[i]);

		if (bVaryScale) {
			int32 s = FMath::RandRange(-2, 2);
			float scale = (10.0f + s) / 10.0f;

			MaxScale = FVector(scale, scale, scale);

			ResourceMesh->SetRelativeScale3D(MaxScale);
		}

		ResourceMesh->SetMobility(EComponentMobility::Static);
	}

	SetQuantity(MaxYield);
}

void AVegetation::YieldStatus()
{
	ResourceMesh->SetRelativeScale3D(IntialScale);

	SetQuantity(0);

	FTimerHandle growTimer;
	GetWorldTimerManager().SetTimer(growTimer, this, &AVegetation::Grow, TimeLength / 10, false);
}

void AVegetation::Grow()
{
	FVector scale = ResourceMesh->GetRelativeScale3D();

	if (scale.X < 1.0f) {
		scale.X += 0.1f;
	}
	if (scale.Y < 1.0f) {
		scale.Y += 0.1f;
	}
	if (scale.Z < 1.0f) {
		scale.Z += 0.1f;
	}

	ResourceMesh->SetRelativeScale3D(scale);

	if (!IsChoppable()) {
		FTimerHandle growTimer;
		GetWorldTimerManager().SetTimer(growTimer, this, &AVegetation::Grow, TimeLength / 10, false);
	}
}

bool AVegetation::IsChoppable()
{
	FVector scale = ResourceMesh->GetRelativeScale3D();

	if (scale.X < MaxScale.X || scale.Y < MaxScale.Y || scale.Z < MaxScale.Z)
		return false;

	SetQuantity(MaxYield);

	if (Owner->IsValidLowLevelFast() && Owner->IsA<AFarm>()) {
		AFarm* farm = Cast<AFarm>(Owner);

		farm->ProductionDone();
	}

	return true;
}