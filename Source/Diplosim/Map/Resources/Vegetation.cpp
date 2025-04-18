#include "Vegetation.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

#include "Player/Camera.h"
#include "Map/Grid.h"

AVegetation::AVegetation()
{
	ResourceHISM->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody, ECollisionResponse::ECR_Overlap);
	ResourceHISM->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	ResourceHISM->NumCustomDataFloats = 11;

	TimeLength = 30.0f;
}

void AVegetation::YieldStatus(int32 Instance, int32 Yield)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, FString::Printf(TEXT("%d"), Instance));
	
	if (ResourceHISM->PerInstanceSMCustomData[Instance * 11 + 10] == 0.0f) {
		APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		ACamera* camera = PController->GetPawn<ACamera>();

		camera->Grid->RemoveTree(this, Instance);

		return;
	}

	FTransform transform;
	ResourceHISM->GetInstanceTransform(Instance, transform);
	transform.SetScale3D(FVector(ResourceHISM->PerInstanceSMCustomData[Instance * 11 + 9] / 10));

	ResourceHISM->UpdateInstanceTransform(Instance, transform, false);

	GrowingInstances.Add(Instance);

	if (!GetWorldTimerManager().IsTimerActive(GrowTimer))
		GetWorldTimerManager().SetTimer(GrowTimer, this, &AVegetation::Grow, TimeLength / 100.0f, true);
}

void AVegetation::Grow()
{
	for (int32 i = GrowingInstances.Num() - 1; i > -1; i--) {
		int32 inst = GrowingInstances[i];

		ResourceHISM->SetCustomDataValue(inst, 1, FMath::Clamp(ResourceHISM->PerInstanceSMCustomData[inst * 11 + 8] + 1.0f, 0.0f, 10.0f));

		if (ResourceHISM->PerInstanceSMCustomData[inst * 11 + 8] < 10)
			continue;

		FTransform transform;
		ResourceHISM->GetInstanceTransform(inst, transform);

		FVector scale = transform.GetScale3D();

		if (scale.X < ResourceHISM->PerInstanceSMCustomData[inst * 11 + 9])
			scale.X += ResourceHISM->PerInstanceSMCustomData[inst * 11 + 9] / 10;

		if (scale.Y < ResourceHISM->PerInstanceSMCustomData[inst * 11 + 9])
			scale.Y += ResourceHISM->PerInstanceSMCustomData[inst * 11 + 9] / 10;

		if (scale.Z < ResourceHISM->PerInstanceSMCustomData[inst * 11 + 9])
			scale.Z += ResourceHISM->PerInstanceSMCustomData[inst * 11 + 9] / 10;

		transform.SetScale3D(scale);

		ResourceHISM->UpdateInstanceTransform(inst, transform, false);

		ResourceHISM->PerInstanceSMCustomData[inst * 11 + 8] = 0;

		if (!IsHarvestable(inst, scale))
			continue;

		GrowingInstances.Remove(inst);
	}

	if (GrowingInstances.IsEmpty())
		GetWorldTimerManager().ClearTimer(GrowTimer);
}

bool AVegetation::IsHarvestable(int32 Instance, FVector Scale)
{
	if (Scale.X < ResourceHISM->PerInstanceSMCustomData[Instance * 11 + 9] || Scale.Y < ResourceHISM->PerInstanceSMCustomData[Instance * 11 + 9] || Scale.Z < ResourceHISM->PerInstanceSMCustomData[Instance * 11 + 9])
		return false;

	return true;
}