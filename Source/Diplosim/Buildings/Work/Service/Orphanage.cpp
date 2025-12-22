#include "Buildings/Work/Service/Orphanage.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "AI/BuildingComponent.h"
#include "Buildings/House.h"
#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/DiplosimTimerManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Managers/PoliticsManager.h"

AOrphanage::AOrphanage()
{

}

void AOrphanage::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (IsValid(GetOccupant(Citizen))) {
		Citizen->bGain = true;

		Camera->TimerManager->UpdateTimerLength("Energy", this, 1);
	}
	else {
		PickChildren(Citizen);
	}
}

void AOrphanage::Leave(ACitizen* Citizen)
{
	Super::Leave(Citizen);

	if (!IsValid(GetOccupant(Citizen)))
		return;

	Citizen->bGain = false;

	int32 timeToCompleteDay = Camera->Grid->AtmosphereComponent->GetTimeToCompleteDay();
	Camera->TimerManager->UpdateTimerLength("Energy", this, (timeToCompleteDay / 100) * Citizen->EnergyMultiplier);
}

void AOrphanage::AddVisitor(ACitizen* Occupant, ACitizen* Visitor)
{
	Visitor->BuildingComponent->Orphanage = this;

	Super::AddVisitor(Occupant, Visitor);
}

void AOrphanage::RemoveVisitor(ACitizen* Occupant, ACitizen* Visitor)
{
	Visitor->BuildingComponent->Orphanage = nullptr;

	Camera->TimerManager->RemoveTimer("Orphanage", Visitor);

	Super::RemoveVisitor(Occupant, Visitor);
}

void AOrphanage::Kickout(ACitizen* Citizen)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	if (Camera->PoliticsManager->GetLawValue(faction->Name, "Work Age") > Citizen->BioStruct.Age && IsValid(GetOccupant(Citizen)))
		return;

	RemoveVisitor(GetOccupant(Citizen), Citizen);
}

void AOrphanage::PickChildren(ACitizen* Citizen)
{
	int32 money = Citizen->GetLeftoverMoney();

	if (Citizen->BioStruct.Partner != nullptr)
		money += Citizen->GetLeftoverMoney();
	
	TArray<ACitizen*> favourites;
	
	for (ACitizen* worker : GetOccupied()) {
		for (ACitizen* child : GetVisitors(worker)) {
			int32 count = 0;

			for (FPersonality* personality : Camera->CitizenManager->GetCitizensPersonalities(Citizen)) {
				for (FPersonality* p : Camera->CitizenManager->GetCitizensPersonalities(child)) {
					if (personality->Trait == p->Trait)
						count += 2;
					else if (personality->Likes.Contains(p->Trait))
						count++;
					else if (personality->Dislikes.Contains(p->Trait))
						count--;
				}
			}

			if (count < 0)
				continue;

			favourites.Add(child);
		}
	}

	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	int32 maxF = FMath::CeilToInt((100 - Citizen->Hunger) / (25.0f * Citizen->FoodMultiplier));
	int32 cost = Camera->PoliticsManager->GetLawValue(faction->Name, "Food Cost");

	int32 amount = FMath::Min(Citizen->BuildingComponent->House->Space - Citizen->BuildingComponent->House->GetVisitors(Citizen->BuildingComponent->House->GetOccupant(Citizen)).Num(), money / (maxF * cost));

	for (int32 i = 0; i < amount; i++) {
		int32 index = Camera->Stream.RandRange(0, favourites.Num() - 1);

		ACitizen* child = favourites[index];

		if (Citizen->BioStruct.Sex == ESex::Female)
			child->BioStruct.Mother = Citizen;
		else
			child->BioStruct.Father = Citizen;

		if (Citizen->BioStruct.Partner != nullptr) {
			if (Citizen->BioStruct.Partner->BioStruct.Sex == ESex::Female)
				child->BioStruct.Mother = Citizen->BioStruct.Partner;
			else
				child->BioStruct.Father = Citizen->BioStruct.Partner;

			Citizen->BioStruct.Partner->BioStruct.Children.Add(child);
		}
		
		for (ACitizen* c : Citizen->BioStruct.Children) {
			c->BioStruct.Siblings.Add(child);
			child->BioStruct.Siblings.Add(c);
		}

		Citizen->BioStruct.Children.Add(child);

		Citizen->BuildingComponent->House->AddVisitor(Citizen->BuildingComponent->House->GetOccupant(Citizen), child);

		child->AIController->AIMoveTo(Citizen->BuildingComponent->House);

		child->Camera->TimerManager->CreateTimer("Idle", child, 60.0f, "DefaultAction", {}, false, true);

		RemoveVisitor(GetOccupant(child), child);

		favourites.RemoveAt(index);
	}

	Citizen->AIController->AIMoveTo(Citizen->BuildingComponent->House);

	Citizen->Camera->TimerManager->CreateTimer("Idle", Citizen, 60.0f, "DefaultAction", {}, false, true);
}