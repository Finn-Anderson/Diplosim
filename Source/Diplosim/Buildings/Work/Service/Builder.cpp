#include "Buildings/Work/Service/Builder.h"

#include "Kismet/GameplayStatics.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Player/Camera.h"
#include "Player/Managers/ConstructionManager.h"
#include "Components/WidgetComponent.h"
#include "Universal/HealthComponent.h"
#include "Player/Managers/CitizenManager.h"

ABuilder::ABuilder()
{
	HealthComponent->MaxHealth = 50;
	HealthComponent->Health = HealthComponent->MaxHealth;

	BuildPercentage = 0;
}

bool ABuilder::CheckInstant() 
{
	for (const ABuilding* building : Camera->CitizenManager->Buildings) {
		if (!building->IsA<ABuilder>())
			continue;

		return false;
	}

	return true;
}

void ABuilder::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (!GetOccupied().Contains(Citizen))
		return;

	UConstructionManager* cm = Camera->ConstructionManager;

	if (cm->IsBeingConstructed(nullptr, this))
		Citizen->AIController->AIMoveTo(cm->GetBuilding(this));
	else
		cm->FindConstruction(this);
}

void ABuilder::CheckCosts(ACitizen* Citizen, ABuilding* Building)
{
	if (CheckStored(Citizen, Building->TargetList))
		GetWorldTimerManager().SetTimer(ConstructTimer, FTimerDelegate::CreateUObject(this, &ABuilder::AddBuildPercentage, Citizen, Building), 0.1f / Citizen->GetProductivity(), true);
}

void ABuilder::AddBuildPercentage(ACitizen* Citizen, ABuilding* Building)
{
	if (Citizen->Building.BuildingAt != Building)
		return;

	BuildPercentage += 1;

	if (BuildPercentage == 100) {
		Building->OnBuilt();

		Done(Citizen, Building);

		BuildPercentage = 0;

		GetWorldTimerManager().ClearTimer(ConstructTimer);

		if (Camera->WidgetComponent->GetAttachParent() == GetRootComponent())
			Camera->DisplayInteract(this);
	}
}

void ABuilder::Repair(ACitizen* Citizen, ABuilding* Building)
{
	Building->HealthComponent->AddHealth(1);

	if (Building->HealthComponent->IsMaxHealth()) {
		Done(Citizen, Building);

		return;
	}

	FTimerHandle checkSitesTimer;
	GetWorldTimerManager().SetTimer(checkSitesTimer, FTimerDelegate::CreateUObject(this, &ABuilder::Repair, Citizen, Building), 0.1f, false);
}

void ABuilder::Done(ACitizen* Citizen, ABuilding* Building)
{
	UConstructionManager* cm = Camera->ConstructionManager;
	cm->RemoveBuilding(Building);
}