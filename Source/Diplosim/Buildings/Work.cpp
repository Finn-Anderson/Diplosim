#include "Work.h"

#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

#include "AI/Citizen.h"
#include "Player/Camera.h"
#include "Player/ResourceManager.h"


AWork::AWork()
{
	Wage = 0;
}

void AWork::UpkeepCost()
{
	for (int i = 0; i < Occupied.Num(); i++) {
		ACitizen* c = Cast<ACitizen>(Occupied[i]);

		c->Balance += Wage;
	}

	int32 cost = Wage * Occupied.Num() + Upkeep;

	if (!Camera->ResourceManagerComponent->TakeUniversalResource(Money, cost)) {
		// Shutdown building;
	}
}

void AWork::FindCitizens()
{
	TArray<AActor*> citizens;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), citizens);

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	for (int i = 0; i < citizens.Num(); i++) {
		ACitizen* c = Cast<ACitizen>(citizens[i]);

		const ANavigationData* NavData = nav->GetNavDataForProps(c->GetNavAgentPropertiesRef());

		FPathFindingQuery query(this, *NavData, GetActorLocation(), c->GetActorLocation());
		query.bAllowPartialPaths = false;

		bool path = nav->TestPathSync(query, EPathFindingMode::Hierarchical);

		if (c->Employment == nullptr && path) {
			AddCitizen(c);
		}
	}

	if (GetCapacity() > Occupied.Num()) {
		GetWorldTimerManager().SetTimer(FindTimer, this, &AWork::FindCitizens, 30.0f, false);
	}
}

void AWork::AddCitizen(ACitizen* Citizen)
{
	if (GetCapacity() > Occupied.Num()) {
		Occupied.Add(Citizen);

		Citizen->Employment = this;

		Citizen->MoveTo(this);

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

		if (GetCapacity() == Occupied.Num()) {
			GetWorldTimerManager().ClearTimer(FindTimer);
		}
	}
}

void AWork::RemoveCitizen(ACitizen* Citizen)
{
	if (Occupied.Contains(Citizen)) {
		Leave(Citizen);

		Occupied.Remove(Citizen);

		Citizen->Employment = nullptr;

		if (!GetWorldTimerManager().IsTimerActive(FindTimer)) {
			GetWorldTimerManager().SetTimer(FindTimer, this, &AWork::FindCitizens, 30.0f, false);
		}
	}
}

void AWork::Store(int32 Amount, ACitizen* Citizen)
{
	if (Camera->ResourceManagerComponent->AddLocalResource(this, Amount)) {
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