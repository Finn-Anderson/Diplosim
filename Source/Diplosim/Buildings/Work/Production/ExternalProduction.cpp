#include "ExternalProduction.h"

#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Universal/Resource.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"

AExternalProduction::AExternalProduction()
{
	DecalComponent->SetVisibility(true);

	Resource = nullptr;
}

void AExternalProduction::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (GetOccupied().Contains(Citizen))
		Production(Citizen);
}

void AExternalProduction::OnRadialOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TSubclassOf<AResource> r = Camera->ResourceManager->GetResource(this);

	if (OtherActor->IsA(r) && !Instances.Contains(OtherBodyIndex))
		Instances.Add(OtherBodyIndex);

	if (OtherActor->IsA(r) && Resource == nullptr)
		Resource = Cast<AResource>(OtherActor);

	Super::OnRadialOverlapBegin(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}

void AExternalProduction::OnRadialOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	TSubclassOf<AResource> resource = Camera->ResourceManager->GetResource(this);

	if (OtherActor->IsA(resource) && Instances.Contains(OtherBodyIndex))
		Instances.Remove(OtherBodyIndex);

	Super::OnRadialOverlapEnd(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex);
}

bool AExternalProduction::RemoveCitizen(ACitizen* Citizen)
{
	bool bCheck = Super::RemoveCitizen(Citizen);

	if (!bCheck)
		return false;

	for (FWorkerStruct workerStruct : Resource->WorkerStruct) {
		if (!workerStruct.Citizens.Contains(Citizen))
			continue;

		workerStruct.Citizens.Remove(Citizen);

		return true;
	}

	return true;
}

void AExternalProduction::Production(ACitizen* Citizen)
{
	Super::Production(Citizen);

	if (Citizen->Building.BuildingAt != this)
		return;

	int32 instance = -1;

	for (int32 inst : Instances) {
		FTransform transform;
		Resource->ResourceHISM->GetInstanceTransform(inst, transform);

		FWorkerStruct workerStruct;
		workerStruct.Instance = inst;

		int32 index = Resource->WorkerStruct.Find(workerStruct);

		if (!Citizen->AIController->CanMoveTo(transform.GetLocation()) || transform.GetScale3D().Z < 1.0f || (index > -1 && Resource->WorkerStruct[index].Citizens.Num() == Resource->MaxWorkers))
			continue;

		if (instance == -1) {
			instance = inst;

			continue;
		}

		FTransform currentTransform;
		FTransform newTransform;

		Resource->ResourceHISM->GetInstanceTransform(instance, currentTransform);
		Resource->ResourceHISM->GetInstanceTransform(inst, newTransform);

		double magnitude = Citizen->AIController->GetClosestActor(GetActorLocation(), currentTransform.GetLocation(), newTransform.GetLocation());

		if (magnitude <= 0.0f)
			continue;

		instance = inst;
	}

	if (instance != -1) {
		Resource->AddWorker(Citizen, instance);

		FTransform transform;
		Resource->ResourceHISM->GetInstanceTransform(instance, transform);

		Citizen->AIController->AIMoveTo(Resource, transform.GetLocation(), instance);
	}
	else
		GetWorldTimerManager().SetTimer(ProdTimer, FTimerDelegate::CreateUObject(this, &AExternalProduction::Production, Citizen), 30.0f, false);
}