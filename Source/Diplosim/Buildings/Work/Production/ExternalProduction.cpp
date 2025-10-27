#include "ExternalProduction.h"

#include "Components/DecalComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Universal/Resource.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/CitizenManager.h"
#include "Map/Grid.h"
#include "Map/Resources/Vegetation.h"

AExternalProduction::AExternalProduction()
{
	DecalComponent->SetVisibility(true);

	Range = 1000;
}

void AExternalProduction::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (IsCapacityFull())
		return;

	if (GetOccupied().Contains(Citizen))
		Production(Citizen);
}

bool AExternalProduction::RemoveCitizen(ACitizen* Citizen)
{
	bool bCheck = Super::RemoveCitizen(Citizen);

	if (!bCheck)
		return false;

	for (auto& element : GetValidResources()) {
		for (FWorkerStruct workerStruct : element.Key->WorkerStruct) {
			if (!workerStruct.Citizens.Contains(Citizen))
				continue;

			workerStruct.Citizens.Remove(Citizen);

			return true;
		}
	}

	return true;
}

void AExternalProduction::Production(ACitizen* Citizen)
{
	Super::Production(Citizen);

	if (Citizen->Building.BuildingAt != this)
		return;

	AResource* resource = nullptr;
	int32 instance = -1;

	for (auto& element : GetValidResources()) {
		for (int32 inst : element.Value) {
			FTransform transform;
			element.Key->ResourceHISM->GetInstanceTransform(inst, transform);

			FWorkerStruct workerStruct;
			workerStruct.Instance = inst;

			int32 index = element.Key->WorkerStruct.Find(workerStruct);

			if (!Citizen->AIController->CanMoveTo(transform.GetLocation()) || transform.GetScale3D().Z < element.Key->ResourceHISM->PerInstanceSMCustomData[inst * 11 + 9] || (index > -1 && element.Key->WorkerStruct[index].Citizens.Num() == element.Key->MaxWorkers))
				continue;

			if (instance == -1) {
				resource = element.Key;
				instance = inst;

				continue;
			}

			FTransform currentTransform;
			resource->ResourceHISM->GetInstanceTransform(instance, currentTransform);

			double magnitude = Citizen->AIController->GetClosestActor(50.0f, GetActorLocation(), currentTransform.GetLocation(), transform.GetLocation(), false);

			if (magnitude <= 0.0f)
				continue;

			resource = element.Key;
			instance = inst;
		}
	}

	if (instance != -1) {
		resource->AddWorker(Citizen, instance);

		FTransform transform;
		resource->ResourceHISM->GetInstanceTransform(instance, transform);

		Citizen->AIController->AIMoveTo(resource, transform.GetLocation(), instance);
	}
	else {
		TArray<FTimerParameterStruct> params;
		Camera->CitizenManager->SetParameter(Citizen, params);
		Camera->CitizenManager->CreateTimer("Production", this, 30, "Production", params, false);
	}
}

TMap<AResource*, TArray<int32>> AExternalProduction::GetValidResources()
{
	TSubclassOf<AResource> resourceClass = Camera->ResourceManager->GetResources(this)[0];
	TArray<TSubclassOf<AResource>> resources;

	if (!resourceClass->GetDefaultObject<AResource>()->ParentResource.IsEmpty())
		resources = resourceClass->GetDefaultObject<AResource>()->ParentResource;

	TArray<FResourceHISMStruct> resourceStructs;
	resourceStructs.Append(Camera->Grid->MineralStruct);
	resourceStructs.Append(Camera->Grid->TreeStruct);

	TMap<AResource*, TArray<int32>> validResources;

	for (FResourceHISMStruct resourceStruct : resourceStructs) {
		if (!resources.Contains(resourceStruct.ResourceClass))
			continue;

		TArray<int32> instances = resourceStruct.Resource->ResourceHISM->GetInstancesOverlappingSphere(GetActorLocation(), Range);

		validResources.Add(resourceStruct.Resource, instances);
	}

	return validResources;
}