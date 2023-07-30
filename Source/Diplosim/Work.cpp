#include "Work.h"

#include "Kismet/GameplayStatics.h"

#include "Citizen.h"
#include "Camera.h"
#include "ResourceManager.h"


AWork::AWork()
{
	Wage = 0;
}

void AWork::OnBuilt()
{
	GetWorldTimerManager().SetTimer(FindTimer, this, &AWork::FindCitizens, 30.0f, true, 2.0f);

	GetWorldTimerManager().SetTimer(CostTimer, this, &AWork::UpkeepCost, 300.0f, true);
}

void AWork::UpkeepCost()
{
	for (int i = 0; i < Occupied.Num(); i++) {
		ACitizen* c = Cast<ACitizen>(Occupied[i]);

		c->Balance += Wage;
	}

	int32 cost = Wage * Occupied.Num() + Upkeep;

	Camera->ResourceManager->ChangeResource(Money, -cost);
}

void AWork::FindCitizens()
{
	TArray<AActor*> citizens;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), citizens);

	for (int i = 0; i < citizens.Num(); i++) {
		ACitizen* c = Cast<ACitizen>(citizens[i]);

		if (c->Employment == nullptr) {
			AddCitizen(c);
		}
	}
}

void AWork::AddCitizen(ACitizen* Citizen)
{
	if (GetCapacity() > Occupied.Num()) {
		Occupied.Add(Citizen);

		Citizen->Employment = this;

		Citizen->MoveTo(this);
	}
	else {
		GetWorldTimerManager().ClearTimer(FindTimer);
	}
}

void AWork::RemoveCitizen(ACitizen* Citizen)
{
	if (Occupied.Contains(Citizen)) {
		Leave(Citizen);

		Occupied.Remove(Citizen);

		Citizen->Employment = nullptr;

		if (!GetWorldTimerManager().IsTimerActive(FindTimer)) {
			GetWorldTimerManager().SetTimer(FindTimer, this, &AWork::FindCitizens, 30.0f, true);
		}
	}
}