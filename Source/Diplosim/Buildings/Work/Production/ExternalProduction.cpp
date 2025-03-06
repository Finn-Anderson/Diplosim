#include "ExternalProduction.h"

#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Universal/Resource.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/CitizenManager.h"

AExternalProduction::AExternalProduction()
{
	DecalComponent->SetVisibility(true);
}

void AExternalProduction::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	int32 amount = 0;

	for (FItemStruct item : Storage)
		amount += item.Amount;

	if (amount == StorageCap)
		return;

	if (GetOccupied().Contains(Citizen))
		Production(Citizen);
}

void AExternalProduction::OnRadialOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TSubclassOf<AResource> resource = Camera->ResourceManager->GetResources(this)[0];

	TArray<TSubclassOf<AResource>> resources;
	
	if (!resource->GetDefaultObject<AResource>()->ParentResource.IsEmpty())
		resources = resource->GetDefaultObject<AResource>()->ParentResource;

	if (OtherActor->IsA(resource) || resources.Contains(OtherActor->GetClass())) {
		FValidResourceStruct validResource;
		validResource.Resource = Cast<AResource>(OtherActor);

		int32 index = INDEX_NONE;
		Resources.Find(validResource, index);

		if (index == INDEX_NONE) {
			Resources.Add(validResource);

			index = Resources.Num() - 1;
		}

		if (!Resources[index].Instances.Contains(OtherBodyIndex))
			Resources[index].Instances.Add(OtherBodyIndex);
	}

	Super::OnRadialOverlapBegin(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}

void AExternalProduction::OnRadialOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	TSubclassOf<AResource> resource = Camera->ResourceManager->GetResources(this)[0];

	TArray<TSubclassOf<AResource>> resources;

	if (!resource->GetDefaultObject<AResource>()->ParentResource.IsEmpty())
		resources = resource->GetDefaultObject<AResource>()->ParentResource;

	if (OtherActor->IsA(resource) || resources.Contains(OtherActor->GetClass())) {
		FValidResourceStruct validResource;
		validResource.Resource = Cast<AResource>(OtherActor);

		int32 index = INDEX_NONE;
		Resources.Find(validResource, index);

		if (index != INDEX_NONE && Resources[index].Instances.Contains(OtherBodyIndex))
			Resources[index].Instances.Remove(OtherBodyIndex);
	} 

	Super::OnRadialOverlapEnd(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex);
}

bool AExternalProduction::RemoveCitizen(ACitizen* Citizen)
{
	bool bCheck = Super::RemoveCitizen(Citizen);

	if (!bCheck)
		return false;

	for (FValidResourceStruct validResource : Resources) {
		for (FWorkerStruct workerStruct : validResource.Resource->WorkerStruct) {
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

	int32 instance = -1;
	AResource* resource = nullptr;

	for (FValidResourceStruct validResource : Resources) {
		for (int32 inst : validResource.Instances) {
			FTransform transform;
			validResource.Resource->ResourceHISM->GetInstanceTransform(inst, transform);

			FWorkerStruct workerStruct;
			workerStruct.Instance = inst;

			int32 index = validResource.Resource->WorkerStruct.Find(workerStruct);

			if (!Citizen->AIController->CanMoveTo(transform.GetLocation()) || transform.GetScale3D().Z < validResource.Resource->ResourceHISM->PerInstanceSMCustomData[inst * 11 + 9] || (index > -1 && validResource.Resource->WorkerStruct[index].Citizens.Num() == validResource.Resource->MaxWorkers))
				continue;

			if (instance == -1) {
				resource = validResource.Resource;
				instance = inst;

				continue;
			}

			FTransform currentTransform;
			resource->ResourceHISM->GetInstanceTransform(instance, currentTransform);

			double magnitude = Citizen->AIController->GetClosestActor(50.0f, GetActorLocation(), currentTransform.GetLocation(), transform.GetLocation(), false);

			if (magnitude <= 0.0f)
				continue;

			resource = validResource.Resource;
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
		FTimerStruct timer;
		timer.CreateTimer("Production", this, 30.0f, FTimerDelegate::CreateUObject(this, &AExternalProduction::Production, Citizen), false);

		Camera->CitizenManager->Timers.Add(timer);
	}
}