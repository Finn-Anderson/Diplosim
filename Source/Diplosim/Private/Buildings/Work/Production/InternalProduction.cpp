#include "Buildings/Work/Production/InternalProduction.h"

#include "NiagaraComponent.h"

#include "AI/DiplosimAIController.h"
#include "AI/Citizen/Citizen.h"
#include "Buildings/Misc/Broch.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/DiplosimTimerManager.h"
#include "Player/Managers/ConquestManager.h"

AInternalProduction::AInternalProduction()
{
	PrimaryActorTick.bCanEverTick = false;

	MinYield = 1.0f;
	MaxYield = 5.0f;

	TimeLength = 10;

	bMultiplicitive = false;

	bNoTimer = false;
}

void AInternalProduction::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (!GetOccupied().Contains(Citizen) || bNoTimer)
		return;

	if (IsCapacityFull()) {
		ParticleComponent->Deactivate();

		return;
	}

	for (const FItemStruct& item : Intake) {
		if (item.Use > item.Stored) {
			CheckStored(Citizen, Intake);

			ParticleComponent->Deactivate();

			return;
		}
	}

	SetSocketLocation(Citizen);

	FTimerStruct* timer = Camera->TimerManager->FindTimer("Internal", this);

	if (timer == nullptr)
		SetTimer();
	else {
		AlterTimer();

		PauseTimer(false);
	}
}

void AInternalProduction::Leave(ACitizen* Citizen)
{
	Super::Leave(Citizen);

	if (!GetOccupied().Contains(Citizen) || bNoTimer)
		return;

	if (GetCitizensAtBuilding().IsEmpty())
		PauseTimer(true);
	else
		AlterTimer();
}

bool AInternalProduction::IsAtWork(ACitizen* Citizen)
{
	bool bWorking = Super::IsAtWork(Citizen);

	if (!bWorking) {
		AActor* goal = Citizen->AIController->MoveRequest.GetGoalActor();

		if (IsValid(goal)) {
			TArray<FItemStruct> items = Intake;

			if (IsValid(Citizen->Carrying.Type)) {
				FItemStruct item;
				item.Resource = Citizen->Carrying.Type->GetClass();
				item.Amount = Citizen->Carrying.Amount;

				items.Add(item);
			}

			for (const FItemStruct& item : items) {
				TMap<TSubclassOf<ABuilding>, int32> buildingTypes = Camera->ResourceManager->GetBuildings(item.Resource);

				for (auto& element : buildingTypes) {
					if (!goal->IsA(element.Key))
						continue;

					bWorking = true;

					break;
				}

				if (bWorking)
					break;
			}
		}
	}

	return bWorking;
}

void AInternalProduction::Production(ACitizen* Citizen)
{
	Super::Production(Citizen);

	TArray<TSubclassOf<AResource>> resources = Camera->ResourceManager->GetResources(this);

	if (!resources.IsEmpty()) {
		AResource* r = Cast<AResource>(resources[0]->GetDefaultObject());
		r->Camera = Camera;

		AResource* resource = r->GetHarvestedResource();
		FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

		float yield = MinYield;

		if (MinYield != MaxYield)
			yield = Camera->Stream.RandRange(MinYield, MaxYield);

		if (bMultiplicitive) {
			yield = FMath::CeilToInt32(Camera->ResourceManager->GetResourceAmount(FactionName, resource->GetClass()) * yield);

			Camera->ResourceManager->AddUniversalResource(faction, resource->GetClass(), yield);
		}
		else {
			TArray<ACitizen*> workers = GetCitizensAtBuilding();
			int32 index = Camera->Stream.RandRange(0, workers.Num() - 1);

			AActor* location = this;
			if (!resources.Contains(resource->GetClass()))
				location = faction->EggTimer;

			workers[index]->Carry(resource, yield, location);
		}
	}

	if (!IsCapacityFull())
		SetTimer();

	for (FItemStruct& item : Intake) {
		item.Stored -= item.Use;

		if (item.Use > item.Stored)
			for (ACitizen* citizen : GetCitizensAtBuilding())
				CheckStored(citizen, Intake);
	}
}

float AInternalProduction::GetTime()
{
	float time = TimeLength;

	for (ACitizen* citizen : GetCitizensAtBuilding())
		time -= (TimeLength / (Capacity * 2)) * citizen->GetProductivity();

	return time;
}

void AInternalProduction::SetTimer()
{
	TArray<FTimerParameterStruct> params;
	Camera->TimerManager->SetParameter(GetCitizensAtBuilding()[0], params);
	Camera->TimerManager->CreateTimer("Internal", this, GetTime(), "Production", params, false);
}

void AInternalProduction::AlterTimer()
{
	FTimerStruct* timer = Camera->TimerManager->FindTimer("Internal", this);

	if (timer == nullptr)
		return;

	timer->Target = GetTime();
}

void AInternalProduction::PauseTimer(bool bPause)
{
	FTimerStruct* timer = Camera->TimerManager->FindTimer("Internal", this);

	if (timer == nullptr)
		return;

	timer->bPaused = bPause;
}