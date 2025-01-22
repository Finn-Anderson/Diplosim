#include "Player/Managers/CitizenManager.h"

#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
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
#include "Buildings/Misc/Broch.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Universal/DiplosimGameModeBase.h"

UCitizenManager::UCitizenManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	CooldownTimer = 0;

	BrochLocation = FVector::Zero();

	FoodCost = 0;

	ReadJSONFile(FPaths::ProjectDir() + "/Content/Custom/Structs/Personalities.json", "Personalities");

	ReadJSONFile(FPaths::ProjectDir() + "/Content/Custom/Structs/Parties.json", "Parties");

	ReadJSONFile(FPaths::ProjectDir() + "/Content/Custom/Structs/Laws.json", "Laws");

	ReadJSONFile(FPaths::ProjectDir() + "/Content/Custom/Structs/Religions.json", "Religions");
}

void UCitizenManager::BeginPlay()
{
	Super::BeginPlay();

	StartDiseaseTimer();
}

void UCitizenManager::ReadJSONFile(FString path, FString Section)
{
	TSharedPtr<FJsonObject> jsonObject = MakeShareable(new FJsonObject());

	FString fileContents;
	FFileHelper::LoadFileToString(fileContents, *path);
	TSharedRef<TJsonReader<>> jsonReader = TJsonReaderFactory<>::Create(fileContents);

	if (FJsonSerializer::Deserialize(jsonReader, jsonObject) && jsonObject.IsValid()) {
		for (auto& element : jsonObject->Values) {
			for (auto& e : element.Value->AsArray()) {
				FPersonality personality;
				FPartyStruct party;
				FLawStruct law;
				FReligionStruct religion;

				for (auto& v : e->AsObject()->Values) {
					uint8 index = 0;

					if (v.Value->Type == EJson::Array) {
						for (auto& ev : v.Value->AsArray()) {
							index = FCString::Atoi(*ev->AsString());

							if (Section == "Personalities") {
								if (v.Key == "Likes")
									personality.Likes.Add(EPersonality(index));
								else
									personality.Dislikes.Add(EPersonality(index));
							}
							else if (Section == "Parties") {
								party.Agreeable.Add(EPersonality(index));
							}
							else if (Section == "Religions") {
								religion.Agreeable.Add(EPersonality(index));
							}
							else {
								FBillStruct bill;

								for (auto& bv : ev->AsObject()->Values) {
									if (bv.Value->Type == EJson::Array) {
										for (auto& bev : bv.Value->AsArray()) {
											index = FCString::Atoi(*bev->AsString());

											if (bv.Key == "Agreeing")
												bill.Agreeing.Add(EParty(index));
											else if (bv.Key == "Opposing")
												bill.Opposing.Add(EParty(index));
											else if (bv.Key == "For")
												bill.For.Add(EPersonality(index));
											else
												bill.Against.Add(EPersonality(index));
										}
									}
									else if (bv.Value->Type == EJson::Number) {
										index = FCString::Atoi(*bv.Value->AsString());

										bill.Value = index;
									}
									else if (bv.Value->Type == EJson::String) {
										if (bv.Key == "Description")
											bill.Description = bv.Value->AsString();
										else
											bill.Warning = bv.Value->AsString();
									}
									else {
										bill.bIsLaw = bv.Value->AsBool();
									}
								}

								law.Bills.Add(bill);
							}
						}
					}
					else {
						index = FCString::Atoi(*v.Value->AsString());

						if (Section == "Personalities")
							personality.Trait = EPersonality(index);
						else if (Section == "Parties")
							party.Party = EParty(index);
						else if (Section == "Religions")
							religion.Faith = EReligion(index);
						else
							law.BillType = EBillType(index);
					}
				}

				if (Section == "Personalities")
					Personalities.Add(personality);
				else if (Section == "Parties")
					Parties.Add(party);
				else if (Section == "Religions")
					Religions.Add(religion);
				else
					Laws.Add(law);
			}
		}
	}
}

