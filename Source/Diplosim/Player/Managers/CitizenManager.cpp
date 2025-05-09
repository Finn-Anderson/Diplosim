#include "Player/Managers/CitizenManager.h"

#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/WidgetComponent.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"

#include "AI/Citizen.h"
#include "AI/Enemy.h"
#include "AI/AttackComponent.h"
#include "AI/DiplosimAIController.h"
#include "AI/AIMovementComponent.h"
#include "Universal/HealthComponent.h"
#include "Buildings/Work/Service/Clinic.h"
#include "Buildings/Work/Service/Religion.h"
#include "Buildings/Work/Service/School.h"
#include "Buildings/Misc/Broch.h"
#include "Buildings/Misc/Festival.h"
#include "Buildings/House.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Map/Grid.h"
#include "Map/Atmosphere/Clouds.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Universal/DiplosimGameModeBase.h"

UCitizenManager::UCitizenManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetComponentTickInterval(0.2f);

	CooldownTimer = 0;

	BrochLocation = FVector::Zero();

	IssuePensionHour = 18;

	ReadJSONFile(FPaths::ProjectDir() + "/Content/Custom/Structs/Personalities.json");

	ReadJSONFile(FPaths::ProjectDir() + "/Content/Custom/Structs/Parties.json");

	ReadJSONFile(FPaths::ProjectDir() + "/Content/Custom/Structs/Laws.json");

	ReadJSONFile(FPaths::ProjectDir() + "/Content/Custom/Structs/Religions.json");

	ReadJSONFile(FPaths::ProjectDir() + "/Content/Custom/Structs/Conditions.json");
}

void UCitizenManager::ReadJSONFile(FString path)
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
				FConditionStruct condition;

				for (auto& v : e->AsObject()->Values) {
					uint8 index = 0;

					if (v.Value->Type == EJson::Object) {
						for (auto& bv : v.Value->AsObject()->Values) {
							if (bv.Value->Type == EJson::Array) {
								for (auto& bev : bv.Value->AsArray()) {
									if (bv.Key == "Descriptions") {
										FDescriptionStruct description;

										for (auto& pv : bev->AsObject()->Values) {
											if (pv.Key == "Description")
												description.Description = pv.Value->AsString();
											else if (pv.Key == "Min")
												description.Min = FCString::Atoi(*pv.Value->AsString());
											else
												description.Max = FCString::Atoi(*pv.Value->AsString());
										}

										law.Description.Add(description);
									}
									else {
										FLeanStruct lean;

										for (auto& pv : bev->AsObject()->Values) {
											if (pv.Key == "Party")
												lean.Party = EParty(FCString::Atoi(*pv.Value->AsString()));
											else if (pv.Key == "Personality")
												lean.Personality = EPersonality(FCString::Atoi(*pv.Value->AsString()));
											else {
												for (auto& pev : pv.Value->AsArray()) {
													int32 value = FCString::Atoi(*pv.Value->AsString());

													if (pv.Key == "ForRange")
														lean.ForRange.Add(value);
													else
														lean.AgainstRange.Add(value);
												}
											}
										}

										law.Lean.Add(lean);
									}
								}
							}
							else if (bv.Value->Type == EJson::Number) {
								index = FCString::Atoi(*bv.Value->AsString());

								law.Value = index;
							}
							else if (bv.Value->Type == EJson::String) {
								law.Warning = bv.Value->AsString();
							}
						}
					}
					else if (v.Value->Type == EJson::Array) {
						for (auto& ev : v.Value->AsArray()) {
							index = FCString::Atoi(*ev->AsString());

							if (element.Key == "Personalities") {
								if (v.Key == "Likes")
									personality.Likes.Add(EPersonality(index));
								else
									personality.Dislikes.Add(EPersonality(index));
							}
							else if (element.Key == "Parties") {
								party.Agreeable.Add(EPersonality(index));
							}
							else if (element.Key == "Religions") {
								religion.Agreeable.Add(EPersonality(index));
							}
							else {
								FAffectStruct affect;

								for (auto& bv : ev->AsObject()->Values) {
									index = FCString::Atoi(*bv.Value->AsString());

									if (bv.Key == "Affect")
										affect.Affect = EAffect(index);
									else
										affect.Amount = bv.Value->AsNumber();
								}

								condition.Affects.Add(affect);
							}
						}
					}
					else if (v.Value->Type == EJson::String) {
						condition.Name = v.Value->AsString();
					}
					else {
						index = FCString::Atoi(*v.Value->AsString());

						if (element.Key == "Personalities")
							personality.Trait = EPersonality(index);
						else if (element.Key == "Parties")
							party.Party = EParty(index);
						else if (element.Key == "Religions")
							religion.Faith = EReligion(index);
						else if (element.Key == "Laws")
							law.BillType = EBillType(index);
						else {
							if (v.Key == "Grade")
								condition.Grade = EGrade(index);
							else if (v.Key == "Spreadability")
								condition.Spreadability = index;
							else if (v.Key == "Level")
								condition.Level = index;
							else
								condition.DeathLevel = index;
						}
					}
				}

				if (element.Key == "Personalities")
					Personalities.Add(personality);
				else if (element.Key == "Parties")
					Parties.Add(party);
				else if (element.Key == "Religions")
					Religions.Add(religion);
				else if (element.Key == "Laws")
					Laws.Add(law);
				else if (element.Key == "Injuries")
					Injuries.Add(condition);
				else
					Diseases.Add(condition);
			}
		}
	}
}

