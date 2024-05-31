#include "Buildings/Builder.h"

#include "Kismet/GameplayStatics.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Player/Camera.h"
#include "Player/ResourceManager.h"
#include "Player/ConstructionManager.h"
#include "Components/WidgetComponent.h"

ABuilder::ABuilder()
{
	BuildPercentage = 0;
}

bool ABuilder::CheckInstant() 
{
	TArray<AActor*> foundBuilders;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), this->GetClass(), foundBuilders);

	if (foundBuilders.Num() == 1) {
		return true;
	}

	return false;
}

void ABuilder::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (!Occupied.Contains(Citizen))
		return;

	UConstructionManager* cm = Camera->ConstructionManagerComponent;

	if (cm->IsBeingConstructed(nullptr, this))
		Citizen->AIController->AIMoveTo(cm->GetBuilding(this));
	else
		cm->FindConstruction(this);
}

void ABuilder::CarryResources(ACitizen* Citizen, ABuilding* Building)
{
	int32 amount = 0;
	int32 capacity = 10;

	TArray<TSubclassOf<AResource>> resources = Camera->ResourceManagerComponent->GetResource(Building);

	if (capacity > Building->Storage)
		capacity = Building->Storage;

	TSubclassOf<AResource> resource;

	UConstructionManager* cm = Camera->ConstructionManagerComponent;
	ABuilding* construction = cm->GetBuilding(this);

	for (FCostStruct costStruct : construction->GetCosts()) {
		if (resources.Contains(costStruct.Type)) {
			amount = FMath::Clamp(costStruct.Cost - costStruct.Stored, 0, capacity);

			resource = costStruct.Type;

			break;
		}
	}

	Camera->ResourceManagerComponent->TakeCommittedResource(resource, amount);

	Camera->ResourceManagerComponent->TakeLocalResource(Building, amount);

	Citizen->Carry(Cast<AResource>(resource->GetDefaultObject()), amount, construction);
}

void ABuilder::StoreMaterials(ACitizen* Citizen, ABuilding* Building)
{
	for (int32 i = 0; i < Building->GetCosts().Num(); i++) {
		if (Building->GetCosts()[i].Type->GetDefaultObject<AResource>() != Citizen->Carrying.Type)
			continue;

		Building->CostList[i].Stored += Citizen->Carrying.Amount;

		Citizen->Carry(nullptr, 0, nullptr);

		break;
	}
}

void ABuilder::CheckCosts(ACitizen* Citizen, ABuilding* Building)
{
	bool construct = true;

	for (FCostStruct costStruct : Building->GetCosts()) {
		if (costStruct.Stored == costStruct.Cost)
			continue;

		construct = false;

		CheckGatherSites(Citizen, costStruct);

		break;
	}

	if (construct)
		GetWorldTimerManager().SetTimer(ConstructTimer, FTimerDelegate::CreateUObject(this, &ABuilder::AddBuildPercentage, Citizen, Building), 0.1f, true);
}

void ABuilder::CheckGatherSites(ACitizen* Citizen, FCostStruct Stock)
{
	TArray<TSubclassOf<class ABuilding>> buildings = Camera->ResourceManagerComponent->GetBuildings(Stock.Type);

	ABuilding* target = nullptr;

	for (int32 j = 0; j < buildings.Num(); j++) {
		TArray<AActor*> foundBuildings;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), buildings[j], foundBuildings);

		for (int32 k = 0; k < foundBuildings.Num(); k++) {
			ABuilding* building = Cast<ABuilding>(foundBuildings[k]);

			if (building->Storage < 1)
				continue;

			if (target == nullptr) {
				target = building;

				continue;
			}

			int32 storage = 0;

			if (target != nullptr)
				storage = target->Storage;

			double magnitude = Citizen->AIController->GetClosestActor(Citizen->GetActorLocation(), target->GetActorLocation(), building->GetActorLocation(), storage, building->Storage);

			if (magnitude <= 0.0f)
				continue;

			target = building;
		}
	}

	if (target != nullptr)
		Citizen->AIController->AIMoveTo(target);
	else {
		FTimerHandle checkSitesTimer;
		GetWorldTimerManager().SetTimer(checkSitesTimer, FTimerDelegate::CreateUObject(this, &ABuilder::CheckGatherSites, Citizen, Stock), 30.0f, false);
	}
}

void ABuilder::AddBuildPercentage(ACitizen* Citizen, ABuilding* Building)
{
	if (Citizen->Building.BuildingAt != Building)
		return;

	BuildPercentage += 1;

	if (BuildPercentage == 100) {
		Building->OnBuilt();

		UConstructionManager* cm = Camera->ConstructionManagerComponent;
		cm->RemoveBuilding(Building);

		BuildPercentage = 0;

		Citizen->AIController->AIMoveTo(Citizen->Building.Employment);

		GetWorldTimerManager().ClearTimer(ConstructTimer);

		if (Camera->WidgetComponent->GetAttachParent() == GetRootComponent())
			Camera->DisplayInteract(this);
	}
}