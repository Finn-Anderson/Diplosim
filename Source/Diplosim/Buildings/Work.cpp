#include "Work.h"

#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Player/Camera.h"
#include "Player/ResourceManager.h"
#include "InteractableInterface.h"

AWork::AWork()
{
	Wage = 0;
}

void AWork::UpkeepCost(int32 Cost)
{
	for (int i = 0; i < Occupied.Num(); i++) {
		ACitizen* c = Cast<ACitizen>(Occupied[i]);

		c->Balance += Wage;
	}

	Cost = Wage * Occupied.Num() + Upkeep;

	Super::UpkeepCost(Cost);
}

void AWork::FindCitizens()
{
	TArray<AActor*> citizens;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), citizens);

	for (int i = 0; i < citizens.Num(); i++) {
		ACitizen* c = Cast<ACitizen>(citizens[i]);

		if (c->BioStruct.Age < 18 || c->Building.Employment != nullptr || !c->AIController->CanMoveTo(GetActorLocation()))
			continue;
			
		AddCitizen(c);

		if (GetCapacity() == GetOccupied().Num())
			return;
	}

	if (GetCapacity() > GetOccupied().Num())
		GetWorldTimerManager().SetTimer(FindTimer, this, &AWork::FindCitizens, 30.0f, false);
}

bool AWork::AddCitizen(ACitizen* Citizen)
{
	bool bCheck = Super::AddCitizen(Citizen);

	if (!bCheck)
		return false;

	Citizen->Building.Employment = this;

	Citizen->AIController->AIMoveTo(this);

	return true;
}

bool AWork::RemoveCitizen(ACitizen* Citizen)
{
	bool bCheck = Super::RemoveCitizen(Citizen);

	if (!bCheck)
		return false;

	Citizen->Building.Employment = nullptr;

	if (!GetWorldTimerManager().IsTimerActive(FindTimer))
		GetWorldTimerManager().SetTimer(FindTimer, this, &AWork::FindCitizens, 30.0f, false);

	return true;
}

void AWork::Store(int32 Amount, ACitizen* Citizen)
{
	if (Camera->ResourceManagerComponent->AddLocalResource(this, Amount)) {
		Citizen->Carry(nullptr, 0, nullptr);

		Production(Citizen);
	}
	else {
		GetWorldTimerManager().ClearTimer(ProdTimer);

		FTimerHandle StoreCheckTimer;
		GetWorldTimerManager().SetTimer(StoreCheckTimer, FTimerDelegate::CreateUObject(this, &AWork::Store, Amount, Citizen), 30.0f, false);
	}
}

void AWork::Production(ACitizen* Citizen)
{

}

TArray<ACitizen*> AWork::GetWorkers()
{
	TArray<ACitizen*> workers;

	for (ACitizen* citizen : GetOccupied()) {
		if (citizen->Building.BuildingAt != this)
			continue;

		workers.Add(citizen);
	}

	return workers;
}