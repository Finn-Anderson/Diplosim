#include "Buildings/Work/Service/Religion.h"

#include "Components/DecalComponent.h"

#include "Buildings/House.h"
#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"

AReligion::AReligion()
{
	MassLength = 120;
	WaitLength = 900;

	DecalComponent->SetVisibility(true);

	bRange = true;

	bMass = true;
}

void AReligion::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	FTimerStruct timer;
	timer.CreateTimer(this, MassLength, FTimerDelegate::CreateUObject(this, &AReligion::Mass), false);

	if (GetOccupied().Contains(Citizen) && !Camera->CitizenManager->Timers.Contains(timer))
		Mass();

	Worshipping.Add(Citizen);

	if (GetCitizensAtBuilding().Num() == 1)
		PauseTimer(false);
}

void AReligion::Leave(ACitizen* Citizen)
{
	Super::Leave(Citizen);

	Worshipping.Remove(Citizen);

	if (GetCitizensAtBuilding().IsEmpty())
		PauseTimer(true);
}

void AReligion::Mass()
{
	bMass = !bMass;

	if (bMass) {
		for (AHouse* house : Houses) {
			for (ACitizen* citizen : house->GetOccupied()) {
				if (citizen->Spirituality.Faith.Religion == Belief.Religion)
					citizen->AIController->AIMoveTo(this);
			}
		}

		FTimerStruct timer;
		timer.CreateTimer(this, MassLength, FTimerDelegate::CreateUObject(this, &AReligion::Mass), false);

		Camera->CitizenManager->Timers.Add(timer);
	}
	else {
		for (AHouse* house : Houses) {
			for (ACitizen* citizen : house->GetOccupied()) {
				citizen->Spirituality.bBoost = false;
				citizen->AIController->DefaultAction();
			}
		}
				
		for (ACitizen* citizen : Worshipping)
			citizen->Spirituality.bBoost = true;

		FTimerStruct timer;
		timer.CreateTimer(this, WaitLength, FTimerDelegate::CreateUObject(this, &AReligion::Mass), false);

		Camera->CitizenManager->Timers.Add(timer);
	}
}

void AReligion::PauseTimer(bool bPause)
{
	FTimerStruct timer;
	timer.CreateTimer(this, MassLength, FTimerDelegate::CreateUObject(this, &AReligion::Mass), false);

	int32 index = Camera->CitizenManager->Timers.Find(timer);

	Camera->CitizenManager->Timers[index].bPaused = bPause;
}