void UCitizenManager::Loop()
{
	for (int32 i = Timers.Num() - 1; i > -1; i--) {
		if ((!Citizens.Contains(Timers[i].Actor) && !Buildings.Contains(Timers[i].Actor) && Timers[i].Actor != GetOwner()) || (Timers[i].Actor->IsA<ACitizen>() && Cast<ACitizen>(Timers[i].Actor)->Rebel)) {
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

	for (FPartyStruct& party : Parties) {
		if (party.Leader != nullptr)
			continue;

		SelectNewLeader(party.Party);
	}

	for (FLawStruct& law : Laws) {
		if (law.Cooldown == 0)
			continue;

		law.Cooldown--;
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

		if (GetMembersParty(citizen) != nullptr && GetMembersParty(citizen)->Party == EParty::ShellBreakers)
			rebelCount++;
	}

	if (Citizens.Num() > 0 && (rebelCount / Citizens.Num()) * 100 > 33 && !IsRebellion()) {
		CooldownTimer--;

		if (CooldownTimer < 1) {
			auto value = Async(EAsyncExecution::TaskGraphMainThread, [this]() { return FMath::RandRange(1, 3); });

			if (value.Get() == 3) {
				Overthrow();
			}
			else {
				FLawStruct lawStruct;
				lawStruct.BillType = EBillType::Abolish;

				int32 index = Laws.Find(lawStruct);

				ProposeBill(Laws[index].Bills[0]);
			}
		}
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

FTimerStruct* UCitizenManager::FindTimer(FString ID, AActor* Actor)
{
	FTimerStruct timer;
	timer.ID = ID;
	timer.Actor = Actor;

	int32 index = Timers.Find(timer);

	if (index == INDEX_NONE)
		return nullptr;

	return &Timers[index];
}

void UCitizenManager::RemoveTimer(FString ID, AActor* Actor)
{
	FTimerStruct* timer = FindTimer(ID, Actor);

	if (timer == nullptr)
		return;

	Timers.RemoveSingle(*timer);
}

void UCitizenManager::ResetTimer(FString ID, AActor* Actor)
{
	FTimerStruct* timer = FindTimer(ID, Actor);

	if (timer == nullptr)
		return;

	timer->Timer = 0;
}

int32 UCitizenManager::GetElapsedTime(FString ID, AActor* Actor)
{
	FTimerStruct* timer = FindTimer(ID, Actor);

	if (timer == nullptr)
		return 0;

	return timer->Target - timer->Timer;
}

//
// Death
//
void UCitizenManager::ClearCitizen(ACitizen* Citizen)
{
	for (FPartyStruct party : Parties) {
		if (!party.Members.Contains(Citizen))
			continue;

		if (party.Leader == Citizen)
			SelectNewLeader(party.Party);

		party.Members.Remove(Citizen);

		break;
	}

	for (FPersonality personality : Personalities)
		if (personality.Citizens.Contains(Citizen))
			personality.Citizens.Remove(Citizen);

	Citizens.Remove(Citizen);
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
	timer.CreateTimer("Disease", GetOwner(), FMath::RandRange(180, 1200), FTimerDelegate::CreateUObject(this, &UCitizenManager::SpawnDisease), false);

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

void UCitizenManager::Injure(ACitizen* Citizen, int32 Odds)
{
	int32 index = FMath::RandRange(1, 100);

	if (index < Odds)
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
		timer.CreateTimer("Mass", citizen, timeToCompleteDay * 2, FTimerDelegate::CreateUObject(citizen, &ACitizen::SetMassStatus, EMassStatus::Neutral), false);

		Timers.Add(timer);

		citizen->AIController->DefaultAction();
	}
}

//
// Politics
//
FPartyStruct* UCitizenManager::GetMembersParty(ACitizen* Citizen)
{
	FPartyStruct* partyStruct = nullptr;

	for (FPartyStruct &party : Parties) {
		TEnumAsByte<ESway>* sway = party.Members.Find(Citizen);

		if (sway == nullptr)
			continue;

		partyStruct = &party;

		break;
	}

	return partyStruct;
}

EParty UCitizenManager::GetCitizenParty(ACitizen* Citizen)
{
	FPartyStruct* partyStruct = GetMembersParty(Citizen);

	if (partyStruct == nullptr)
		return EParty::Undecided;

	return partyStruct->Party;
}

void UCitizenManager::SelectNewLeader(EParty Party)
{
	TArray<ACitizen*> candidates;

	FPartyStruct partyStruct;
	partyStruct.Party = Party;

	int32 index = Parties.Find(partyStruct);

	FPartyStruct* party = &Parties[index];

	for (auto &element : party->Members) {
		if (GetMembersParty(element.Key)->Party != party->Party || element.Key->bHasBeenLeader)
			continue;

		if (candidates.Num() < 3)
			candidates.Add(element.Key);
		else {
			ACitizen* lowest = nullptr;

			for (ACitizen* candidate : candidates)
				if (lowest == nullptr || party->Members.Find(lowest) > party->Members.Find(candidate))
					lowest = candidate;

			if (party->Members.Find(element.Key) > party->Members.Find(lowest)) {
				candidates.Remove(lowest);

				candidates.Add(element.Key);
			}
		}
	}

	if (candidates.IsEmpty())
		return;

	auto value = Async(EAsyncExecution::TaskGraph, [candidates]() { return FMath::RandRange(0, candidates.Num() - 1); });

	ACitizen* chosen = candidates[value.Get()];

	party->Leader = chosen;

	chosen->bHasBeenLeader = true;
	party->Members.Emplace(chosen, ESway::Radical);
}

void UCitizenManager::StartElectionTimer()
{
	RemoveTimer("Election", GetOwner());
	
	int32 timeToCompleteDay = 360 / (24 * Cast<ACamera>(GetOwner())->Grid->AtmosphereComponent->Speed);

	FTimerStruct timer;
	timer.CreateTimer("Election", GetOwner(), timeToCompleteDay * GetLawValue(EBillType::ElectionTimer), FTimerDelegate::CreateUObject(this, &UCitizenManager::Election), false);

	Timers.Add(timer);
}

void UCitizenManager::Election()
{
	Representatives.Empty();

	TMap<EParty, TArray<ACitizen*>> tally;

	for (FPartyStruct party : Parties) {
		TArray<ACitizen*> citizens;

		for (TPair<ACitizen*, TEnumAsByte<ESway>> pair : party.Members)
			citizens.Add(pair.Key);

		tally.Add(party.Party, citizens);
	}

	for (TPair<EParty, TArray<ACitizen*>>& pair : tally) {
		int32 number = FMath::RoundHalfFromZero(pair.Value.Num() / (float)Citizens.Num() * 100.0f / GetLawValue(EBillType::Representatives));

		if (number == 0 || Representatives.Num() == GetLawValue(EBillType::Representatives))
			continue;

		number -= 1;

		FPartyStruct partyStruct;
		partyStruct.Party = pair.Key;

		int32 index = Parties.Find(partyStruct);

		FPartyStruct* party = &Parties[index];

		Representatives.Add(party->Leader);

		pair.Value.Remove(party->Leader);

		for (int32 i = 0; i < number; i++) {
			if (pair.Value.IsEmpty())
				continue;

			auto value = Async(EAsyncExecution::TaskGraph, [pair]() { return FMath::RandRange(0, pair.Value.Num() - 1); });

			ACitizen* citizen = pair.Value[value.Get()];

			Representatives.Add(citizen);

			pair.Value.Remove(citizen);

			if (Representatives.Num() == GetLawValue(EBillType::Representatives))
				break;
		}
	}

	StartElectionTimer();
}

void UCitizenManager::Bribe(class ACitizen* Representative, bool bAgree)
{
	if (BribeValue.IsEmpty())
		return;

	int32 index = Representatives.Find(Representative);

	int32 bribe = BribeValue[index];

	bool bPass = Cast<ACamera>(GetOwner())->ResourceManager->TakeUniversalResource(Money, bribe, 0);

	if (!bPass) {
		Cast<ACamera>(GetOwner())->ShowWarning("Cannot afford");

		return;
	}

	if (Votes.For.Contains(Representative))
		Votes.For.Remove(Representative);
	else if (Votes.Against.Contains(Representative))
		Votes.For.Remove(Representative);

	Representative->Balance += bribe;

	if (bAgree)
		Votes.For.Add(Representative);
	else
		Votes.Against.Add(Representative);
}

void UCitizenManager::ProposeBill(FBillStruct Bill)
{
	for (FLawStruct& law : Laws) {
		if (!law.Bills.Contains(Bill))
			continue;

		if (law.Cooldown == 0)
			break;

		FString string = "You must wait " + law.Cooldown;

		Cast<ACamera>(GetOwner())->ShowWarning(string + " seconds");

		return;
	}

	ProposedBills.Add(Bill);

	for (FLawStruct& law : Laws) {
		if (!law.Bills.Contains(Bill))
			continue;

		int32 timeToCompleteDay = 360 / (24 * Cast<ACamera>(GetOwner())->Grid->AtmosphereComponent->Speed);

		law.Cooldown = timeToCompleteDay;

		break;
	}

	if (ProposedBills.Num() > 1)
		return;

	SetupBill();
}

void UCitizenManager::SetupBill()
{
	Votes.Clear();

	BribeValue.Empty();

	Cast<ACamera>(GetOwner())->DisplayNewBill();

	if (ProposedBills.IsEmpty())
		return;

	if (GetBillType(ProposedBills[0]) == EBillType::Election) {
		for (FPartyStruct party : Parties) {
			int32 representativeCount = 0;

			for (ACitizen* citizen : Representatives)
				if (party.Members.Contains(citizen))
					representativeCount++;

			float representPerc = representativeCount / Citizens.Num() * 100.0f;
			float partyPerc = party.Members.Num() / Citizens.Num() * 100.0f;

			if (partyPerc > representPerc)
				ProposedBills[0].Agreeing.Add(party.Party);
			else if (representPerc > partyPerc)
				ProposedBills[0].Opposing.Add(party.Party);
		}
	}

	for (ACitizen* citizen : Representatives)
		GetInitialVotes(citizen, ProposedBills[0]);

	for (ACitizen* citizen : Representatives) {
		int32 bribe = Async(EAsyncExecution::TaskGraph, [this]() { return FMath::RandRange(2, 20); }).Get();

		if (Votes.For.Contains(citizen) || Votes.Against.Contains(citizen))
			bribe *= 4;

		bribe *= (uint8)*GetMembersParty(citizen)->Members.Find(citizen);

		BribeValue.Add(bribe);
	}

	FTimerStruct timer;

	timer.CreateTimer("Bill", GetOwner(), 60, FTimerDelegate::CreateUObject(this, &UCitizenManager::MotionBill, ProposedBills[0]), false);
	Timers.Add(timer);
}

void UCitizenManager::MotionBill(FBillStruct Bill)
{
	AsyncTask(ENamedThreads::GameThread, [this, Bill]() {
		int32 count = 1;

		for (ACitizen* citizen : Representatives) {
			if (Votes.For.Contains(citizen) || Votes.Against.Contains(citizen))
				continue;

			FTimerHandle VerdictTimer;
			GetWorld()->GetTimerManager().SetTimer(VerdictTimer, FTimerDelegate::CreateUObject(this, &UCitizenManager::GetVerdict, citizen, Bill), 0.1f * count, false);

			count++;
		}

		FTimerHandle VerdictTimer;
		GetWorld()->GetTimerManager().SetTimer(VerdictTimer, FTimerDelegate::CreateUObject(this, &UCitizenManager::TallyVotes, Bill), 0.1f * count + 0.1f, false);
	});
}

void UCitizenManager::GetInitialVotes(ACitizen* Representative, FBillStruct Bill)
{
	TArray<FString> verdict;

	if (Bill.Agreeing.Contains(GetMembersParty(Representative)->Party))
		verdict = { "Agreeing", "Agreeing", "Agreeing", "Agreeing", "Agreeing", "Agreeing", "Agreeing", "Abstaining", "Abstaining", "Opposing" };
	else if (Bill.Opposing.Contains(GetMembersParty(Representative)->Party))
		verdict = { "Opposing", "Opposing", "Opposing", "Opposing", "Opposing", "Opposing", "Opposing", "Abstaining", "Abstaining", "Agreeing" };
	else
		verdict = { "Abstaining", "Abstaining", "Abstaining", "Abstaining", "Abstaining", "Abstaining", "Agreeing", "Agreeing", "Opposing", "Opposing" };

	for (FPersonality* personality : GetCitizensPersonalities(Representative)) {
		if (Bill.For.Contains(personality->Trait))
			verdict.Append({ "Agreeing", "Agreeing", "Agreeing" });
		else if (Bill.Against.Contains(personality->Trait))
			verdict.Append({ "Opposing", "Opposing", "Opposing" });
	}

	auto value = Async(EAsyncExecution::TaskGraph, [verdict]() { return FMath::RandRange(0, verdict.Num() - 1); });

	FString result = verdict[value.Get()];

	if (result == "Agreeing")
		Votes.For.Add(Representative);
	else if (result == "Opposing")
		Votes.Against.Add(Representative);
}

void UCitizenManager::GetVerdict(ACitizen* Representative, FBillStruct Bill)
{
	TArray<FString> verdict;

	if (Bill.Agreeing.Contains(GetMembersParty(Representative)->Party))
		verdict = { "Agreeing", "Agreeing", "Agreeing", "Agreeing", "Agreeing", "Agreeing", "Agreeing", "Opposing" };
	else if (Bill.Opposing.Contains(GetMembersParty(Representative)->Party))
		verdict = { "Opposing", "Opposing", "Opposing", "Opposing", "Opposing", "Opposing", "Opposing", "Agreeing" };
	else
		verdict = { "Agreeing", "Agreeing", "Opposing", "Opposing" };

	auto value = Async(EAsyncExecution::TaskGraph, [verdict]() { return FMath::RandRange(0, verdict.Num() - 1); });

	FString result = verdict[value.Get()];

	if (result == "Agreeing")
		Votes.For.Add(Representative);
	else if (result == "Opposing")
		Votes.Against.Add(Representative);
}

void UCitizenManager::TallyVotes(FBillStruct Bill)
{
	bool bPassed = false;

	if (Votes.For.Num() > Votes.Against.Num()) {
		for (FLawStruct &law : Laws) {
			if (!law.Bills.Contains(Bill))
				continue;

			if (law.BillType == EBillType::Abolish) {
				ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());

				FTimerStruct timer;

				timer.CreateTimer("Abolish", GetOwner(), 6, FTimerDelegate::CreateUObject(gamemode->Broch->HealthComponent, &UHealthComponent::TakeHealth, 1000, GetOwner()), false);
				Timers.Add(timer);
			}
			else if (law.BillType == EBillType::Election) {
				Election();
			}
			else {
				FBillStruct currentLaw;
				currentLaw.bIsLaw = true;

				int32 index = law.Bills.Find(currentLaw);
				law.Bills[index].bIsLaw = false;

				index = law.Bills.Find(Bill);
				law.Bills[index].bIsLaw = true;
			}

			if (law.BillType == EBillType::Representatives && Cast<ACamera>(GetOwner())->ParliamentUIInstance->IsInViewport())
				Cast<ACamera>(GetOwner())->RefreshRepresentatives();

			break;
		}

		bPassed = true;
	}

	Cast<ACamera>(GetOwner())->LawPassed(bPassed, Votes.For.Num(), Votes.Against.Num());

	Cast<ACamera>(GetOwner())->LawPassedUIInstance->AddToViewport();

	ProposedBills.Remove(Bill);

	SetupBill();
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

EBillType UCitizenManager::GetBillType(FBillStruct Bill)
{
	for (FLawStruct law : Laws)
		if (law.Bills.Contains(Bill))
			return law.BillType;

	return EBillType::Abolish;
}

int32 UCitizenManager::GetCooldownTimer(FLawStruct Law)
{
	int32 index = Laws.Find(Law);

	if (index == INDEX_NONE)
		return 0;

	return Laws[index].Cooldown;
}

//
// Rebel
//
void UCitizenManager::Overthrow()
{
	CooldownTimer = 1500;

	for (ACitizen* citizen : Citizens) {
		if (GetMembersParty(citizen) == nullptr || GetMembersParty(citizen)->Party != EParty::ShellBreakers)
			return;

		if (Representatives.Contains(citizen))
			Representatives.Remove(citizen);

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

//
// Genetics
//
void UCitizenManager::Pray()
{
	bool bPass = Cast<ACamera>(GetOwner())->ResourceManager->TakeUniversalResource(Money, GetPrayCost(), 0);

	if (!bPass) {
		Cast<ACamera>(GetOwner())->ShowWarning("Cannot afford");

		return;
	}

	IncrementPray("Good", 1);

	int32 timeToCompleteDay = 360 / (24 * Cast<ACamera>(GetOwner())->Grid->AtmosphereComponent->Speed);

	FTimerStruct timer;
	timer.CreateTimer("Pray", GetOwner(), timeToCompleteDay, FTimerDelegate::CreateUObject(this, &UCitizenManager::IncrementPray, FString("Good"), -1), false);

	Timers.Add(timer);
}

void UCitizenManager::IncrementPray(FString Type, int32 Increment)
{
	if (Type == "Good")
		PrayStruct.Good = FMath::Max(PrayStruct.Good + Increment, 0);
	else
		PrayStruct.Bad = FMath::Max(PrayStruct.Bad + Increment, 0);
}

int32 UCitizenManager::GetPrayCost()
{
	int32 cost = 50;

	for (int32 i = 0; i < PrayStruct.Good; i++)
		cost *= 1.15;

	return cost;
}

void UCitizenManager::Sacrifice()
{
	if (Citizens.IsEmpty()) {
		Cast<ACamera>(GetOwner())->ShowWarning("Cannot afford");

		return;
	}

	int32 index = FMath::RandRange(0, Citizens.Num() - 1);
	ACitizen* citizen = Citizens[index];

	citizen->AIController->StopMovement();
	citizen->MovementComponent->SetMaxSpeed(0.0f);

	UNiagaraComponent* component = UNiagaraFunctionLibrary::SpawnSystemAttached(SacrificeSystem, citizen->GetRootComponent(), NAME_None, FVector::Zero(), FRotator(0.0f), EAttachLocation::SnapToTarget, true);

	FTimerStruct timer;
	timer.CreateTimer("Sacrifice", GetOwner(), 4.0f, FTimerDelegate::CreateUObject(citizen->HealthComponent, &UHealthComponent::TakeHealth, 1000, GetOwner()), false);

	Timers.Add(timer);

	IncrementPray("Bad", 1);

	int32 timeToCompleteDay = 360 / (24 * Cast<ACamera>(GetOwner())->Grid->AtmosphereComponent->Speed);

	timer.CreateTimer("Pray", GetOwner(), timeToCompleteDay, FTimerDelegate::CreateUObject(this, &UCitizenManager::IncrementPray, FString("Bad"), -1), false);

	Timers.Add(timer);
}

//
// Personality
//
TArray<FPersonality*> UCitizenManager::GetCitizensPersonalities(class ACitizen* Citizen)
{
	TArray<FPersonality*> personalities;

	for (FPersonality& personality : Personalities) {
		if (!personality.Citizens.Contains(Citizen))
			continue;

		personalities.Add(&personality);
	}

	return personalities;
}