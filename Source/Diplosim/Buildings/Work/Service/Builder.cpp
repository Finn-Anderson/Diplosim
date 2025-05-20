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
	if (CheckStored(Citizen, Building->TargetList)) {
		FVector size = Building->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize();

		float time = (size.X + size.Y + size.Z) / 500.0f * 0.2f / Citizen->GetProductivity();

		Camera->CitizenManager->CreateTimer("Construct", GetOwner(), time, FTimerDelegate::CreateUObject(this, &ABuilder::AddBuildPercentage, Citizen, Building), true, true);
	}
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

		if (Camera->WidgetComponent->GetAttachParent() == GetRootComponent())
			Camera->DisplayInteract(this);
	}
}

void ABuilder::StartRepairTimer(ACitizen* Citizen, ABuilding* Building)
{
	Camera->CitizenManager->CreateTimer("Repair", GetOwner(), 0.2f, FTimerDelegate::CreateUObject(this, &ABuilder::Repair, Citizen, Building), true, true);
}

void ABuilder::Repair(ACitizen* Citizen, ABuilding* Building)
{
	Building->HealthComponent->AddHealth(2);

	if (Building->HealthComponent->IsMaxHealth()) {
		Done(Citizen, Building);

		return;
	}
}

void ABuilder::Done(ACitizen* Citizen, ABuilding* Building)
{
	Camera->CitizenManager->RemoveTimer("Repair", GetOwner());

	Camera->ConstructionManager->RemoveBuilding(Building);
}