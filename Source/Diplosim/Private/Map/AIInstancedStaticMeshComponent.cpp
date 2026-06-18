#include "Map/AIInstancedStaticMeshComponent.h"

UAIInstancedStaticMeshComponent::UAIInstancedStaticMeshComponent()
{

}

void UAIInstancedStaticMeshComponent::BatchUpdateTransforms(TMap<int32, FTransform> InstanceTransformsToUpdate)
{
	if (InstanceTransformsToUpdate.IsEmpty())
		return;

	Async(EAsyncExecution::TaskGraphMainTick, [this, InstanceTransformsToUpdate]() {
		SetHasPerInstancePrevTransforms(true);
		int32 count = 0;

		for (auto& element : InstanceTransformsToUpdate) {
			if (!PerInstanceSMData.IsValidIndex(element.Key))
				continue;

			count++;

			if (InstanceTransformsToUpdate.Num() == count) {
				UpdateInstanceTransform(element.Key, element.Value);

				continue;
			}

			FPrimitiveInstanceId id = PrimitiveInstanceDataManager.IndexToId(element.Key);

			FInstancedStaticMeshInstanceData& InstanceData = PerInstanceSMData[element.Key];
			InstanceData.Transform = element.Value.ToMatrixWithScale();

			// Effectively trying to do PrimitiveInstanceDataManager.TransformChanged(element.Key);
			FTransform prevTransform;
			GetInstancePrevTransform(element.Key, prevTransform, false);
			SetPreviousTransformById(id, prevTransform, false);

			if (!InstanceBodies.IsValidIndex(element.Key))
				return;

			FBodyInstance*& InstanceBodyInstance = InstanceBodies[element.Key];
			InstanceBodyInstance->SetBodyTransform(element.Value, TeleportFlagToEnum(true));
			InstanceBodyInstance->UpdateBodyScale(element.Value.GetScale3D());
		}
	});
}

void UAIInstancedStaticMeshComponent::BatchUpdateData(TArray<int32> Instances)
{
	if (Instances.IsEmpty())
		return;

	Async(EAsyncExecution::TaskGraphMainTick, [this, Instances]() {
		for (int32 instance : Instances) {
			TArray<float> data;
			for (int32 i = 0; i < NumCustomDataFloats; i++)
				data.Add(PerInstanceSMCustomData[instance * NumCustomDataFloats + i]);

			SetCustomData(instance, data);
		}
	});
}