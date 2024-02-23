#include "House.h"

#include "Kismet/GameplayStatics.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Player/Camera.h"
#include "Player/ResourceManager.h"

AHouse::AHouse()
{
	Rent = 0;
}

void AHouse::UpkeepCost(int32 Cost)
{
	for (ACitizen* citizen : GetOccupied()) {
		if (citizen->Balance < Rent) {
			RemoveCitizen(citizen);
		}
		else {
			citizen->Balance -= Rent;
		}
	}

	int32 rent = Rent * Occupied.Num();
	Camera->ResourceManagerComponent->AddUniversalResource(Money, rent);

	Super::UpkeepCost(Upkeep);
}

void AHouse::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	Citizen->SetEnergyTimer(true);
}

void AHouse::Leave(ACitizen* Citizen)
{
	Super::Leave(Citizen);

	Citizen->SetEnergyTimer(false);
}

void AHouse::FindCitizens()
{
	TArray<AActor*> citizens;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), citizens);

	for (AActor* actor : citizens) {
		ACitizen* citizen = Cast<ACitizen>(actor);

		if (citizen->Balance < Rent || !citizen->AIController->CanMoveTo(GetActorLocation()))
			continue;

		if (citizen->Building.House == nullptr) {
			AddCitizen(citizen);
		}
		else if (citizen->Building.Employment != nullptr) {
			double magnitude = citizen->AIController->GetClosestActor(citizen->Building.Employment->GetActorLocation(), citizen->Building.House->GetActorLocation(), GetActorLocation());

			if (magnitude <= 0.0f)
				continue;

			AddCitizen(citizen);
		}

		if (GetCapacity() == GetOccupied().Num())
			return;
	}

	if (GetCapacity() > Occupied.Num())
		GetWorldTimerManager().SetTimer(FindTimer, this, &AHouse::FindCitizens, 30.0f, false);
}

bool AHouse::AddCitizen(ACitizen* Citizen)
{
	bool bCheck = Super::AddCitizen(Citizen);

	if (!bCheck)
		return false;

	Citizen->Building.House = this;

	return true;
}

bool AHouse::RemoveCitizen(ACitizen* Citizen)
{
	bool bCheck = Super::RemoveCitizen(Citizen);

	if (!bCheck)
		return false;

	Citizen->Building.House = nullptr;

	if (!GetWorldTimerManager().IsTimerActive(FindTimer))
		GetWorldTimerManager().SetTimer(FindTimer, this, &AHouse::FindCitizens, 30.0f, false);

	return true;
}