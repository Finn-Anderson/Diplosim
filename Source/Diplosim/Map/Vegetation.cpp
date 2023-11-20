#include "Vegetation.h"

#include "Buildings/Farm.h"

AVegetation::AVegetation()
{
	IntialScale = FVector(0.1f, 0.1f, 0.1f);

	TimeLength = 30.0f;
}

void AVegetation::BeginPlay()
{
	Super::BeginPlay();

	if (MeshList.Num() > 0) {
		int32 i = FMath::RandRange(0, (MeshList.Num() - 1));

		ResourceMesh->SetMobility(EComponentMobility::Movable);

		ResourceMesh->SetStaticMesh(MeshList[i]);

		ResourceMesh->SetMobility(EComponentMobility::Static);
	}

	SetQuantity(MaxYield);
}

void AVegetation::YieldStatus()
{
	ResourceMesh->SetRelativeScale3D(IntialScale);

	SetQuantity(0);

	FTimerHandle growTimer;
	GetWorldTimerManager().SetTimer(growTimer, this, &AVegetation::Grow, TimeLength, false);
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
		GetWorldTimerManager().SetTimer(growTimer, this, &AVegetation::Grow, TimeLength, false);
	}
}

bool AVegetation::IsChoppable()
{
	FVector scale = ResourceMesh->GetRelativeScale3D();
	if (scale.X < 1.0f || scale.Y < 1.0f || scale.Z < 1.0f)
		return false;

	SetQuantity(MaxYield);

	if (Owner->IsValidLowLevelFast() && Owner->IsA<AFarm>()) {
		AFarm* farm = Cast<AFarm>(Owner);

		farm->ProductionDone();
	}

	return true;
}