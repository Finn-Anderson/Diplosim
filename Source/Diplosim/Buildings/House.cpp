#include "House.h"

#include "Kismet/GameplayStatics.h"

#include "AI/Citizen.h"
#include "Player/Camera.h"
#include "Player/ResourceManager.h"
#include "Work.h"

AHouse::AHouse()
{
	Rent = 0;
}

void AHouse::OnBuilt()
{
	GetWorldTimerManager().SetTimer(CostTimer, this, &AHouse::UpkeepCost, 300.0f, true);

	FindCitizens();
}

void AHouse::UpkeepCost()
{
	for (int i = 0; i < Occupied.Num(); i++) {
		ACitizen* c = Cast<ACitizen>(Occupied[i]);

		c->Balance -= Rent;

		if (c->Balance < 0) {
			RemoveCitizen(c);
		}
	}

	int32 cost = Rent * Occupied.Num() - Upkeep;

	Camera->ResourceManager->ChangeResource(Money, cost);
}

void AHouse::FindCitizens()
{
	TArray<AActor*> citizens;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), citizens);

	for (int i = 0; i < citizens.Num(); i++) {
		ACitizen* c = Cast<ACitizen>(citizens[i]);

		if (GetCapacity() <= GetOccupied().Num())
			return;

		if (c->Balance < Rent)
			continue;

		if (c->House == nullptr) {
			c->House = this;
			
		}
		else {
			float dOldHouse = (c->House->GetActorLocation() - c->Employment->GetActorLocation()).Length();

			float dNewHouse = (GetActorLocation() - c->Employment->GetActorLocation()).Length();

			if (dOldHouse > dNewHouse) {
				c->House = this;
			}
		}
		
	}

	if (GetCapacity() > Occupied.Num()) {
		GetWorldTimerManager().SetTimer(FindTimer, this, &AHouse::FindCitizens, 30.0f, false, 2.0f);
	}
}

void AHouse::AddCitizen(ACitizen* Citizen)
{
	if (GetCapacity() > Occupied.Num()) {
		Occupied.Add(Citizen);

		Citizen->House = this;
	}
}

void AHouse::RemoveCitizen(ACitizen* Citizen)
{
	if (Occupied.Contains(Citizen)) {
		Citizen->House = nullptr;

		Occupied.Remove(Citizen);
	}
}