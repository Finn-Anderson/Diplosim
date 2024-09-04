#include "Vegetation.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"

AVegetation::AVegetation()
{
	ResourceHISM->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody, ECollisionResponse::ECR_Overlap);
	ResourceHISM->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	ResourceHISM->NumCustomDataFloats = 2;

	IntialScale = FVector(0.1f, 0.1f, 0.1f);

	MaxScale = FVector(1.0f, 1.0f, 1.0f);

	TimeLength = 30.0f;
}

void AVegetation::YieldStatus(int32 Instance, int32 Yield)
{
	FTransform transform;
	ResourceHISM->GetInstanceTransform(Instance, transform);
	transform.SetScale3D(IntialScale);

	ResourceHISM->UpdateInstanceTransform(Instance, transform, false);

	GrowingInstances.Add(Instance);

	if (!GetWorldTimerManager().IsTimerActive(GrowTimer))
		GetWorldTimerManager().SetTimer(GrowTimer, this, &AVegetation::Grow, TimeLength / 100.0f, true);
}

void AVegetation::Grow()
{
	for (int32 i = GrowingInstances.Num() - 1; i > -1; i--) {
		int32 inst = GrowingInstances[i];

		ResourceHISM->SetCustomDataValue(inst, 1, FMath::Clamp(ResourceHISM->PerInstanceSMCustomData[inst * 2 + 1] + 1.0f, 0.0f, 10.0f));

		if (ResourceHISM->PerInstanceSMCustomData[inst * 2 + 1] < 10)
			continue;

		FTransform transform;
		ResourceHISM->GetInstanceTransform(inst, transform);

		FVector scale = transform.GetScale3D();

		if (scale.X < 1.0f)
			scale.X += 0.1f;

		if (scale.Y < 1.0f)
			scale.Y += 0.1f;

		if (scale.Z < 1.0f)
			scale.Z += 0.1f;

		transform.SetScale3D(scale);

		ResourceHISM->UpdateInstanceTransform(inst, transform, false);

		ResourceHISM->PerInstanceSMCustomData[inst * 2 + 1] = 0;

		if (!IsHarvestable(inst, scale))
			continue;

		GrowingInstances.Remove(inst);
	}

	if (GrowingInstances.IsEmpty())
		GetWorldTimerManager().ClearTimer(GrowTimer);
}

bool AVegetation::IsHarvestable(int32 Instance, FVector Scale)
{
	if (Scale.X < MaxScale.X || Scale.Y < MaxScale.Y || Scale.Z < MaxScale.Z)
		return false;

	return true;
}