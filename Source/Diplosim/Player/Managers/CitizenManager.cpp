#include "Player/Managers/CitizenManager.h"

#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/KismetArrayLibrary.h"

#include "AI/Citizen.h"
#include "AI/AttackComponent.h"
#include "AI/DiplosimAIController.h"
#include "Universal/HealthComponent.h"
#include "Buildings/Work/Service/Clinic.h"
#include "Player/Camera.h"

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

	Async(EAsyncExecution::Thread, [this]() {
		for (ABuilding* building : Buildings) {
			building->UpkeepTimer++;

			if (building->UpkeepTimer == 300)
				building->UpkeepCost();
		}

		Shuffle();

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

			for (FConditionStruct condition : citizen->HealthIssues) {
				condition.Level++;

				if (condition.Level == condition.DeathLevel)
					citizen->HealthComponent->TakeHealth(100, citizen);
			}

			citizen->FindJobAndHouse();
		}

		DiseaseTimer++;

		if (DiseaseTimer == DiseaseTimerTarget)
			SpawnDisease();
	});
}

void UCitizenManager::StartTimers()
{
	SetComponentTickEnabled(true);
}

void UCitizenManager::Shuffle()
{
	for (int32 i = 0; i < Citizens.Num(); i++) {
		int32 index = FMath::RandRange(i, Citizens.Num() - 1);

		if (index == i)
			continue;

		Citizens.Swap(i, index);
	}
}

//
// Disease
//
void UCitizenManager::SpawnDisease()
{
	int32 index = FMath::RandRange(0, Infectible.Num() - 1);
	ACitizen* citizen = Infectible[index];

	index = FMath::RandRange(0, Diseases.Num() - 1);
	citizen->HealthIssues.Add(Diseases[index]);

	Infect(citizen);

	DiseaseTimer = 0;

	DiseaseTimerTarget = FMath::RandRange(180, 1200);
}

void UCitizenManager::Infect(ACitizen* Citizen)
{
	Citizen->DiseaseNiagaraComponent->Activate();
	Citizen->PopupComponent->SetHiddenInGame(false);

	Citizen->SetActorTickEnabled(true);

	AsyncTask(ENamedThreads::GameThread, [this, Citizen]() { Citizen->SetPopupImageState("Add", "Disease"); });

	Infected.Add(Citizen);

	UpdateHealthText(Citizen);

	TArray<AActor*> clinics;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AClinic::StaticClass(), clinics);

	for (AActor* actor : clinics)
		for (ACitizen* healer : Cast<AClinic>(actor)->GetCitizensAtBuilding())
			PickCitizenToHeal(healer);
}

void UCitizenManager::Injure(ACitizen* Citizen)
{
	int32 index = FMath::RandRange(1, 100);

	if (index < 90)
		return;
	
	TArray<FConditionStruct> conditions;

	for (FConditionStruct condition : Injuries)
		if (condition.Buildings.Contains(Citizen->Building.Employment->GetClass()))
			conditions.Add(condition);

	index = FMath::RandRange(0, conditions.Num() - 1);
	Citizen->HealthIssues.Add(conditions[index]);

	for (FAffectStruct affect : conditions[index].Affects) {
		if (affect.Affect == EAffect::Movement)
			Citizen->InitialSpeed *= affect.Amount;
		else if (affect.Affect == EAffect::Damage)
			Citizen->AttackComponent->Damage *= affect.Amount;
		else {
			Citizen->HealthComponent->MaxHealth *= affect.Amount;

			if (Citizen->HealthComponent->Health > Citizen->HealthComponent->MaxHealth)
				Citizen->HealthComponent->Health = Citizen->HealthComponent->MaxHealth;
		}
	}

	Injured.Add(Citizen);

	UpdateHealthText(Citizen);

	TArray<AActor*> clinics;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AClinic::StaticClass(), clinics);

	for (AActor* actor : clinics)
		for (ACitizen* healer : Cast<AClinic>(actor)->GetCitizensAtBuilding())
			PickCitizenToHeal(healer);
}

void UCitizenManager::Cure(ACitizen* Citizen)
{
	for (FConditionStruct condition : Citizen->HealthIssues) {
		for (FAffectStruct affect : condition.Affects) {
			if (affect.Affect == EAffect::Movement)
				Citizen->InitialSpeed /= affect.Amount;
			else if (affect.Affect == EAffect::Damage)
				Citizen->AttackComponent->Damage /= affect.Amount;
			else
				Citizen->HealthComponent->MaxHealth /= affect.Amount;
		}
	}

	Citizen->HealthIssues.Empty();

	Citizen->DiseaseNiagaraComponent->Deactivate();

	if (Citizen->Hunger > 25) {
		Citizen->PopupComponent->SetHiddenInGame(true);

		Citizen->SetActorTickEnabled(false);
	}

	AsyncTask(ENamedThreads::GameThread, [this, Citizen]() { Citizen->SetPopupImageState("Remove", "Disease"); });

	Infected.Remove(Citizen);
	Injured.Remove(Citizen);
	Healing.Remove(Citizen);

	UpdateHealthText(Citizen);
}

void UCitizenManager::UpdateHealthText(ACitizen* Citizen)
{
	AsyncTask(ENamedThreads::GameThread, [this, Citizen]() {
		ACamera* camera = Cast<ACamera>(GetOwner());

		if (camera->WidgetComponent->IsAttachedTo(Citizen->GetRootComponent()))
			camera->UpdateHealthIssues();
	});
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