#include "Buildings/Work/Service/Religion.h"

#include "Components/DecalComponent.h"

#include "Buildings/House.h"
#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"

AReligion::AReligion()
{
	MassLength = 120.0f;
	WaitLength = 900.0f;

	DecalComponent->SetVisibility(true);

	bRange = true;

	bMass = true;
}

void AReligion::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (GetOccupied().Contains(Citizen) && !GetWorld()->GetTimerManager().IsTimerActive(MassTimer))
		Mass();

	Worshipping.Add(Citizen);
}

void AReligion::Leave(ACitizen* Citizen)
{
	Super::Leave(Citizen);

	Worshipping.Remove(Citizen);
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

		GetWorld()->GetTimerManager().SetTimer(MassTimer, this, &AReligion::Mass, MassLength, false);
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

		GetWorld()->GetTimerManager().SetTimer(MassTimer, this, &AReligion::Mass, WaitLength, false);
	}
}