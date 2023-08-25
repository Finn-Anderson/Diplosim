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

void AHouse::UpkeepCost()
{
	for (int i = 0; i < Occupied.Num(); i++) {
		ACitizen* c = Cast<ACitizen>(Occupied[i]);

		if (c->Balance < Rent) {
			RemoveCitizen(c);
		}
		else {
			c->Balance -= Rent;
		}
	}

	int32 rent = Rent * Occupied.Num();
	Camera->ResourceManagerComponent->AddUniversalResource(Money, rent);

	if (!Camera->ResourceManagerComponent->TakeResource(Money, Upkeep)) {
		// Shutdown building;
	}
}

void AHouse::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	int32 maxE = 20;

	if (Citizen->Balance >= 7) {
		Citizen->Balance -= 7;
		maxE = 100;
	}
	else if (Citizen->Balance >= 3) {
		Citizen->Balance -= 3;
		maxE = 75;
	}
	else if (Citizen->Balance >= 1) {
		Citizen->Balance -= 1;
		maxE = 40;
	}

	Citizen->StartGainEnergyTimer();
}

void AHouse::Leave(ACitizen* Citizen)
{
	Super::Leave(Citizen);

	Citizen->StartLoseEnergyTimer();
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
			AddCitizen(c);
		}
		else if (c->Employment != nullptr) {
			float dOldHouse = (c->House->GetActorLocation() - c->Employment->GetActorLocation()).Length();

			float dNewHouse = (GetActorLocation() - c->Employment->GetActorLocation()).Length();

			if (dOldHouse > dNewHouse) {
				AddCitizen(c);
			}
		}
	}

	if (GetCapacity() > Occupied.Num()) {
		GetWorldTimerManager().SetTimer(FindTimer, this, &AHouse::FindCitizens, 30.0f, false);
	}
}

void AHouse::AddCitizen(ACitizen* Citizen)
{
	if (GetCapacity() > Occupied.Num()) {
		Occupied.Add(Citizen);

		Citizen->House = this;

		if (Citizen->Partner == nullptr) {
			TArray<ACitizen*> citizens = GetOccupied();

			for (int i = 0; i < citizens.Num(); i++) {
				if (Citizen->Partner != nullptr) {
					return;
				}

				ACitizen* c = citizens[i];

				Citizen->SetPartner(c);
			}
		}
	}
}

void AHouse::RemoveCitizen(ACitizen* Citizen)
{
	if (Occupied.Contains(Citizen)) {
		Citizen->House = nullptr;

		Occupied.Remove(Citizen);

		if (!GetWorldTimerManager().IsTimerActive(FindTimer)) {
			GetWorldTimerManager().SetTimer(FindTimer, this, &AHouse::FindCitizens, 30.0f, false);
		}
	}
}