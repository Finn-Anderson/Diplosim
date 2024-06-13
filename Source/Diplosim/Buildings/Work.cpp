#include "Work.h"

#include "Kismet/GameplayStatics.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
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

	int32 upkeep = Wage * Occupied.Num();
	Camera->ResourceManagerComponent->TakeUniversalResource(Money, upkeep, -100000);
}

void AWork::FindCitizens()
{
	TArray<AActor*> citizens;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), citizens);

	for (int i = 0; i < citizens.Num(); i++) {
		ACitizen* c = Cast<ACitizen>(citizens[i]);

		if (!c->CanWork() || c->Building.Employment != nullptr || !c->AIController->CanMoveTo(GetActorLocation()))
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

void AWork::Production(ACitizen* Citizen)
{
	StoreResource(Citizen);
}