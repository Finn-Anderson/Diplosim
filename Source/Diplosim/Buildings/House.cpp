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

	if (!Camera->ResourceManagerComponent->TakeUniversalResource(Money, Upkeep)) {
		// Shutdown building;
	}
}

void AHouse::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	TArray<TSubclassOf<AResource>> Resources;
	for (int32 i = 0; i < Food.Num(); i++) {
		int32 curAmount = Camera->ResourceManagerComponent->GetResourceAmount(Food[i]);

		if (curAmount > 0) {
			for (int32 j = 0; j < curAmount; j++) {
				Resources.Add(Food[i]);
			}
		}
	}

	if (!Resources.IsEmpty()) {
		int32 maxE = 25;

		int32 maxF = FMath::Clamp(Resources.Num(), 1, 3);
		int32 maxB = FMath::Clamp(Citizen->Balance, 1, 3);

		int32 quantity = FMath::Min(maxF, maxB);
		
		for (int32 i = 0; i < quantity; i++) {
			int32 selected = FMath::RandRange(0, Resources.Num());

			Camera->ResourceManagerComponent->TakeUniversalResource(Resources[selected], 1);

			Resources.RemoveAt(selected);
		}

		Citizen->Balance -= quantity;
		Camera->ResourceManagerComponent->AddUniversalResource(Money, quantity);

		maxE += quantity * 25;

		Citizen->StartGainEnergyTimer(maxE);
	}
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