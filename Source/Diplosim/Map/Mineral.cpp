#include "Mineral.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Buildings/Building.h"

AMineral::AMineral()
{
	ResourceHISM->SetCanEverAffectNavigation(true);
	ResourceHISM->bFillCollisionUnderneathForNavmesh = true;
	ResourceHISM->NumCustomDataFloats = 2;
}

void AMineral::SetRandomQuantity(int32 Instance)
{
	int32 q = FMath::RandRange(1000, 10000);
	SetQuantity(Instance, q);
}

void AMineral::YieldStatus(int32 Instance, int32 Yield)
{
	int32 quantity = ResourceHISM->PerInstanceSMCustomData[Instance * 2] - Yield;

	ResourceHISM->SetCustomDataValue(Instance, 0, FMath::Clamp(quantity, 0, MaxWorkers));

	if (quantity == 0) {
		for (FWorkerStruct workerStruct : WorkerStruct) {
			if (workerStruct.Instance != Instance)
				continue;

			for (ACitizen* citizen : workerStruct.Citizens)
				citizen->AIController->AIMoveTo(citizen->Building.Employment);

			workerStruct.Citizens.Empty();
		}


		ResourceHISM->RemoveInstance(Instance);
	}
	else if (quantity < MaxYield)
		MaxYield = quantity;
}