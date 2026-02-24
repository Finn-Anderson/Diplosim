#include "Universal/Resource.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

#include "AI/Citizen/Citizen.h"
#include "Player/Camera.h"

AResource::AResource()
{
	PrimaryActorTick.bCanEverTick = false;

	ResourceHISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("ResourceMesh"));
	ResourceHISM->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ResourceHISM->SetCollisionObjectType(ECollisionChannel::ECC_Destructible);
	ResourceHISM->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	ResourceHISM->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	ResourceHISM->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Overlap);
	ResourceHISM->SetMobility(EComponentMobility::Static);
	ResourceHISM->SetCanEverAffectNavigation(false);
	ResourceHISM->bCastDynamicShadow = true;
	ResourceHISM->CastShadow = true;
	ResourceHISM->bWorldPositionOffsetWritesVelocity = false;
	ResourceHISM->bAutoRebuildTreeOnInstanceChanges = false;

	SetRootComponent(ResourceHISM);

	DroppedResource = nullptr;

	MinYield = 1;
	MaxYield = 5;

	MaxWorkers = 1;
}

void AResource::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Camera = PController->GetPawn<ACamera>();
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
	int32 yield = Camera->Stream.RandRange(MinYield, MaxYield);

	return yield;
}

void AResource::AddWorker(ACitizen* Citizen, int32 Instance)
{
	FWorkerStruct workerStruct;
	workerStruct.Instance = Instance;
	workerStruct.Citizens.Add(Citizen);

	int32 index = WorkerStruct.Find(workerStruct);

	if (index == INDEX_NONE)
		WorkerStruct.Add(workerStruct);
	else
		WorkerStruct[index].Citizens.Add(Citizen);
}

TArray<ACitizen*> AResource::RemoveWorker(ACitizen* Citizen, int32 Instance)
{
	TArray<ACitizen*> citizens;

	if (Instance == INDEX_NONE) {
		for (FWorkerStruct worker : WorkerStruct) {
			if (!worker.Citizens.Contains(Citizen))
				continue;

			citizens.Add(Citizen);
			worker.Citizens.Remove(Citizen);

			break;
		}
	}
	else {
		FWorkerStruct workerStruct;
		workerStruct.Instance = Instance;

		int32 index = WorkerStruct.Find(workerStruct);

		if (index == INDEX_NONE)
			return citizens;

		if (!IsValid(Citizen)) {
			citizens.Append(WorkerStruct[index].Citizens);
			WorkerStruct[index].Citizens.Empty();
		}
		else {
			citizens.Add(Citizen);
			WorkerStruct[index].Citizens.Remove(Citizen);
		}
	}

	return citizens;
}

AResource* AResource::GetHarvestedResource()
{
	int32 chance = Camera->Stream.RandRange(1, 100);

	if (SpecialResource != nullptr && chance > 99)
		return Cast<AResource>(SpecialResource->GetDefaultObject());

	if (DroppedResource != nullptr)
		return Cast<AResource>(DroppedResource->GetDefaultObject());

	return this;
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