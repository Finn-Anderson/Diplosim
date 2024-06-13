#include "ExternalProduction.h"

#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Resource.h"
#include "Player/Camera.h"
#include "Player/ResourceManager.h"

AExternalProduction::AExternalProduction()
{
	DecalComponent = CreateDefaultSubobject<UDecalComponent>("DecalComponent");
	DecalComponent->SetupAttachment(RootComponent);
	DecalComponent->DecalSize = FVector(1500.0f, 1500.0f, 1500.0f);
	DecalComponent->SetRelativeRotation(FRotator(-90, 0, 0));
}

void AExternalProduction::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (GetOccupied().Contains(Citizen))
		Production(Citizen);
}

void AExternalProduction::FindCitizens()
{
	Super::FindCitizens();

	TSubclassOf<AResource> resource = Camera->ResourceManagerComponent->GetResource(this);

	for (TSubclassOf<AResource> parentClass : Cast<AResource>(resource->GetDefaultObject())->GetParentResources()) {
		FValidResourceStruct validResourceStruct;

		TArray<AActor*> actors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), parentClass, actors);

		validResourceStruct.Resource = Cast<AResource>(actors[0]);

		validResourceStruct.Instances = validResourceStruct.Resource->ResourceHISM->GetInstancesOverlappingSphere(GetActorLocation(), 1500.0f);

		ValidResourceList.Add(validResourceStruct);
	}
}

bool AExternalProduction::RemoveCitizen(ACitizen* Citizen)
{
	bool bCheck = Super::RemoveCitizen(Citizen);

	if (!bCheck)
		return false;

	for (FValidResourceStruct validResourceStruct : ValidResourceList) {
		for (FWorkerStruct workerStruct : validResourceStruct.Resource->WorkerStruct) {
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

	for (FValidResourceStruct validResourceStruct : ValidResourceList) {
		for (int32 inst : validResourceStruct.Instances) {
			FTransform transform;
			validResourceStruct.Resource->ResourceHISM->GetInstanceTransform(inst, transform);

			FWorkerStruct workerStruct;
			workerStruct.Instance = inst;

			int32 index = validResourceStruct.Resource->WorkerStruct.Find(workerStruct);

			if (!Citizen->AIController->CanMoveTo(transform.GetLocation()) || transform.GetScale3D().Z < 1.0f || (index > -1 && validResourceStruct.Resource->WorkerStruct[index].Citizens.Num() == validResourceStruct.Resource->MaxWorkers))
				continue;

			if (instance == -1) {
				resource = validResourceStruct.Resource;
				instance = inst;

				continue;
			}

			FTransform currentTransform;
			FTransform newTransform;

			resource->ResourceHISM->GetInstanceTransform(instance, currentTransform);
			validResourceStruct.Resource->ResourceHISM->GetInstanceTransform(inst, newTransform);

			double magnitude = Citizen->AIController->GetClosestActor(GetActorLocation(), currentTransform.GetLocation(), newTransform.GetLocation());

			if (magnitude <= 0.0f)
				continue;

			resource = validResourceStruct.Resource;
			instance = inst;
		}
	}

	if (resource != nullptr) {
		resource->AddWorker(Citizen, instance);

		FTransform transform;
		resource->ResourceHISM->GetInstanceTransform(instance, transform);

		Citizen->AIController->AIMoveTo(resource, transform.GetLocation(), instance);
	}
	else
		GetWorldTimerManager().SetTimer(ProdTimer, FTimerDelegate::CreateUObject(this, &AExternalProduction::Production, Citizen), 30.0f, false);
}