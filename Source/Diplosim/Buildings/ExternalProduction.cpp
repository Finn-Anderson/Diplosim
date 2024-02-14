#include "ExternalProduction.h"

#include "Components/SphereComponent.h"
#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Resource.h"
#include "Map/Vegetation.h"
#include "Player/Camera.h"
#include "Player/ResourceManager.h"

AExternalProduction::AExternalProduction()
{
	DecalComponent = CreateDefaultSubobject<UDecalComponent>("DecalComponent");
	DecalComponent->SetupAttachment(RootComponent);
	DecalComponent->DecalSize = FVector(1500.0f, 1500.0f, 1500.0f);
	DecalComponent->SetRelativeRotation(FRotator(-90, 0, 0));

	Instance = -1;
}

void AExternalProduction::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (GetOccupied().Contains(Citizen))
		Store(Citizen->Carrying.Amount, Citizen);
}

void AExternalProduction::FindCitizens()
{
	Super::FindCitizens();

	TArray<AActor*> actors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), Camera->ResourceManagerComponent->GetResource(this), actors);

	Resource = Cast<AResource>(actors[0]);

	InstanceList = Resource->ResourceHISM->GetInstancesOverlappingSphere(GetActorLocation(), 1500.0f);
}

void AExternalProduction::RemoveCitizen(ACitizen* Citizen)
{
	Super::RemoveCitizen(Citizen);

	for (FWorkerStruct workerStruct : Resource->WorkerStruct) {
		if (!workerStruct.Citizens.Contains(Citizen))
			continue;

		workerStruct.Citizens.Remove(Citizen);

		break;
	}
}

void AExternalProduction::Production(ACitizen* Citizen)
{
	if (Citizen->Building.BuildingAt != this)
		return;

	Instance = -1;

	for (int32 inst : InstanceList) {
		if (Resource->IsA<AVegetation>() && Resource->ResourceHISM->PerInstanceSMCustomData[inst * 4 + 3] == 0.0f)
			continue;

		FTransform transform;
		Resource->ResourceHISM->GetInstanceTransform(inst, transform);

		FWorkerStruct workerStruct;
		workerStruct.Instance = inst;

		int32 index = Resource->WorkerStruct.Find(workerStruct);

		int32 quantity = Resource->ResourceHISM->PerInstanceSMCustomData[inst * 4 + 0];

		if (!Citizen->AIController->CanMoveTo(transform.GetLocation()) || quantity == 0 || (index > -1 && Resource->WorkerStruct[index].Citizens.Num() == Resource->MaxWorkers))
			continue;

		if (Instance == -1) {
			Instance = inst;
			
			continue;
		}

		FTransform currentTransform;
		FTransform newTransform;

		Resource->ResourceHISM->GetInstanceTransform(Instance, currentTransform);
		Resource->ResourceHISM->GetInstanceTransform(inst, newTransform);

		double magnitude = Citizen->AIController->GetClosestActor(Citizen->GetActorLocation(), currentTransform.GetLocation(), newTransform.GetLocation());

		if (magnitude <= 0.0f)
			continue;

		Instance = inst;
	}

	if (Resource != nullptr) {
		Resource->AddWorker(Citizen, Instance);

		Citizen->AIController->AIMoveTo(Resource, Instance);
	}
	else
		GetWorldTimerManager().SetTimer(ProdTimer, FTimerDelegate::CreateUObject(this, &AExternalProduction::Production, Citizen), 30.0f, false);
}