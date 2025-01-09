#include "Vegetation.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"

AVegetation::AVegetation()
{
	ResourceHISM->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody, ECollisionResponse::ECR_Overlap);
	ResourceHISM->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	ResourceHISM->NumCustomDataFloats = 10;

	TimeLength = 30.0f;
}

void AVegetation::YieldStatus(int32 Instance, int32 Yield)
{
	AsyncTask(ENamedThreads::GameThread, [this, Instance]() {
		if (ResourceHISM->PerInstanceSMCustomData[Instance * 10 + 9] == 0.0f) {
			ResourceHISM->RemoveInstance(Instance);

			return;
		}

		FTransform transform;
		ResourceHISM->GetInstanceTransform(Instance, transform);
		transform.SetScale3D(FVector(ResourceHISM->PerInstanceSMCustomData[Instance * 10 + 8] / 10));

		ResourceHISM->UpdateInstanceTransform(Instance, transform, false);

		GrowingInstances.Add(Instance);

		if (!GetWorldTimerManager().IsTimerActive(GrowTimer))
			GetWorldTimerManager().SetTimer(GrowTimer, this, &AVegetation::Grow, TimeLength / 100.0f, true);
	});
}

void AVegetation::Grow()
{
	for (int32 i = GrowingInstances.Num() - 1; i > -1; i--) {
		int32 inst = GrowingInstances[i];

		ResourceHISM->SetCustomDataValue(inst, 1, FMath::Clamp(ResourceHISM->PerInstanceSMCustomData[inst * 10 + 7] + 1.0f, 0.0f, 10.0f));

		if (ResourceHISM->PerInstanceSMCustomData[inst * 10 + 7] < 10)
			continue;

		FTransform transform;
		ResourceHISM->GetInstanceTransform(inst, transform);

		FVector scale = transform.GetScale3D();

		if (scale.X < ResourceHISM->PerInstanceSMCustomData[inst * 10 + 8])
			scale.X += ResourceHISM->PerInstanceSMCustomData[inst * 10 + 8] / 10;

		if (scale.Y < ResourceHISM->PerInstanceSMCustomData[inst * 10 + 8])
			scale.Y += ResourceHISM->PerInstanceSMCustomData[inst * 10 + 8] / 10;

		if (scale.Z < ResourceHISM->PerInstanceSMCustomData[inst * 10 + 8])
			scale.Z += ResourceHISM->PerInstanceSMCustomData[inst * 10 + 8] / 10;

		transform.SetScale3D(scale);

		ResourceHISM->UpdateInstanceTransform(inst, transform, false);

		ResourceHISM->PerInstanceSMCustomData[inst * 10 + 7] = 0;

		if (!IsHarvestable(inst, scale))
			continue;

		GrowingInstances.Remove(inst);
	}

	if (GrowingInstances.IsEmpty())
		GetWorldTimerManager().ClearTimer(GrowTimer);
}

bool AVegetation::IsHarvestable(int32 Instance, FVector Scale)
{
	if (Scale.X < ResourceHISM->PerInstanceSMCustomData[Instance * 10 + 8] || Scale.Y < ResourceHISM->PerInstanceSMCustomData[Instance * 10 + 8] || Scale.Z < ResourceHISM->PerInstanceSMCustomData[Instance * 10 + 8])
		return false;

	return true;
}