void UCitizenManager::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (DeltaTime < 0.01f || DeltaTime > 1.0f)
		return;

	Async(EAsyncExecution::Thread, [this, DeltaTime]() {
		Loop(DeltaTime);

		if (Citizens.IsEmpty() && Enemies.IsEmpty())
			return;

		TArray<TEnumAsByte<EObjectTypeQuery>> objects;

		TArray<AActor*> ignore;
		ignore.Add(Cast<ACamera>(GetOwner())->Grid);

		for (FResourceHISMStruct resourceStruct : Cast<ACamera>(GetOwner())->Grid->MineralStruct)
			ignore.Add(resourceStruct.Resource);

		for (FResourceHISMStruct resourceStruct : Cast<ACamera>(GetOwner())->Grid->FlowerStruct)
			ignore.Add(resourceStruct.Resource);

		TArray<AActor*> actors;

		for (ACitizen* citizen : Citizens) {
			if (citizen->HealthComponent->GetHealth() <= 0 || IsValid(citizen->Building.BuildingAt))
				continue;

			int32 reach = citizen->Range / 15.0f;
			UKismetSystemLibrary::SphereOverlapActors(GetWorld(), citizen->GetActorLocation(), citizen->Range, objects, nullptr, ignore, actors);

			for (AActor* actor : actors) {
				if (actor == citizen || (!actor->IsA<AAI>() && !actor->IsA<AResource>() && !actor->IsA<ABuilding>()))
					continue;

				UHealthComponent* healthComp = actor->GetComponentByClass<UHealthComponent>();

				if (healthComp && healthComp->GetHealth() <= 0)
					continue;

				if (actor->IsA<AAI>()) {
					AAI* ai = Cast<AAI>(actor);

					if (ai->Capsule->GetCollisionObjectType() == citizen->Capsule->GetCollisionObjectType()) {
						if (citizen->CanReach(ai, reach) && !Infected.IsEmpty()) {
							ACitizen* c = Cast<ACitizen>(ai);

							if (!IsValid(c->Building.Employment) || !c->Building.Employment->IsA<AClinic>()) {
								for (FConditionStruct condition : citizen->HealthIssues) {
									if (c->HealthIssues.Contains(condition))
										continue;

									int32 chance = FMath::RandRange(1, 100);

									if (chance <= condition.Spreadability)
										c->HealthIssues.Add(condition);
								}

								if (!c->HealthIssues.IsEmpty() && !c->DiseaseNiagaraComponent->IsActive())
									Infect(c);
							}

							if (IsValid(citizen->Building.Employment) && citizen->Building.Employment->IsA<AClinic>() && FindTimer("Healing", citizen) == nullptr) {
								citizen->CitizenHealing = c;

								FTimerStruct timer;
								timer.CreateTimer("Healing", citizen, 2.0f / citizen->GetProductivity(), FTimerDelegate::CreateUObject(citizen, &ACitizen::Heal, c), false);
							}
						}
					}

					if (ai->IsA<AEnemy>() || Cast<ACitizen>(ai)->Rebel != citizen->Rebel) {
						if (!*citizen->AttackComponent->ProjectileClass && !citizen->AIController->CanMoveTo(ai->GetActorLocation()))
							continue;

						if (!citizen->AttackComponent->OverlappingEnemies.Contains(ai))
							citizen->AttackComponent->OverlappingEnemies.Add(ai);

						if (citizen->AttackComponent->OverlappingEnemies.Num() == 1)
							citizen->AttackComponent->SetComponentTickEnabled(true);
					}
				}
				else if (citizen->AIController->MoveRequest.GetGoalActor() == actor && citizen->CanReach(actor, reach)) {
					AsyncTask(ENamedThreads::GameThread, [this, citizen, actor]() {
						if (actor->IsA<AResource>() && FindTimer("Harvest", citizen) == nullptr) {
							AResource* r = Cast<AResource>(actor);

							citizen->StartHarvestTimer(r);
						}
						else if (actor->IsA<ABuilding>()) {
							ABuilding* b = Cast<ABuilding>(actor);

								b->Enter(citizen);
						}

						citizen->AIController->StopMovement();
					});
				}
			}

			// Cleanup
			for (int32 i = citizen->AttackComponent->OverlappingEnemies.Num() - 1; i > -1; i--) {
				AActor* actor = citizen->AttackComponent->OverlappingEnemies[i];

				if (actors.Contains(actor))
					continue;

				citizen->AttackComponent->OverlappingEnemies.RemoveAt(i);
			}
		}

		for (AAI* enemy : Enemies) {
			if (enemy->HealthComponent->GetHealth() <= 0)
				continue;

			int32 reach = enemy->Range / 15.0f;

			UKismetSystemLibrary::SphereOverlapActors(GetWorld(), enemy->GetActorLocation(), enemy->Range, objects, nullptr, ignore, actors);

			for (AActor* actor : actors) {
				UHealthComponent* healthComp = actor->GetComponentByClass<UHealthComponent>();

				if (healthComp && healthComp->GetHealth() <= 0)
					continue;

				if (actor->IsA<AResource>() || (actor->GetClass() == enemy->GetClass() && (enemy->IsA<AEnemy>() || (enemy->IsA<ACitizen>() && Cast<ACitizen>(enemy)->Rebel == Cast<ACitizen>(actor)->Rebel))))
					continue;

				if (!*enemy->AttackComponent->ProjectileClass && !enemy->AIController->CanMoveTo(actor->GetActorLocation()))
					continue;

				enemy->AttackComponent->OverlappingEnemies.Add(actor);

				if (enemy->AttackComponent->OverlappingEnemies.Num() == 1)
					enemy->AttackComponent->SetComponentTickEnabled(true);
			}

			for (int32 i = enemy->AttackComponent->OverlappingEnemies.Num() - 1; i > -1; i--) {
				AActor* actor = enemy->AttackComponent->OverlappingEnemies[i];

				if (actors.Contains(actor))
					continue;

				enemy->AttackComponent->OverlappingEnemies.RemoveAt(i);
			}
		}
	});
}

