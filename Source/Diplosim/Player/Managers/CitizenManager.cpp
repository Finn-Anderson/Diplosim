#include "Player/Managers/CitizenManager.h"

#include "AI/Citizen.h"
#include "Universal/HealthComponent.h"

UCitizenManager::UCitizenManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetComponentTickInterval(1.0f);

	DiseaseTimer = 0;
}

void UCitizenManager::BeginPlay()
{
	Super::BeginPlay();

	DiseaseTimerTarget = FMath::RandRange(180, 600);
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

		for (FDiseaseStruct disease : citizen->CaughtDiseases) {
			disease.Level++;

			if (disease.Level == disease.DeathLevel)
				citizen->HealthComponent->TakeHealth(100, citizen);
		}
	}

	DiseaseTimer++;

	if (DiseaseTimer == DiseaseTimerTarget)
		SpawnDisease();
}

void UCitizenManager::StartTimers()
{
	SetComponentTickEnabled(true);
}

//
// Disease
//
void UCitizenManager::SpawnDisease()
{
	int32 index = FMath::RandRange(0, Citizens.Num() - 1);
	ACitizen* citizen = Citizens[index];

	index = FMath::RandRange(0, Diseases.Num() - 1);
	citizen->CaughtDiseases.Add(Diseases[index]);

	DiseaseTimerTarget = FMath::RandRange(180, 600);
}