#pragma once

#include "CoreMinimal.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "AIInstancedStaticMeshComponent.generated.h"

UCLASS()
class DIPLOSIM_API UAIInstancedStaticMeshComponent : public UInstancedStaticMeshComponent
{
	GENERATED_BODY()
	
public:
	UAIInstancedStaticMeshComponent();

	void BatchUpdateTransforms(TMap<int32, FTransform> InstanceTransformsToUpdate);
};