void UCitizenManager::Loop(float DeltaTime)
{
	for (int32 i = Timers.Num() - 1; i > -1; i--) {
		if (!IsValid(Timers[i].Actor) || (Timers[i].Actor->IsA<ACitizen>() && (!Citizens.Contains(Timers[i].Actor) || Cast<ACitizen>(Timers[i].Actor)->Rebel)) || (Timers[i].Actor->IsA<ABuilding>() && !Buildings.Contains(Timers[i].Actor))) {
			Timers.RemoveAt(i);

			continue;
		}

		if (Timers[i].bPaused)
			continue;

		Timers[i].Timer += DeltaTime;

		if ((Timers[i].ID == "Harvest" || Timers[i].ID == "Internal")) {
			TArray<ACitizen*> citizens;

			if (Timers[i].Actor->IsA<ACitizen>())
				citizens.Add(Cast<ACitizen>(Timers[i].Actor));
			else
				citizens = Cast<AWork>(Timers[i].Actor)->GetCitizensAtBuilding();

			for (ACitizen* citizen : Citizens) {
				if (citizen->HarvestVisualTimer < Timers[i].Timer || citizen->HarvestVisualResource == nullptr)
					continue;

				citizen->HarvestVisualTimer += citizen->HarvestVisualTargetTimer;

				citizen->SetHarvestVisuals(citizen->HarvestVisualResource);
			}
			
		}

		if (Timers[i].Timer >= Timers[i].Target) {
			FTimerDelegate delegate = Timers[i].Delegate;

			if (Timers[i].bOnGameThread)
				AsyncTask(ENamedThreads::GameThread, [this, delegate]() { delegate.ExecuteIfBound(); });
			else
				delegate.ExecuteIfBound();

			if (Timers[i].bRepeat)
				Timers[i].Timer = 0;
			else
				Timers.RemoveAt(i);
		}
	}

	if (!Citizens.IsEmpty()) {
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

		int32 timeToCompleteDay = 360 / (24 * Cast<ACamera>(GetOwner())->Grid->AtmosphereComponent->Speed);

		for (ACitizen* citizen : Citizens) {
			if (citizen->Rebel)
				continue;

			for (FConditionStruct& condition : citizen->HealthIssues) {
				condition.Level++;

				if (condition.Level == condition.DeathLevel)
					citizen->HealthComponent->TakeHealth(100, citizen);
			}
			
			citizen->AllocatedBuildings.Empty(); 
			citizen->AllocatedBuildings = { citizen->Building.School, citizen->Building.Employment, citizen->Building.House };

			if (citizen->CanFindAnything(timeToCompleteDay)) {
				for (ABuilding* building : Buildings) {
					if (!IsValid(building) || !citizen->AIController->CanMoveTo(building->GetActorLocation()) || citizen->AllocatedBuildings.Contains(building))
						continue;

					if (building->IsA<ASchool>())
						citizen->FindEducation(Cast<ASchool>(building), timeToCompleteDay);
					else if (building->IsA<AWork>())
						citizen->FindJob(Cast<AWork>(building), timeToCompleteDay);
					else if (building->IsA<AHouse>())
						citizen->FindHouse(Cast<AHouse>(building), timeToCompleteDay);
				}

				AsyncTask(ENamedThreads::GameThread, [this, citizen, timeToCompleteDay]() { citizen->SetJobHouseEducation(timeToCompleteDay); });
			}

			citizen->SetHappiness();

			if (GetMembersParty(citizen) != nullptr && GetMembersParty(citizen)->Party == EParty::ShellBreakers)
				rebelCount++;
		}

		if (!Citizens.IsEmpty() && (rebelCount / Citizens.Num()) * 100 > 33 && !IsRebellion()) {
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

					ProposeBill(Laws[index]);
				}
			}
		}
	}

	AsyncTask(ENamedThreads::GameThread, [this]() { Cast<ACamera>(GetOwner())->UpdateUI(); });
}

