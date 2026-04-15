#include "Map/Resources/Vegetation.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "NiagaraComponent.h"

#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/DiplosimTimerManager.h"

AVegetation::AVegetation()
{
	ResourceHISM->SetGenerateOverlapEvents(false);
	ResourceHISM->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody, ECollisionResponse::ECR_Overlap);
	ResourceHISM->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	ResourceHISM->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	ResourceHISM->NumCustomDataFloats = 11;

	TimeLength = 30.0f;
}

void AVegetation::YieldStatus(int32 Instance, int32 Yield)
{
	if (ResourceHISM->PerInstanceSMCustomData[Instance * ResourceHISM->NumCustomDataFloats + 9] == 0.0f) {
		Camera->Grid->RemoveTree(this, { Instance });

		return;
	}
	else if (Yield == 0) {
		UNiagaraComponent* fireComp = Camera->Grid->AtmosphereComponent->GetFireComponent(this, Instance);
		if (fireComp)
			fireComp->Deactivate();
	}

	FTransform transform;
	ResourceHISM->GetInstanceTransform(Instance, transform);
	transform.SetScale3D(FVector(ResourceHISM->PerInstanceSMCustomData[Instance * ResourceHISM->NumCustomDataFloats + 8] / 10));

	ResourceHISM->UpdateInstanceTransform(Instance, transform, false);

	GrowingInstances.Add(Instance);

	if (Camera->TimerManager->FindTimer("Grow", this) == nullptr)
		Camera->TimerManager->CreateTimer("Grow", this, TimeLength / 100.0f, "Grow", {}, true, true);
}

void AVegetation::Grow()
{
	for (int32 i = GrowingInstances.Num() - 1; i > -1; i--) {
		int32 inst = GrowingInstances[i];
		int32 tick = ResourceHISM->PerInstanceSMCustomData[inst * ResourceHISM->NumCustomDataFloats + 7] + 1.0f;

		if (tick < 10.0f) {
			ResourceHISM->SetCustomDataValue(inst, 7, tick);

			continue;
		}

		FTransform transform;
		ResourceHISM->GetInstanceTransform(inst, transform);

		FVector scale = transform.GetScale3D();
		float targetScale = ResourceHISM->PerInstanceSMCustomData[inst * ResourceHISM->NumCustomDataFloats + 8];

		if (scale.X < targetScale)
			scale.X += targetScale / 10.0f;

		if (scale.Y < targetScale)
			scale.Y += targetScale / 10.0f;

		if (scale.Z < targetScale)
			scale.Z += targetScale / 10.0f;

		transform.SetScale3D(scale);

		ResourceHISM->UpdateInstanceTransform(inst, transform, false);
		ResourceHISM->SetCustomDataValue(inst, 7, 0.0f);
		ResourceHISM->SetCustomDataValue(inst, 10, scale.Z);

		if (!IsHarvestable(inst, scale, targetScale))
			continue;

		GrowingInstances.Remove(inst);
	}

	if (GrowingInstances.IsEmpty())
		Camera->TimerManager->RemoveTimer("Grow", this);
}

bool AVegetation::IsHarvestable(int32 Instance, FVector Scale, float TargetScale)
{
	if (Scale.X < TargetScale || Scale.Y < TargetScale || Scale.Z < TargetScale)
		return false;

	return true;
}

float AVegetation::OnFire(int32 Instance)
{
	FString id = "OnFire" + FString::FromInt(Instance);
	FTimerStruct* timer = Camera->TimerManager->FindTimer(id, this);

	if (timer == nullptr) {
		TArray<FTimerParameterStruct> params;
		Camera->TimerManager->SetParameter(Instance, params);
		Camera->TimerManager->SetParameter(0, params);
		Camera->TimerManager->CreateTimer(id, this, 5.0f, "YieldStatus", params, false, true);

		return 5.0f;
	}
	else {
		return timer->Target - timer->Timer;
	}
}