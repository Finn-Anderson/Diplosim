#include "Player/Managers/CitizenManager.h"

#include "AI/Citizen.h"

UCitizenManager::UCitizenManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetComponentTickInterval(1.0f);
}

void UCitizenManager::BeginPlay()
{
	Super::BeginPlay();
}

void UCitizenManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	for (ACitizen* citizen : Citizens) {
		citizen->HungerTimer++;
		citizen->EnergyTimer++;
		citizen->AgeTimer++;

		if (citizen->HungerTimer == 3)
			citizen->Eat();

		if (citizen->EnergyTimer == 6)
			citizen->CheckGainOrLoseEnergy();

		if (citizen->AgeTimer == 45)
			citizen->Birthday();
	}
}

void UCitizenManager::StartTimers()
{
	SetComponentTickEnabled(true);
}