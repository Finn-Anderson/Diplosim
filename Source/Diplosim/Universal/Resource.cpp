#include "Resource.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"

AResource::AResource()
{
	PrimaryActorTick.bCanEverTick = false;

	ResourceHISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("ResourceMesh"));
	ResourceHISM->SetCollisionObjectType(ECollisionChannel::ECC_Destructible);
	ResourceHISM->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Overlap);
	ResourceHISM->SetMobility(EComponentMobility::Static);
	ResourceHISM->SetCanEverAffectNavigation(false);
	ResourceHISM->bCastDynamicShadow = true;
	ResourceHISM->CastShadow = true;

	DroppedResource = nullptr;

	MinYield = 1;
	MaxYield = 5;

	MaxWorkers = 1;
}

int32 AResource::GetYield(ACitizen* Citizen, int32 Instance)
{
	int32 yield = GenerateYield();

	RemoveWorker(Citizen, Instance);

	YieldStatus(Instance, yield);

	return yield;
}

int32 AResource::GenerateYield()
{
	int32 yield = FMath::RandRange(MinYield, MaxYield);

	return yield;
}

void AResource::AddWorker(ACitizen* Citizen, int32 Instance)
{
	FWorkerStruct workerStruct;
	workerStruct.Instance = Instance;
	workerStruct.Citizens.Add(Citizen);

	int32 index = WorkerStruct.Find(workerStruct);

	if (index == -1)
		WorkerStruct.Add(workerStruct);
	else
		WorkerStruct[index].Citizens.Add(Citizen);
}

void AResource::RemoveWorker(ACitizen* Citizen, int32 Instance)
{
	FWorkerStruct workerStruct;
	workerStruct.Instance = Instance;

	int32 index = WorkerStruct.Find(workerStruct);

	if (index == -1)
		return;

	WorkerStruct[index].Citizens.Remove(Citizen);
}

AResource* AResource::GetHarvestedResource()
{
	if (DroppedResource == nullptr)
		return this;

	return Cast<AResource>(DroppedResource->GetDefaultObject());
}

TArray<TSubclassOf<class AResource>> AResource::GetParentResources()
{
	TArray<TSubclassOf<class AResource>> parent;
	parent.Add(GetClass());

	if (ParentResource.IsEmpty())
		return parent;

	return ParentResource;
}

void AResource::YieldStatus(int32 Instance, int32 Yield)
{

}