FTimerStruct* UCitizenManager::FindTimer(FString ID, AActor* Actor)
{
	if (!IsValid(Actor))
		return nullptr;
	
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

	timer->Actor = nullptr;
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

void UCitizenManager::UpdateTimerLength(FString ID, AActor* Actor, int32 NewTarget)
{
	FTimerStruct* timer = FindTimer(ID, Actor);

	if (timer == nullptr)
		return;

	timer->Target = NewTarget;
}

//
// House
//
void UCitizenManager::UpdateRent(TSubclassOf<class AHouse> HouseType, int32 NewRent)
{
	for (ABuilding* building : Buildings) {
		if (!building->IsA(HouseType))
			continue;

		Cast<AHouse>(building)->Rent = NewRent;
	}
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
	for (ABuilding* building : Buildings) {
		if (!IsValid(building) || !building->IsA<AWork>())
			continue;

		AWork* work = Cast<AWork>(building);

		for (ACitizen* citizen : work->GetOccupied()) {
			if (citizen->HoursWorked.Contains(Hour))
				citizen->HoursWorked.Remove(Hour);

			if (work->bOpen)
				citizen->HoursWorked.Add(Hour);
		}

		work->CheckWorkStatus(Hour);
	}
}

//
// Sleep
//
void UCitizenManager::CheckSleepStatus(int32 Hour)
{
	for (ACitizen* citizen : Citizens) {
		if (citizen->HoursSleptToday.Contains(Hour))
			citizen->HoursSleptToday.Remove(Hour);

		if (citizen->bSleep)
			citizen->HoursSleptToday.Add(Hour);

		if (citizen->HoursSleptToday.Num() < citizen->IdealHoursSlept && !citizen->bSleep && (!IsValid(citizen->Building.Employment) || !citizen->Building.Employment->bOpen) && IsValid(citizen->Building.House) && citizen->Building.BuildingAt == citizen->Building.House)
			citizen->bSleep = true;
		else if (citizen->bSleep)
			citizen->bSleep = false;
	}
}

//
// Disease
//
void UCitizenManager::StartDiseaseTimer()
{
	int32 timeToCompleteDay = 360 / (24 * Cast<ACamera>(GetOwner())->Grid->AtmosphereComponent->Speed);

	FTimerStruct timer;
	timer.CreateTimer("Disease", GetOwner(), FMath::RandRange(timeToCompleteDay / 2, timeToCompleteDay * 3), FTimerDelegate::CreateUObject(this, &UCitizenManager::SpawnDisease), false);

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

	GetClosestHealer(Citizen);
}

void UCitizenManager::Injure(ACitizen* Citizen, int32 Odds)
{
	int32 index = FMath::RandRange(1, 100);

	if (index < Odds)
		return;

	index = FMath::RandRange(0, Injuries.Num() - 1);
	Citizen->HealthIssues.Add(Injuries[index]);

	for (FAffectStruct affect : Injuries[index].Affects) {
		if (affect.Affect == EAffect::Movement)
			Citizen->ApplyToMultiplier("Speed", affect.Amount);
		else if (affect.Affect == EAffect::Damage)
			Citizen->ApplyToMultiplier("Damage", affect.Amount);
		else
			Citizen->ApplyToMultiplier("Health", affect.Amount);
	}

	Injured.Add(Citizen);

	UpdateHealthText(Citizen);

	GetClosestHealer(Citizen);
}

void UCitizenManager::Cure(ACitizen* Citizen)
{
	for (FConditionStruct condition : Citizen->HealthIssues) {
		for (FAffectStruct affect : condition.Affects) {
			if (affect.Affect == EAffect::Movement)
				Citizen->ApplyToMultiplier("Speed", 1.0f + (1.0f - affect.Amount));
			else if (affect.Affect == EAffect::Damage)
				Citizen->ApplyToMultiplier("Damage", 1.0f + (1.0f - affect.Amount));
			else
				Citizen->ApplyToMultiplier("Health", 1.0f + (1.0f - affect.Amount));
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

void UCitizenManager::GetClosestHealer(class ACitizen* Citizen)
{
	AsyncTask(ENamedThreads::GameThread, [this, Citizen]() {
		TArray<AActor*> clinics;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AClinic::StaticClass(), clinics);

		ACitizen* healer = nullptr;

		for (AActor* actor : clinics) {
			AClinic* clinic = Cast<AClinic>(actor);

			if (!clinic->bOpen) {
				clinic->Open();

				continue;
			}

			for (ACitizen* h : clinic->GetCitizensAtBuilding()) {
				if (!h->AIController->CanMoveTo(Citizen->GetActorLocation()) || h->AIController->MoveRequest.GetGoalActor() != nullptr)
					continue;

				if (healer == nullptr) {
					healer = h;

					continue;
				}

				double magnitude = h->AIController->GetClosestActor(50.0f, Citizen->GetActorLocation(), healer->GetActorLocation(), h->GetActorLocation());

				if (magnitude > 0.0f)
					healer = h;
			}
		}

		if (healer == nullptr)
			return;

		PickCitizenToHeal(healer, Citizen);
	});
}

void UCitizenManager::PickCitizenToHeal(ACitizen* Healer, ACitizen* Citizen)
{
	if (Citizen == nullptr) {
		for (ACitizen* citizen : Infected) {
			if (Healing.Contains(citizen))
				continue;

			if (Citizen == nullptr) {
				Citizen = citizen;

				continue;
			}

			double magnitude = Healer->AIController->GetClosestActor(50.0f, Healer->GetActorLocation(), Citizen->GetActorLocation(), citizen->GetActorLocation());

			if (magnitude > 0.0f)
				Citizen = citizen;
		}
	}

	if (Citizen != nullptr) {
		Healer->AIController->AIMoveTo(Citizen);

		Healing.Add(Citizen);
	}
	else if (Healer->Building.BuildingAt != Healer->Building.Employment) {
		Healer->AIController->DefaultAction();
	}
}

//
// Event
//
void UCitizenManager::CreateEvent(EEventType Type, TSubclassOf<class ABuilding> Building, FString Period, int32 Day, int32 StartHour, int32 EndHour, bool bRecurring, bool bFireFestival)
{
	FEventStruct event;
	event.Type = Type;

	int32 index = Events.Find(event);

	FEventTimeStruct times;
	times.Period = Period;
	times.Day = Day;
	times.StartHour = StartHour;
	times.EndHour = EndHour;
	times.bRecurring = bRecurring;
	times.bFireFestival = bFireFestival;
	
	if (index != INDEX_NONE) {
		Events[index].Times.Add(times);

		return;
	}

	event.Times.Add(times);
	event.Building = Building;

	Events.Add(event);
}

void UCitizenManager::ExecuteEvent(FString Period, int32 Day, int32 Hour)
{
	for (FEventStruct event : Events) {
		FString command = "";
		FEventTimeStruct time;

		for (FEventTimeStruct times : event.Times) {
			if (times.Period != "" && times.Day != 0 && times.Period != Period && times.Day != Day)
				continue;

			if (times.StartHour == Hour && !times.bStarted)
				command = "start";
			else if (times.EndHour == Hour && times.bStarted)
				command = "end";

			if (command != "") {
				time = times;

				break;
			}
		}

		if (command == "")
			continue;

		if (command == "start")
			StartEvent(event, time);
		else
			EndEvent(event, time);
	}
}

bool UCitizenManager::IsAttendingEvent(ACitizen* Citizen)
{
	for (FEventStruct event : OngoingEvents())
		if (event.Attendees.Contains(Citizen))
			return true;

	return false;
}

TArray<FEventStruct> UCitizenManager::OngoingEvents()
{
	TArray<FEventStruct> events;
	
	for (FEventStruct event : Events) {
		UAtmosphereComponent* atmosphere = Cast<ACamera>(GetOwner())->Grid->AtmosphereComponent;

		for (FEventTimeStruct times : event.Times) {
			if (!times.bStarted)
				continue;

			events.Add(event);
		}
	}

	return events;
}

FEventTimeStruct UCitizenManager::GetOngoingEventTimes(FEventStruct event)
{
	UAtmosphereComponent* atmosphere = Cast<ACamera>(GetOwner())->Grid->AtmosphereComponent;

	FEventTimeStruct time;

	for (FEventTimeStruct times : event.Times) {
		if (!times.bStarted)
			continue;

		time = times;

		break;
	}

	return time;
}

void UCitizenManager::GotoEvent(ACitizen* Citizen, FEventStruct Event)
{
	if (IsAttendingEvent(Citizen) || (Event.Type != EEventType::Protest && IsValid(Citizen->Building.Employment) && !Citizen->Building.Employment->bCanAttendEvents && Citizen->Building.Employment->bOpen) || (Event.Type == EEventType::Mass && Cast<ABroadcast>(Event.Building->GetDefaultObject())->Belief != Citizen->Spirituality.Faith && Citizen->BioStruct.Age >= 18))
		return;

	int32 index = Events.Find(Event);

	if (Event.Type == EEventType::Holliday || Event.Type == EEventType::Festival) {
		Citizen->SetHolliday(true);
	}
	else if (Event.Type == EEventType::Protest) {
		ABuilding* building = Buildings[FMath::RandRange(0, Buildings.Num() - 1)];

		UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
		const ANavigationData* navData = nav->GetDefaultNavDataInstance();

		FNavLocation location;
		nav->GetRandomPointInNavigableRadius(building->GetActorLocation(), 400, location);

		double length;
		nav->GetPathLength(Citizen->GetActorLocation(), location, length);

		UNavigationPath* path = nav->FindPathToLocationSynchronously(GetWorld(), Citizen->GetActorLocation(), location, Citizen, Citizen->NavQueryFilter);

		Citizen->MovementComponent->SetPoints(path->PathPoints);

		return;
	}
	else if (Event.Type == EEventType::Mass && Citizen->BioStruct.Age < 18) {
		bool validReligion = false;

		if (Citizen->BioStruct.Mother->IsValidLowLevelFast() && Cast<ABroadcast>(Event.Building->GetDefaultObject())->Belief != Citizen->BioStruct.Mother->Spirituality.Faith || Citizen->BioStruct.Father->IsValidLowLevelFast() && Cast<ABroadcast>(Event.Building->GetDefaultObject())->Belief != Citizen->BioStruct.Father->Spirituality.Faith)
			validReligion = true;

		if (!validReligion)
			return;
	}

	AsyncTask(ENamedThreads::GameThread, [this, Citizen, Event]() {
		ABuilding* chosenBuilding = nullptr;
		ACitizen* chosenOccupant = nullptr;

		TArray<AActor*> actors;

		if (IsValid(Event.Building))
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), Event.Building, actors);

		for (AActor* actor : actors) {
			ABuilding* building = Cast<ABuilding>(actor);
			ACitizen* occupant = nullptr;

			if (!Citizen->AIController->CanMoveTo(building->GetActorLocation()) || (Event.Type == EEventType::Mass && building->GetOccupied().IsEmpty()) || (Event.Type == EEventType::Festival && !Cast<AFestival>(building)->bCanHostFestival))
				continue;

			if (Event.Type == EEventType::Festival || Event.Type == EEventType::Mass) {
				bool bSpace = false;

				for (ACitizen* occpnt : building->GetOccupied()) {
					if (building->GetVisitors(occpnt).Num() == building->Space)
						continue;

					occupant = occpnt;
					bSpace = true;

					break;
				}

				if (!bSpace)
					continue;
			}

			if (chosenBuilding == nullptr) {
				chosenBuilding = building;
				chosenOccupant = occupant;

				continue;
			}

			double magnitude = Citizen->AIController->GetClosestActor(400.0f, Citizen->GetActorLocation(), chosenBuilding->GetActorLocation(), building->GetActorLocation());

			if (magnitude <= 0.0f)
				continue;

			chosenBuilding = building;
			chosenOccupant = occupant;
		}

		if (chosenBuilding != nullptr) {
			int32 index = Events.Find(Event);
			Events[index].Attendees.Add(Citizen);

			chosenBuilding->AddVisitor(chosenOccupant, Citizen);

			Citizen->AIController->AIMoveTo(chosenBuilding);
		}
	});
}

void UCitizenManager::StartEvent(FEventStruct Event, FEventTimeStruct Time)
{
	int32 index = Events.Find(Event);
	int32 i = Events[index].Times.Find(Time);

	Events[index].Times[i].bStarted = true;
	
	if (Event.Type == EEventType::Holliday || Event.Type == EEventType::Festival) {
		for (ABuilding* building : Buildings) {
			if (building->IsA<AWork>())
				Cast<AWork>(building)->CheckWorkStatus(Time.StartHour);
			else if (building->IsA<AFestival>() && Event.Type == EEventType::Festival)
				Cast<AFestival>(building)->StartFestival(Time.bFireFestival);
		}
	}
	
	for (ACitizen* citizen : Citizens)
		GotoEvent(citizen, Event);

	Cast<ACamera>(GetOwner())->DisplayEvent("Event", EnumToString<EEventType>(Event.Type));
}

void UCitizenManager::EndEvent(FEventStruct Event, FEventTimeStruct Time)
{
	int32 index = Events.Find(Event);
	int32 i = Events[index].Times.Find(Time);
	
	Event.Attendees.Empty();
	Events[index].Times[i].bStarted = false;

	if (!Time.bRecurring)
		Events[index].Times.RemoveAt(i);

	if (Event.Type == EEventType::Holliday || Event.Type == EEventType::Festival) {
		for (ABuilding* building : Buildings) {
			if (building->IsA<AWork>())
				Cast<AWork>(building)->CheckWorkStatus(Time.EndHour);
			else if (building->IsA<AFestival>() && Event.Type == EEventType::Festival)
				Cast<AFestival>(building)->StopFestival();
		}
	}

	for (ACitizen* citizen : Citizens) {
		if (Event.Type == EEventType::Holliday || Event.Type == EEventType::Festival) {
			citizen->SetHolliday(false);

			if (Event.Type == EEventType::Festival) {
				if (IsValid(citizen->Building.BuildingAt) && citizen->Building.BuildingAt->IsA(Event.Building))
					citizen->SetAttendStatus(EAttendStatus::Attended, false);
				else
					citizen->SetAttendStatus(EAttendStatus::Missed, false);
			}
		}
		else if (Event.Type == EEventType::Mass) {
			if (Cast<ABroadcast>(Event.Building->GetDefaultObject())->Belief != citizen->Spirituality.Faith)
				continue;

			if (citizen->bWorshipping)
				citizen->SetAttendStatus(EAttendStatus::Attended, true);
			else
				citizen->SetAttendStatus(EAttendStatus::Missed, true);
		}

		if (!IsValid(Event.Building))
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
		if (!IsValid(element.Key) || GetMembersParty(element.Key)->Party != party->Party || element.Key->bHasBeenLeader)
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

	int32 representativeType = GetLawValue(EBillType::RepresentativeType);

	for (FPartyStruct party : Parties) {
		TArray<ACitizen*> citizens;

		for (TPair<ACitizen*, TEnumAsByte<ESway>> pair : party.Members) {
			if ((representativeType == 1 && pair.Key->Building.Employment == nullptr) || (representativeType == 2 && pair.Key->Balance < 15))
				continue;

			citizens.Add(pair.Key);
		}

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

void UCitizenManager::ProposeBill(FLawStruct Bill)
{
	int32 index = Laws.Find(Bill);

	if (Laws[index].Cooldown != 0) {
		FString string = "You must wait " + Laws[index].Cooldown;

		Cast<ACamera>(GetOwner())->ShowWarning(string + " seconds");

		return;
	}

	ProposedBills.Add(Bill);

	int32 timeToCompleteDay = 360 / (24 * Cast<ACamera>(GetOwner())->Grid->AtmosphereComponent->Speed);

	Laws[index].Cooldown = timeToCompleteDay;

	Cast<ACamera>(GetOwner())->DisplayNewBill();

	if (ProposedBills.Num() > 1)
		return;

	SetupBill();
}

void UCitizenManager::SetElectionBillLeans(FLawStruct* Bill)
{
	if (Bill->BillType != EBillType::Election)
		return;

	for (FPartyStruct party : Parties) {
		int32 representativeCount = 0;

		for (ACitizen* citizen : Representatives)
			if (party.Members.Contains(citizen))
				representativeCount++;

		float representPerc = representativeCount / Citizens.Num() * 100.0f;
		float partyPerc = party.Members.Num() / Citizens.Num() * 100.0f;

		FLeanStruct lean;
		lean.Party = party.Party;

		if (partyPerc > representPerc)
			lean.ForRange.Append({ 0, 0 });
		else if (representPerc > partyPerc)
			lean.AgainstRange.Append({ 0, 0 });

		Bill->Lean.Add(lean);
	}
}

void UCitizenManager::SetupBill()
{
	Votes.Clear();

	BribeValue.Empty();

	if (ProposedBills.IsEmpty())
		return;

	SetElectionBillLeans(&ProposedBills[0]);

	for (ACitizen* citizen : Representatives)
		GetVerdict(citizen, ProposedBills[0], true, false);

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

void UCitizenManager::MotionBill(FLawStruct Bill)
{
	AsyncTask(ENamedThreads::GameThread, [this, Bill]() {
		int32 count = 1;

		for (ACitizen* citizen : Representatives) {
			if (Votes.For.Contains(citizen) || Votes.Against.Contains(citizen))
				continue;

			FTimerHandle verdictTimer;
			GetWorld()->GetTimerManager().SetTimer(verdictTimer, FTimerDelegate::CreateUObject(this, &UCitizenManager::GetVerdict, citizen, Bill, false, false), 0.1f * count, false);

			count++;
		}

		FTimerHandle tallyTimer;
		GetWorld()->GetTimerManager().SetTimer(tallyTimer, FTimerDelegate::CreateUObject(this, &UCitizenManager::TallyVotes, Bill), 0.1f * count + 0.1f, false);
	});
}

bool UCitizenManager::IsInRange(TArray<int32> Range, int32 Value)
{
	if (Value == 255 || (Range[0] > Range[1] && Value >= Range[0] || Value <= Range[1]) || (Range[0] <= Range[1] && Value >= Range[0] && Value <= Range[1]))
		return true;

	return false;
}

void UCitizenManager::GetVerdict(ACitizen* Representative, FLawStruct Bill, bool bCanAbstain, bool bPrediction)
{
	TArray<FString> verdict;

	FLeanStruct partyLean;
	partyLean.Party = GetMembersParty(Representative)->Party;

	int32 index = Bill.Lean.Find(partyLean);

	if (index != INDEX_NONE && !Bill.Lean[index].ForRange.IsEmpty() && IsInRange(Bill.Lean[index].ForRange, Bill.Value))
		verdict = { "Agreeing", "Agreeing", "Agreeing", "Agreeing", "Agreeing", "Agreeing", "Agreeing", "Abstaining", "Abstaining", "Opposing" };
	else if (index != INDEX_NONE && !Bill.Lean[index].AgainstRange.IsEmpty() && IsInRange(Bill.Lean[index].AgainstRange, Bill.Value))
		verdict = { "Opposing", "Opposing", "Opposing", "Opposing", "Opposing", "Opposing", "Opposing", "Abstaining", "Abstaining", "Agreeing" };
	else
		verdict = { "Abstaining", "Abstaining", "Abstaining", "Abstaining", "Abstaining", "Abstaining", "Agreeing", "Agreeing", "Opposing", "Opposing" };

	if (!bCanAbstain)
		verdict.Remove("Abstaining");

	for (FPersonality* personality : GetCitizensPersonalities(Representative)) {
		FLeanStruct personalityLean;
		personalityLean.Personality = personality->Trait;

		index = Bill.Lean.Find(personalityLean);

		if (index == INDEX_NONE)
			continue;

		if (!Bill.Lean[index].ForRange.IsEmpty() && IsInRange(Bill.Lean[index].ForRange, Bill.Value))
			verdict.Append({ "Agreeing", "Agreeing", "Agreeing" });
		else if (!Bill.Lean[index].AgainstRange.IsEmpty() && IsInRange(Bill.Lean[index].AgainstRange, Bill.Value))
			verdict.Append({ "Opposing", "Opposing", "Opposing" });
	}

	if (Bill.BillType == EBillType::EducationCost || Bill.BillType == EBillType::FoodCost) {
		int32 leftoverMoney = 0;

		if (IsValid(Representative->Building.Employment))
			leftoverMoney += Representative->Building.Employment->Wage;

		if (IsValid(Representative->Building.House))
			leftoverMoney -= Representative->Building.House->Rent;

		if (leftoverMoney < Bill.Value)
			verdict.Append({ "Opposing", "Opposing", "Opposing" });
	}
	else if (Bill.BillType == EBillType::PensionAge) {
		index = Laws.Find(Bill);

		if (Laws[index].Value <= Representative->BioStruct.Age) {
			if (Bill.Value > Representative->BioStruct.Age)
				verdict.Append({ "Opposing", "Opposing", "Opposing" });
			else
				verdict.Append({ "Abstaining", "Abstaining", "Abstaining" });
		}
		else if (Bill.Value <= Representative->BioStruct.Age)
			verdict.Append({ "Agreeing", "Agreeing", "Agreeing" });
	}
	else if (Bill.BillType == EBillType::RepresentativeType) {
		if (Bill.Value == 1 && !IsValid(Representative->Building.Employment))
			verdict.Append({ "Opposing", "Opposing", "Opposing" });
		else if (Bill.Value = 2 && Representative->Balance < 15)
			verdict.Append({ "Opposing", "Opposing", "Opposing" });
	}

	auto value = Async(EAsyncExecution::TaskGraph, [verdict]() { return FMath::RandRange(0, verdict.Num() - 1); });

	FString result = verdict[value.Get()];

	if (bPrediction) {
		if (result == "Agreeing")
			Predictions.For.Add(Representative);
		else if (result == "Opposing")
			Predictions.Against.Add(Representative);
	}
	else {
		if (result == "Agreeing")
			Votes.For.Add(Representative);
		else if (result == "Opposing")
			Votes.Against.Add(Representative);
	}
}

void UCitizenManager::TallyVotes(FLawStruct Bill)
{
	bool bPassed = false;

	if (Votes.For.Num() > Votes.Against.Num()) {
		int32 index = Laws.Find(Bill);

		if (Laws[index].BillType == EBillType::Abolish) {
			ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());

			FTimerStruct timer;

			timer.CreateTimer("Abolish", GetOwner(), 6, FTimerDelegate::CreateUObject(gamemode->Broch->HealthComponent, &UHealthComponent::TakeHealth, 1000, GetOwner()), false);
			Timers.Add(timer);
		}
		else if (Laws[index].BillType == EBillType::Election) {
			Election();
		}
		else {
			Laws[index].Value = Bill.Value;

			if (Laws[index].BillType == EBillType::WorkAge || Laws[index].BillType == EBillType::EducationAge) {
				for (ACitizen* citizen : Citizens) {
					if (IsValid(citizen->Building.Employment) && !citizen->CanWork(citizen->Building.Employment))
						citizen->Building.Employment->RemoveCitizen(citizen);

					if (IsValid(citizen->Building.School) && (citizen->BioStruct.Age >= GetLawValue(EBillType::WorkAge) || citizen->BioStruct.Age < GetLawValue(EBillType::EducationAge)))
						citizen->Building.School->RemoveVisitor(citizen->Building.School->GetOccupant(citizen), citizen);
				}
			}
			else if (Laws[index].BillType == EBillType::VoteAge) {
				for (ACitizen* citizen : Citizens) {
					if (citizen->BioStruct.Age >= Laws[index].Value)
						continue;

					FPartyStruct* party = GetMembersParty(citizen);

					if (party == nullptr)
						continue;

					party->Members.Remove(citizen);
				}
			}
		}

		if (Laws[index].BillType == EBillType::Representatives && Cast<ACamera>(GetOwner())->ParliamentUIInstance->IsInViewport())
			Cast<ACamera>(GetOwner())->RefreshRepresentatives();

		bPassed = true;
	}

	Cast<ACamera>(GetOwner())->LawPassed(bPassed, Votes.For.Num(), Votes.Against.Num());

	Cast<ACamera>(GetOwner())->LawPassedUIInstance->AddToViewport();

	ProposedBills.Remove(Bill);

	SetupBill();
}

FString UCitizenManager::GetBillPassChance(FLawStruct Bill)
{
	Predictions.Clear();

	TArray<int32> results = { 0, 0, 0 };
	
	SetElectionBillLeans(&Bill);

	for (int32 i = 0; i < 10; i++) {
		for (ACitizen* citizen : Representatives)
			GetVerdict(citizen, Bill, true, true);

		int32 abstainers = (Representatives.Num() - Predictions.For.Num() - Predictions.Against.Num());

		if (Predictions.For.Num() > Predictions.Against.Num() + abstainers)
			results[0]++;
		else if (Predictions.Against.Num() > Predictions.For.Num() + abstainers)
			results[1]++;
		else
			results[2]++;

		Predictions.Clear();
	}

	if (results[0] > results[1] + results[2])
		return "High";
	else if (results[1] > results[0] + results[2])
		return "Low";
	else
		return "Random";
}

int32 UCitizenManager::GetLawValue(EBillType BillType)
{
	FLawStruct lawStruct;
	lawStruct.BillType = BillType;

	int32 index = Laws.Find(lawStruct);
	
	return Laws[index].Value;
}

int32 UCitizenManager::GetCooldownTimer(FLawStruct Law)
{
	int32 index = Laws.Find(Law);

	if (index == INDEX_NONE)
		return 0;

	return Laws[index].Cooldown;
}

void UCitizenManager::IssuePensions(int32 Hour)
{
	if (Hour != IssuePensionHour)
		return;

	for (ACitizen* citizen : Citizens)
		if (citizen->BioStruct.Age >= GetLawValue(EBillType::PensionAge))
			citizen->Balance += GetLawValue(EBillType::Pension);
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
	Citizens.Remove(Citizen);
	Enemies.Add(Citizen);

	Citizen->Rebel = true;

	Citizen->HatMesh->SetStaticMesh(Citizen->RebelHat);

	Citizen->Capsule->SetCollisionObjectType(ECollisionChannel::ECC_GameTraceChannel3);

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