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
#include "Buildings/Work/Service/Religion.h"
#include "Player/Camera.h"
#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"

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
// Work
//
void UCitizenManager::CheckWorkStatus(int32 Hour)
{
	for (ACitizen* citizen : Citizens) {
		if (citizen->Building.Employment == nullptr)
			continue;

		AWork* work = citizen->Building.Employment;

		if (work->WorkStart == Hour)
			work->Open();
		else if (work->WorkEnd == Hour)
			work->Close();
	}
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

//
// Event
//
void UCitizenManager::ExecuteEvent(FString Period, int32 Day, int32 Hour)
{
	for (FEventStruct eventStruct : Events) {
		FString command = "";

		for (FEventTimeStruct times : eventStruct.Times) {
			if (times.Period != Period && times.Day != Day)
				continue;

			if (times.StartHour == Hour)
				command = "start";
			else if (times.EndHour == Hour)
				command = "end";

			if (command != "")
				break;
		}

		if (command == "")
			continue;

		Cast<ACamera>(GetOwner())->DisplayEvent("Event", UEnum::GetValueAsString(eventStruct.Type));

		if (eventStruct.Type == EEventType::Mass) {
			if (command == "start")
				CallMass(eventStruct.Buildings);
			else
				EndMass(Cast<AReligion>(eventStruct.Buildings[0]->GetDefaultObject())->Belief);

			continue;
		}

		for (TSubclassOf<ABuilding> Building : eventStruct.Buildings) {
			TArray<AActor*> actors;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), Building, actors);

			for (AActor* actor : actors) {
				if (command == "start")
					Cast<AWork>(actor)->Close();
				else
					Cast<AWork>(actor)->Open();
			}
		}
	}
}

void UCitizenManager::CallMass(TArray<TSubclassOf<ABuilding>> BuildingList)
{
	for (TSubclassOf<ABuilding> building : BuildingList) {
		TArray<AActor*> actors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), building, actors);

		for (ACitizen* citizen : Citizens) {
			if (Cast<AReligion>(actors[0])->Belief != citizen->Spirituality.Faith || citizen->Building.Employment->IsA<AReligion>() || !citizen->Building.Employment->bCanAttendEvents)
				continue;

			AReligion* chosenBuilding = nullptr;

			for (AActor* actor : actors) {
				AReligion* religiousBuilding = Cast<AReligion>(actor);

				if (!citizen->AIController->CanMoveTo(religiousBuilding->GetActorLocation()) || religiousBuilding->GetOccupied().IsEmpty())
					continue;

				if (chosenBuilding == nullptr) {
					chosenBuilding = religiousBuilding;

					continue;
				}

				double magnitude = citizen->AIController->GetClosestActor(citizen->GetActorLocation(), chosenBuilding->GetActorLocation(), religiousBuilding->GetActorLocation());

				if (magnitude <= 0.0f)
					continue;

				chosenBuilding = religiousBuilding;
			}

			if (chosenBuilding != nullptr)
				citizen->AIController->AIMoveTo(chosenBuilding);
		}
	}
}

void UCitizenManager::EndMass(EReligion Belief)
{
	for (ACitizen* citizen : Citizens) {
		if (Belief != citizen->Spirituality.Faith)
			continue;

		if (citizen->bWorshipping)
			citizen->SetMassStatus(EMassStatus::Attended);
		else
			citizen->SetMassStatus(EMassStatus::Missed);

		int32 timeToCompleteDay = 360 / (24 * citizen->Camera->Grid->AtmosphereComponent->Speed);

		FTimerStruct timer;
		timer.CreateTimer(citizen, timeToCompleteDay * 2, FTimerDelegate::CreateUObject(citizen, &ACitizen::SetMassStatus, EMassStatus::Neutral), false);

		Timers.Add(timer);

		citizen->AIController->DefaultAction();
	}
}