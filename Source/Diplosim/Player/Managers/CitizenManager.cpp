#include "Player/Managers/CitizenManager.h"

#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"

#include "AI/Citizen.h"
#include "AI/AttackComponent.h"
#include "AI/DiplosimAIController.h"
#include "AI/AIMovementComponent.h"
#include "Universal/HealthComponent.h"
#include "Buildings/Work/Service/Clinic.h"
#include "Buildings/Work/Service/Religion.h"
#include "Player/Camera.h"
#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"

UCitizenManager::UCitizenManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	CooldownTimer = 0;

	BrochLocation = FVector::Zero();
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

	for (FLeaderStruct& leader : Leaders) {
		if (leader.Leader != nullptr)
			continue;

		SelectNewLeader(leader.Party);
	}

	int32 rebelCount = 0;

	for (ACitizen* citizen : Citizens) {
		if (citizen->Rebel)
			continue;

		for (FConditionStruct &condition : citizen->HealthIssues) {
			condition.Level++;

			if (condition.Level == condition.DeathLevel)
				citizen->HealthComponent->TakeHealth(100, citizen);
		}

		citizen->FindJobAndHouse();
			
		citizen->SetHappiness();

		if (citizen->Politics.Ideology.Party == EParty::Freedom)
			rebelCount++;
	}

	if (Citizens.Num() > 0 && (rebelCount / Citizens.Num()) * 100 > 33 && !IsRebellion()) {
		CooldownTimer--;

		if (CooldownTimer < 1)
			Overthrow();
	}

	AsyncTask(ENamedThreads::GameThread, [this]() {
		Cast<ACamera>(GetOwner())->UpdateUI();

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

void UCitizenManager::ResetTimer(AActor* Actor, FTimerDelegate TimerDelegate)
{
	int32 index;

	FTimerStruct timer;
	timer.Actor = Actor;
	timer.Delegate = TimerDelegate;

	index = Timers.Find(timer);

	if (index == INDEX_NONE)
		return;

	Timers[index].Timer = 0;
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
			Citizen->MovementComponent->InitialSpeed *= affect.Amount;
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
				Citizen->MovementComponent->InitialSpeed /= affect.Amount;
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
				AWork* work = Cast<AWork>(actor);

				if (!work->bCanAttendEvents)
					continue;

				if (command == "start")
					work->Close();
				else if (work->WorkStart <= Hour && work->WorkEnd > Hour)
					work->Open();
			}
		}
	}
}

