#include "Map/AIInstancedStaticMeshComponent.h"

UAIInstancedStaticMeshComponent::UAIInstancedStaticMeshComponent()
{
}

void UAIInstancedStaticMeshComponent::BatchUpdateTransforms(TMap<int32, FTransform> InstanceTransformsToUpdate)
{
	if (InstanceTransformsToUpdate.IsEmpty())
		return;

	Modify();
	SetHasPerInstancePrevTransforms(true);

	Async(EAsyncExecution::TaskGraphMainTick, [this, InstanceTransformsToUpdate]() {
		for (auto& element : InstanceTransformsToUpdate) {
			if (!PerInstanceSMData.IsValidIndex(element.Key))
				continue;

			FPrimitiveInstanceId id = PrimitiveInstanceDataManager.IndexToId(element.Key);

			FInstancedStaticMeshInstanceData& InstanceData = PerInstanceSMData[element.Key];
			InstanceData.Transform = element.Value.ToMatrixWithScale();

			// Effectively trying to do PrimitiveInstanceDataManager.TransformChanged(element.Key);
			FTransform prevTransform;
			GetInstancePrevTransform(element.Key, prevTransform, false);
			SetPreviousTransformById(id, prevTransform, false);

			FBodyInstance*& InstanceBodyInstance = InstanceBodies[element.Key];
			InstanceBodyInstance->SetBodyTransform(element.Value, TeleportFlagToEnum(true));
			InstanceBodyInstance->UpdateBodyScale(element.Value.GetScale3D());
		}
	});
}