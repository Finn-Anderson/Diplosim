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
	PrimaryComponentTick.bCanEverTick = false;
}

void UCitizenManager::BeginPlay()
{
	Super::BeginPlay();

	StartDiseaseTimer();
}

void UCitizenManager::Loop()
{
	for (int32 i = Timers.Num() - 1; i > -1; i--) {
		if ((!Citizens.Contains(Timers[i].Actor) && !Buildings.Contains(Timers[i].Actor)) || (Timers[i].Actor->IsA<ACitizen>() && Cast<ACitizen>(Timers[i].Actor)->Rebel)) {
			Timers.RemoveAt(i);

			continue;
		}

		if (Timers[i].bPaused)
			continue;

		Timers[i].Timer++;

		if (Timers[i].Timer >= Timers[i].Target) {
			Timers[i].Delegate.ExecuteIfBound();

			if (Timers[i].bRepeat)
				Timers[i].Timer = 0;
			else
				Timers.RemoveAt(i);
		}
	}

	for (ACitizen* citizen : Citizens) {
		if (citizen->Rebel)
			continue;

		for (FConditionStruct condition : citizen->HealthIssues) {
			condition.Level++;

			if (condition.Level == condition.DeathLevel)
				citizen->HealthComponent->TakeHealth(100, citizen);
		}

		citizen->FindJobAndHouse();
			
		citizen->SetHappiness();
	}

	AsyncTask(ENamedThreads::GameThread, [this]() {
		FTimerHandle FLoopTimer;
		GetWorld()->GetTimerManager().SetTimer(FLoopTimer, this, &UCitizenManager::StartTimers, 1.0f);
	});
}

void UCitizenManager::StartTimers()
{
	Async(EAsyncExecution::Thread, [this]() { Loop(); });
}

void UCitizenManager::RemoveTimer(AActor* Actor, FTimerDelegate TimerDelegate)
{
	int32 index;

	FTimerStruct timer;
	timer.Actor = Actor;
	timer.Delegate = TimerDelegate;

	index = Timers.Find(timer);

	if (index == INDEX_NONE)
		return;

	Timers.RemoveAt(index);
}

//
// Disease
//
void UCitizenManager::StartDiseaseTimer()
{
	FTimerStruct timer;
	timer.CreateTimer(GetOwner(), FMath::RandRange(180, 1200), FTimerDelegate::CreateUObject(this, &UCitizenManager::SpawnDisease), false);

	Timers.Add(timer);
}

void UCitizenManager::SpawnDisease()
{
	int32 index = FMath::RandRange(0, Infectible.Num() - 1);
	ACitizen* citizen = Infectible[index];

	index = FMath::RandRange(0, Diseases.Num() - 1);
	citizen->HealthIssues.Add(Diseases[index]);

	Infect(citizen);

	StartDiseaseTimer();
}

void UCitizenManager::Infect(ACitizen* Citizen)
{
	Citizen->DiseaseNiagaraComponent->Activate();
	Citizen->PopupComponent->SetHiddenInGame(false);

	Citizen->PopupComponent->SetComponentTickEnabled(true);

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

	if (index < 95)
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

		Citizen->PopupComponent->SetComponentTickEnabled(false);
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