bool UCitizenManager::IsWorkEvent(AWork* Work)
{
	for (FEventStruct eventStruct : Events) {
		if (!eventStruct.Buildings.Contains(Work->GetClass()))
			continue;

		UAtmosphereComponent* atmosphere = Cast<ACamera>(GetOwner())->Grid->AtmosphereComponent;

		for (FEventTimeStruct times : eventStruct.Times) {
			if (times.Period != atmosphere->Calendar.Period && times.Day != atmosphere->Calendar.Days[atmosphere->Calendar.Index])
				continue;

			if (atmosphere->Calendar.Hour >= times.StartHour && atmosphere->Calendar.Hour < times.EndHour)
				return true;
		}
	}

	return false;
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

//
// Politics
//
void UCitizenManager::SelectNewLeader(EParty Party)
{
	TArray<ACitizen*> candidates;

	for (ACitizen* citizen : Citizens) {
		if (citizen->Politics.Ideology.Party != Party || citizen->bHasBeenLeader)
			continue;

		if (candidates.Num() < 3)
			candidates.Add(citizen);
		else {
			ACitizen* lowest = nullptr;

			for (ACitizen* candidate : candidates)
				if (lowest == nullptr || lowest->Politics.Ideology.Leaning > candidate->Politics.Ideology.Leaning)
					lowest = candidate;

			if (citizen->Politics.Ideology.Leaning > lowest->Politics.Ideology.Leaning) {
				candidates.Remove(lowest);

				candidates.Add(citizen);
			}
		}
	}

	auto value = Async(EAsyncExecution::TaskGraph, [candidates]() { return FMath::RandRange(0, candidates.Num() - 1); });

	ACitizen* chosen = candidates[value.Get()];

	FLeaderStruct leaderStruct;
	leaderStruct.Party = Party;

	int32 index = Leaders.Find(leaderStruct);

	Leaders[index].Leader = chosen;

	chosen->bHasBeenLeader = true;
	chosen->Politics.Ideology.Leaning = ESway::Radical;
}

void UCitizenManager::StartElectionTimer()
{
	int32 timeToCompleteDay = 360 / (24 * Cast<ACamera>(GetOwner())->Grid->AtmosphereComponent->Speed);

	FTimerStruct timer;
	timer.CreateTimer(GetOwner(), timeToCompleteDay / 0.1f, FTimerDelegate::CreateUObject(this, &UCitizenManager::Election), true);

	Timers.Add(timer);
}

void UCitizenManager::Election()
{
	Representatives.Empty();

	TMap<EParty, TArray<ACitizen*>> tally;
	tally.Add(EParty::Environmentalist);
	tally.Add(EParty::Freedom);
	tally.Add(EParty::Industrialist);
	tally.Add(EParty::Militarist);
	tally.Add(EParty::Religious);

	for (TPair<EParty, TArray<ACitizen*>>& pair : tally)
		for (ACitizen* citizen : Citizens)
			if (citizen->Politics.Ideology.Party == pair.Key)
				pair.Value.Add(citizen);

	for (TPair<EParty, TArray<ACitizen*>>& pair : tally) {
		int32 number = FMath::RoundHalfFromZero(pair.Value.Num() / (float)Citizens.Num() * 100.0f / GetLawValue(EBillType::RepresentativesNum));

		if (number == 0)
			continue;

		number -= 1;

		FLeaderStruct leaderStruct;
		leaderStruct.Party = pair.Key;

		int32 index = Leaders.Find(leaderStruct);

		Representatives.Add(Leaders[index].Leader);

		pair.Value.Remove(Leaders[index].Leader);

		for (int32 i = 0; i < number; i++) {
			if (pair.Value.IsEmpty())
				continue;

			auto value = Async(EAsyncExecution::TaskGraph, [pair]() { return FMath::RandRange(0, pair.Value.Num() - 1); });

			ACitizen* citizen = pair.Value[value.Get()];

			Representatives.Add(citizen);

			pair.Value.Remove(citizen);
		}
	}

	ResetTimer(GetOwner(), FTimerDelegate::CreateUObject(this, &UCitizenManager::Election));
}

void UCitizenManager::ProposeBill(FBillStruct Bill)
{
	Votes.Clear();

	for (ACitizen* citizen : Representatives) {
		FTimerHandle VerdictTimer;
		GetWorld()->GetTimerManager().SetTimer(VerdictTimer, FTimerDelegate::CreateUObject(this, &UCitizenManager::GetVerdict, citizen, Bill), 0.1f, false);
	}
}

void UCitizenManager::GetVerdict(ACitizen* Representative, FBillStruct Bill)
{
	TArray<FString> verdict;

	if (Bill.Agreeing.Contains(Representative->Politics.Ideology.Party))
		verdict = { "Agreeing", "Agreeing", "Agreeing", "Agreeing", "Agreeing", "Agreeing", "Agreeing", "Abstaining", "Abstaining", "Opposing" };
	else if (Bill.Opposing.Contains(Representative->Politics.Ideology.Party))
		verdict = { "Opposing", "Opposing", "Opposing", "Opposing", "Opposing", "Opposing", "Opposing", "Abstaining", "Abstaining", "Agreeing" };
	else
		verdict = { "Abstaining", "Abstaining", "Abstaining", "Abstaining", "Abstaining", "Abstaining", "Agreeing", "Agreeing", "Opposing", "Opposing" };

	auto value = Async(EAsyncExecution::TaskGraph, [verdict]() { return FMath::RandRange(0, verdict.Num() - 1); });

	FString result = verdict[value.Get()];

	if (result == "Agreeing")
		Votes.For.Add(Representative);
	else if (result == "Opposing")
		Votes.Against.Add(Representative);

	if (Representatives.Last() == Representative)
		TallyVotes(Bill);
}

void UCitizenManager::TallyVotes(FBillStruct Bill)
{
	if (Votes.For.Num() <= Votes.Against.Num()) {
		Cast<ACamera>(GetOwner())->LawPassed(false);
	}
	else {
		for (FLawStruct &law : Laws) {
			if (!law.Bills.Contains(Bill))
				continue;

			FBillStruct currentLaw;
			currentLaw.bIsLaw = true;

			int32 index = law.Bills.Find(currentLaw);
			law.Bills[index].bIsLaw = false;

			index = law.Bills.Find(Bill);
			law.Bills[index].bIsLaw = true;
		}

		Cast<ACamera>(GetOwner())->LawPassed(true);
	}
}

float UCitizenManager::GetLawValue(EBillType BillType)
{
	FLawStruct lawStruct;
	lawStruct.BillType = BillType;

	int32 index = Laws.Find(lawStruct);

	FBillStruct billStruct;
	billStruct.bIsLaw = true;

	int32 index2 = Laws[index].Bills.Find(billStruct);
	
	return Laws[index].Bills[index2].Value;
}

//
// Rebel
//
void UCitizenManager::Overthrow()
{
	CooldownTimer = 1500;

	for (ACitizen* citizen : Citizens) {
		if (citizen->Politics.Ideology.Party != EParty::Freedom)
			return;

		SetupRebel(citizen);
	}
}

void UCitizenManager::SetupRebel(class ACitizen* Citizen)
{
	Citizen->Rebel = true;

	Citizen->HatMesh->SetStaticMesh(Citizen->RebelHat);

	Citizen->Capsule->SetCollisionObjectType(ECollisionChannel::ECC_GameTraceChannel3);

	Citizen->AttackComponent->RangeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);

	Citizen->AttackComponent->RangeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECollisionResponse::ECR_Overlap);
	Citizen->AttackComponent->RangeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel3, ECollisionResponse::ECR_Ignore);
	Citizen->AttackComponent->RangeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel4, ECollisionResponse::ECR_Overlap);

	Citizen->Reach->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECollisionResponse::ECR_Overlap);
	Citizen->Reach->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel3, ECollisionResponse::ECR_Ignore);
	Citizen->Reach->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel4, ECollisionResponse::ECR_Overlap);

	Citizen->MoveToBroch();
}

bool UCitizenManager::IsRebellion()
{
	for (ACitizen* citizen : Citizens)
		if (citizen->Rebel)
			return true;

	return false;
}