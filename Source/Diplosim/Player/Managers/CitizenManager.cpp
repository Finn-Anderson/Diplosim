#include "Player/Managers/CitizenManager.h"

#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "Components/WidgetComponent.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Universal/HealthComponent.h"
#include "Buildings/Work/Service/Clinic.h"

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

	DiseaseTimerTarget = FMath::RandRange(180, 1200);
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

	citizen->DiseaseNiagaraComponent->Activate();
	citizen->PopupComponent->SetHiddenInGame(false);

	citizen->SetActorTickEnabled(true);

	citizen->SetPopupImageState("Add", "Disease");

	DiseaseTimer = 0;

	DiseaseTimerTarget = FMath::RandRange(180, 1200);

	TArray<AActor*> clinics;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AClinic::StaticClass(), clinics);

	for (AActor* actor : clinics)
		for (ACitizen* healer : Cast<AClinic>(actor)->GetCitizensAtBuilding())
			PickCitizenToHeal(healer);
}

void UCitizenManager::PickCitizenToHeal(ACitizen* Healer)
{
	for (ACitizen* citizen : Citizens) {
		if (Healing.Contains(citizen))
			continue;

		for (FDiseaseStruct disease : citizen->CaughtDiseases) {
			Healer->AIController->AIMoveTo(citizen);

			Healing.Add(citizen);
		}
	}
}