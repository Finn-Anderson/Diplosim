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

	DiseaseTimer = DiseaseTimerTarget - 10;
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
	int32 index = FMath::RandRange(0, Infectible.Num() - 1);
	ACitizen* citizen = Infectible[index];

	index = FMath::RandRange(0, Diseases.Num() - 1);
	citizen->CaughtDiseases.Add(Diseases[index]);

	Infect(citizen);

	DiseaseTimer = 0;

	DiseaseTimerTarget = FMath::RandRange(180, 1200);
}

void UCitizenManager::Infect(ACitizen* Citizen)
{
	Citizen->DiseaseNiagaraComponent->Activate();
	Citizen->PopupComponent->SetHiddenInGame(false);

	Citizen->SetActorTickEnabled(true);

	Citizen->SetPopupImageState("Add", "Disease");

	Infected.Add(Citizen);

	TArray<AActor*> clinics;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AClinic::StaticClass(), clinics);

	for (AActor* actor : clinics)
		for (ACitizen* healer : Cast<AClinic>(actor)->GetCitizensAtBuilding())
			PickCitizenToHeal(healer);
}

void UCitizenManager::Cure(ACitizen* Citizen)
{
	Citizen->CaughtDiseases.Empty();

	Citizen->DiseaseNiagaraComponent->Deactivate();

	if (Citizen->Hunger > 25) {
		Citizen->PopupComponent->SetHiddenInGame(true);

		Citizen->SetActorTickEnabled(false);
	}

	Citizen->SetPopupImageState("Remove", "Disease");

	Infected.Remove(Citizen);
	Healing.Remove(Citizen);

	Infectible.Remove(Citizen);
}

void UCitizenManager::PickCitizenToHeal(ACitizen* Healer)
{
	ACitizen* goal = nullptr;

	for (ACitizen* citizen : Infected) {
		if (Healing.Contains(citizen))
			continue;

		goal = citizen;

		break;
	}

	if (goal != nullptr) {
		Healer->AIController->AIMoveTo(goal);

		Healing.Add(goal);
	}
	else if (Healer->Building.BuildingAt != Healer->Building.Employment)
		Healer->AIController->DefaultAction();
}