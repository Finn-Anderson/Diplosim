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
#include "Misc/ScopeTryLock.h"

#include "AI/Citizen.h"
#include "AI/Enemy.h"
#include "AI/AttackComponent.h"
#include "AI/DiplosimAIController.h"
#include "AI/AIMovementComponent.h"
#include "Universal/HealthComponent.h"
#include "Buildings/Misc/Special/CloneLab.h"
#include "Buildings/Work/Defence/Wall.h"
#include "Buildings/Work/Defence/Trap.h"
#include "Buildings/Work/Service/Clinic.h"
#include "Buildings/Work/Service/School.h"
#include "Buildings/Work/Service/Orphanage.h"
#include "Buildings/Work/Booster.h"
#include "Buildings/Misc/Broch.h"
#include "Buildings/Misc/Festival.h"
#include "Buildings/House.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Components/SaveGameComponent.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Map/Atmosphere/Clouds.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Map/Atmosphere/NaturalDisasterComponent.h"
#include "Universal/DiplosimGameModeBase.h"

UCitizenManager::UCitizenManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetComponentTickInterval(0.2f);

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
												lean.Party = pv.Value->AsString();
											else if (pv.Key == "Personality")
												lean.Personality = pv.Value->AsString();
											else {
												for (auto& pev : pv.Value->AsArray()) {
													int32 value = FCString::Atoi(*pev->AsString());

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
							if (element.Key == "Personalities") {
								if (v.Key == "Likes")
									personality.Likes.Add(ev->AsString());
								else if (v.Key == "Dislikes")
									personality.Dislikes.Add(ev->AsString());
								else {
									FString key = "";
									float value = 1.0f;

									for (auto& bv : ev->AsObject()->Values) {
										if (bv.Key == "Affect")
											key = bv.Value->AsString();
										else
											value = bv.Value->AsNumber();
									}

									personality.Affects.Add(key, value);
								}
							}
							else if (element.Key == "Parties") {
								party.Agreeable.Add(ev->AsString());
							}
							else if (element.Key == "Religions") {
								religion.Agreeable.Add(ev->AsString());
							}
							else {
								FAffectStruct affect;

								for (auto& bv : ev->AsObject()->Values) {
									if (bv.Key == "Affect")
										affect.Affect = EAffect(FCString::Atoi(*bv.Value->AsString()));
									else
										affect.Amount = bv.Value->AsNumber();
								}

								condition.Affects.Add(affect);
							}
						}
					}
					else if (v.Value->Type == EJson::String) {
						FString value = v.Value->AsString();

						if (element.Key == "Personalities")
							personality.Trait = value;
						else if (element.Key == "Parties")
							party.Party = value;
						else if (element.Key == "Religions")
							religion.Faith = value;
						else if (element.Key == "Laws")
							law.BillType = value;
						else
							condition.Name = value;
					}
					else {
						index = FCString::Atoi(*v.Value->AsString());

						if (v.Key == "Aggressiveness")
							personality.Aggressiveness = v.Value->AsNumber();
						else if (v.Key == "Grade")
							condition.Grade = EGrade(index);
						else if (v.Key == "Spreadability")
							condition.Spreadability = index;
						else if (v.Key == "Level")
							condition.Level = index;
						else
							condition.DeathLevel = index;
					}
				}

				if (element.Key == "Personalities")
					Personalities.Add(personality);
				else if (element.Key == "Parties")
					InitParties.Add(party);
				else if (element.Key == "Religions")
					Religions.Add(religion);
				else if (element.Key == "Laws")
					InitLaws.Add(law);
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

	if (DeltaTime < 0.005f || DeltaTime > 1.0f)
		return;

	TimerLoop();

	CitizenGeneralLoop();
	CalculateDisease();
	CalculateCitizenInteractions();
	
	CalculateFighting();
}

void UCitizenManager::TimerLoop()
{
	if (Timers.IsEmpty())
		return;

	Async(EAsyncExecution::TaskGraph, [this]() {
		FScopeTryLock loopLock(&TimerLoopLock);
		if (!loopLock.IsLocked())
			return;

		for (auto node = Timers.GetHead(); node != nullptr; node = node->GetNextNode()) {
			FTimerStruct& timer = node->GetValue();

			if (!IsValid(this))
				return;

			if (!IsValid(timer.Actor)) {
				FScopeLock lock(&TimerLock);
				Timers.RemoveNode(node);

				continue;
			}

			if (timer.bPaused)
				continue;

			double currentTime = GetWorld()->GetTimeSeconds();

			timer.Timer += (currentTime - timer.LastUpdateTime);
			timer.LastUpdateTime = currentTime;

			if (timer.Timer >= timer.Target && !timer.bModifying) {
				if (timer.bOnGameThread) {
					timer.bModifying = true;

					Async(EAsyncExecution::TaskGraphMainTick, [this, &timer]() { CallTimerFunction(&timer); });
				}
				else {
					timer.bModifying = true;

					CallTimerFunction(&timer);
				}
			}

			if (!timer.bDone)
				continue;

			if (timer.bRepeat) {
				timer.Timer = 0;
				timer.bModifying = false;
				timer.bDone = false;
			}
			else {
				FScopeLock lock(&TimerLock);
				Timers.RemoveNode(node);
			} 
		}
	});
}

void UCitizenManager::CitizenGeneralLoop()
{
	Async(EAsyncExecution::TaskGraph, [this]() {
		FScopeTryLock lock(&CitizenGeneralLoopLock);
		if (!lock.IsLocked())
			return;

		for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
			if (faction.Citizens.IsEmpty())
				continue;

			for (FPartyStruct& party : faction.Politics.Parties) {
				if (party.Leader != nullptr)
					continue;

				SelectNewLeader(&party);
			}

			for (FLawStruct& law : faction.Politics.Laws) {
				if (law.Cooldown == 0)
					continue;

				law.Cooldown--;
			}

			int32 rebelCount = 0;
			int32 happinessCount = 0;

			int32 timeToCompleteDay = 360 / (24 * Camera->Grid->AtmosphereComponent->Speed);

			for (ACitizen* citizen : faction.Citizens) {
				if (!IsValid(citizen) || citizen->HealthComponent->GetHealth() == 0)
					continue;

				citizen->AllocatedBuildings.Empty();
				citizen->AllocatedBuildings = { citizen->Building.School, citizen->Building.Employment, citizen->Building.House };

				if (citizen->CanFindAnything(timeToCompleteDay, &faction)) {
					for (ABuilding* building : faction.Buildings) {
						if (!IsValid(building) || building->HealthComponent->GetHealth() == 0 || citizen->AllocatedBuildings.Contains(building) || (!building->IsA<AWork>() && !building->IsA<AHouse>()) || building->GetOccupied().Num() == building->Capacity || !citizen->AIController->CanMoveTo(building->GetActorLocation()))
							continue;

						if (building->IsA<ASchool>())
							citizen->FindEducation(Cast<ASchool>(building), timeToCompleteDay);
						else if (building->IsA<AWork>())
							citizen->FindJob(Cast<AWork>(building), timeToCompleteDay);
						else if (building->IsA<AHouse>())
							citizen->FindHouse(Cast<AHouse>(building), timeToCompleteDay);
					}

					citizen->SetJobHouseEducation(timeToCompleteDay);
				}

				citizen->SetHappiness();

				int32 happiness = citizen->GetHappiness();
				citizen->SetEyesVisuals(happiness);

				happinessCount += happiness;

				if (GetMembersParty(citizen) != nullptr && GetMembersParty(citizen)->Party == "Shell Breakers")
					rebelCount++;

				if (!IsValid(citizen->Building.Employment))
					continue;

				if (citizen->Building.Employment->IsA<AWall>())
					Cast<AWall>(citizen->Building.Employment)->SetEmergency(!Enemies.IsEmpty() || !faction.Rebels.IsEmpty());
				else if (citizen->Building.Employment->IsA<AClinic>())
					Cast<AClinic>(citizen->Building.Employment)->SetEmergency(!Infected.IsEmpty());
			}

			if (!faction.Police.PoliceReports.IsEmpty() && faction.Rebels.IsEmpty() && Enemies.IsEmpty()) {
				for (int32 i = faction.Police.PoliceReports.Num() - 1; i > -1; i--) {
					FPoliceReport report = faction.Police.PoliceReports[i];

					if (report.Witnesses.IsEmpty()) {
						bool bStillFighting = false;

						TArray<ACitizen*> team1;
						team1.Add(report.Team1.Instigator);
						team1.Append(report.Team1.Assistors);

						TArray<ACitizen*> team2;
						team1.Add(report.Team2.Instigator);
						team1.Append(report.Team2.Assistors);

						for (ACitizen* citizen : team1) {
							for (ACitizen* c : team2) {
								if (!citizen->AttackComponent->OverlappingEnemies.Contains(c))
									continue;

								bStillFighting = true;

								break;
							}

							if (bStillFighting)
								break;
						}

						if (bStillFighting)
							continue;
					}

					if (IsValid(report.RespondingOfficer)) {
						if (report.Witnesses.IsEmpty()) {
							faction.Police.PoliceReports.RemoveAt(i);

							report.RespondingOfficer->AIController->DefaultAction();
						}

						continue;
					}
					else if (report.Witnesses.IsEmpty()) {
						faction.Police.PoliceReports.RemoveAt(i);

						continue;
					}

					float distance = 1000;
					ACitizen* officer = nullptr;

					for (ABuilding* building : faction.Buildings) {
						if (!building->IsA(PoliceStationClass) || building->GetCitizensAtBuilding().IsEmpty())
							continue;

						float dist = FVector::Dist(report.Location, building->GetActorLocation());

						if (distance > dist) {
							distance = dist;

							officer = building->GetCitizensAtBuilding()[Camera->Grid->Stream.RandRange(0, building->GetCitizensAtBuilding().Num() - 1)];
						}
					}

					TArray<ACitizen*> witnesses;
					report.Witnesses.GenerateKeyArray(witnesses);

					if (IsValid(officer)) {
						faction.Police.PoliceReports[i].RespondingOfficer = officer;

						Camera->Grid->AIVisualiser->ToggleOfficerLights(officer, 1.0f);

						officer->AIController->AIMoveTo(witnesses[0]);
					}
				}
			}

			float rebelsPerc = rebelCount / (float)faction.Citizens.Num();

			if (!faction.Citizens.IsEmpty() && rebelsPerc > 0.33f && !IsRebellion(&faction)) {
				faction.RebelCooldownTimer--;

				if (faction.RebelCooldownTimer < 1) {
					int32 value = Camera->Grid->Stream.RandRange(1, 3);

					if (value == 3) {
						Overthrow(&faction);
					}
					else {
						FLawStruct lawStruct;
						lawStruct.BillType = "Abolish";

						int32 index = faction.Politics.Laws.Find(lawStruct);

						ProposeBill(faction.Name, faction.Politics.Laws[index]);
					}
				}
			}

			if (faction.Name == Camera->ColonyName) {
				float happinessPerc = happinessCount / (faction.Citizens.Num() * 100.0f);

				Async(EAsyncExecution::TaskGraphMainTick, [this, happinessPerc, rebelsPerc]() { Camera->UpdateUI(happinessPerc, rebelsPerc); });
			}
		}
	});
}

void UCitizenManager::CalculateDisease()
{
	if (Infected.IsEmpty() && Injured.IsEmpty())
		return;

	Async(EAsyncExecution::TaskGraph, [this]() {
		FScopeTryLock lock(&DiseaseSpreadLock);
		if (!lock.IsLocked())
			return;

		for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
			TArray<ACitizen*> citizens;
			citizens.Append(faction.Citizens);
			citizens.Append(faction.Rebels);

			for (ACitizen* citizen : citizens) {
				if (!IsValid(citizen) || citizen->HealthComponent->GetHealth() <= 0 || citizen->IsHidden() || faction.Police.Arrested.Contains(citizen))
					continue;

				for (FConditionStruct& condition : citizen->HealthIssues) {
					condition.Level++;

					if (condition.Level == condition.DeathLevel)
						citizen->HealthComponent->TakeHealth(1000, citizen);
				}

				if (citizen->HealthComponent->GetHealth() <= 0 || (!Infected.Contains(citizen) && (!IsValid(citizen->Building.Employment) || !citizen->Building.Employment->IsA<AClinic>())))
					continue;

				if (IsValid(citizen->Building.Employment) && citizen->Building.Employment->IsA<AClinic>() && faction.Citizens.Contains(citizen->AIController->MoveRequest.GetGoalActor()) && citizen->CanReach(citizen->AIController->MoveRequest.GetGoalActor(), citizen->Range / 15.0f)) {
					TArray<FTimerParameterStruct> params;
					SetParameter(citizen->AIController->MoveRequest.GetGoalActor(), params);
					
					CreateTimer("Healing", citizen, 1.0f / citizen->GetProductivity(), "Heal", params, false);
				}
				else if (Infectible.Contains(citizen) && Infected.Contains(citizen)) {
					FOverlapsStruct requestedOverlaps;
					requestedOverlaps.GetCitizenInteractions(false, true);

					TArray<AActor*> actors = Camera->Grid->AIVisualiser->GetOverlaps(Camera, citizen, citizen->Range / 15.0f, requestedOverlaps, EFactionType::Both);

					for (AActor* actor : actors) {
						if (!IsValid(actor))
							continue;

						ACitizen* c = Cast<ACitizen>(actor);

						if (c->HealthComponent->GetHealth() <= 0 || !Infectible.Contains(c))
							continue;

						bool bInfected = false;

						for (FConditionStruct condition : citizen->HealthIssues) {
							if (c->HealthIssues.Contains(condition))
								continue;

							int32 chance = Camera->Grid->Stream.RandRange(1, 100);

							if (chance <= condition.Spreadability) {
								c->HealthIssues.Add(condition);

								bInfected = true;

								Camera->NotifyLog("Bad", citizen->BioStruct.Name + " is infected with " + condition.Name, Camera->ConquestManager->GetCitizenFaction(citizen).Name);
							}
						}

						if (bInfected && !Infected.Contains(c))
							Infect(c);
					}
				}
			}

			PairCitizenToHealer(&faction);
		}
	});
}

void UCitizenManager::CalculateCitizenInteractions()
{
	Async(EAsyncExecution::TaskGraph, [this]() {
		FScopeTryLock lock(&GoalInteractionsLock);
		if (!lock.IsLocked())
			return;

		for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
			TArray<ACitizen*> citizens = faction.Citizens;

			for (ACitizen* citizen : citizens) {
				if (!IsValid(citizen) || citizen->HealthComponent->GetHealth() <= 0 || citizen->IsHidden() || faction.Police.Arrested.Contains(citizen) || !citizen->AttackComponent->OverlappingEnemies.IsEmpty())
					continue;

				if (!IsValid(citizen->AIController->MoveRequest.GetGoalActor()) || citizen->AIController->MoveRequest.GetGoalActor()->IsA<AAI>())
					continue;

				FOverlapsStruct requestedOverlaps;
				requestedOverlaps.bBuildings = true;
				requestedOverlaps.bResources = true;

				float reach = citizen->Range / 15.0f;
				TArray<AActor*> actors = Camera->Grid->AIVisualiser->GetOverlaps(Camera, citizen, reach, requestedOverlaps, EFactionType::Same, &faction);

				for (AActor* actor : actors) {
					if (citizen->AIController->MoveRequest.GetGoalActor() != actor || !citizen->CanReach(actor, reach, citizen->AIController->MoveRequest.GetGoalInstance()))
						continue;

					Async(EAsyncExecution::TaskGraphMainTick, [this, citizen, actor]() {
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
		}
	});

	Async(EAsyncExecution::TaskGraph, [this]() {
		FScopeTryLock lock(&ConversationInteractionsLock);
		if (!lock.IsLocked())
			return;

		for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
			TArray<ACitizen*> citizens = faction.Citizens;

			if (!Enemies.IsEmpty() || !faction.Rebels.IsEmpty())
				continue;

			for (ACitizen* citizen : citizens) {
				if (!IsValid(citizen) || citizen->HealthComponent->GetHealth() <= 0 || citizen->IsHidden() || faction.Police.Arrested.Contains(citizen) || !citizen->AttackComponent->OverlappingEnemies.IsEmpty())
					continue;

				bool bCitizenInReport = IsInAPoliceReport(citizen, &faction);

				if (citizen->bConversing || citizen->bSleep || (bCitizenInReport && (!IsValid(citizen->Building.Employment) || !citizen->Building.Employment->IsA(PoliceStationClass))))
					continue;

				int32 reach = citizen->Range / 15.0f;
				
				if (IsValid(citizen->AIController->MoveRequest.GetGoalActor()) && IsValid(citizen->Building.Employment) && citizen->Building.Employment->IsA(PoliceStationClass)) {
					if (!citizen->CanReach(citizen->AIController->MoveRequest.GetGoalActor(), reach) || !citizen->AIController->MoveRequest.GetGoalActor()->IsA<ACitizen>())
						continue;

					ACitizen* c = Cast<ACitizen>(citizen->AIController->MoveRequest.GetGoalActor());

					bool bInterrogate = true;

					for (FPoliceReport report : faction.Police.PoliceReports) {
						if (report.RespondingOfficer != citizen)
							continue;

						if (report.Witnesses.Num() == report.Impartial.Num() + report.AcussesTeam1.Num() + report.AcussesTeam2.Num())
							bInterrogate = false;

						break;
					}

					if (bInterrogate) {
						if (!c->AttackComponent->OverlappingEnemies.IsEmpty())
							StopFighting(c);

						StartConversation(&faction, citizen, c, true);
					}
					else {
						Arrest(citizen, c);
					}
				}
				else if (Camera->Grid->Stream.RandRange(0, 1000) == 1000) {
					FOverlapsStruct requestedOverlaps;
					requestedOverlaps.bCitizens = true;

					TArray<AActor*> actors = Camera->Grid->AIVisualiser->GetOverlaps(Camera, citizen, reach, requestedOverlaps, EFactionType::Same, &faction);
					TArray<ACitizen*> citizensToTalkTo;

					for (AActor* actor : actors) {
						ACitizen* c = Cast<ACitizen>(actor);

						if (!c->bConversing && !c->bSleep && c->AttackComponent->OverlappingEnemies.IsEmpty() && !bCitizenInReport && !IsInAPoliceReport(c, &faction))
							continue;

						citizensToTalkTo.Add(c);
					}

					if (citizensToTalkTo.IsEmpty())
						continue;

					int32 index = Camera->Grid->Stream.RandRange(0, citizensToTalkTo.Num() - 1);

					StartConversation(&faction, citizen, citizensToTalkTo[index], false);
				}
			}
		}
	});

	Async(EAsyncExecution::TaskGraph, [this]() {
		FScopeTryLock lock(&VandalismInteractionsLock);
		if (!lock.IsLocked())
			return;

		for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
			TArray<ACitizen*> citizens = faction.Citizens;

			if (!Enemies.IsEmpty() || !faction.Rebels.IsEmpty() || faction.Buildings.Num() < 2)
				continue;

			for (ACitizen* citizen : citizens) {
				if (!IsValid(citizen) || citizen->HealthComponent->GetHealth() <= 0 || citizen->IsHidden() || faction.Police.Arrested.Contains(citizen) || !citizen->AttackComponent->OverlappingEnemies.IsEmpty())
					continue;

				FPartyStruct* partyStruct = GetMembersParty(citizen);

				if (citizen->bConversing || citizen->bSleep || IsValid(citizen->Building.BuildingAt) || partyStruct == nullptr || partyStruct->Party != "Shell Breakers")
					continue;

				FOverlapsStruct requestedOverlaps;
				requestedOverlaps.bBuildings = true;

				int32 reach = citizen->Range / 15.0f;
				TArray<AActor*> actors = Camera->Grid->AIVisualiser->GetOverlaps(Camera, citizen, reach, requestedOverlaps, EFactionType::Same, &faction);

				for (AActor* actor : actors) {
					if (actor->IsA<ABroch>())
						continue;

					int32 aggressiveness = 0;
					TArray<FPersonality*> personalities = GetCitizensPersonalities(citizen);

					for (FPersonality* personality : personalities)
						aggressiveness += personality->Aggressiveness / personalities.Num();

					int32 max = (1000 + (citizen->GetHappiness() - 50) * 16) / aggressiveness;

					if (Camera->Grid->Stream.RandRange(1, max) != max)
						continue;

					Async(EAsyncExecution::TaskGraphMainTick, [this, citizen, actor]() { Camera->Grid->AtmosphereComponent->SetOnFire(actor); });

					int32 index = INDEX_NONE;

					FOverlapsStruct rqstdOvrlps;
					rqstdOvrlps.bCitizens = true;

					TArray<AActor*> actrs = Camera->Grid->AIVisualiser->GetOverlaps(Camera, citizen, citizen->Range, rqstdOvrlps, EFactionType::Same, &faction);

					for (AActor* a : actrs) {
						ACitizen* witness = Cast<ACitizen>(a);

						if (witness->Building.BuildingAt == actor || witness->Building.House == actor) {
							if (!witness->AttackComponent->OverlappingEnemies.Contains(citizen))
								witness->AttackComponent->OverlappingEnemies.Add(citizen);

							if (!citizen->AttackComponent->OverlappingEnemies.Contains(witness))
								citizen->AttackComponent->OverlappingEnemies.Add(witness);

							if (witness->AttackComponent->OverlappingEnemies.Num() == 1)
								witness->AttackComponent->SetComponentTickEnabled(true);

							if (citizen->AttackComponent->OverlappingEnemies.Num() == 1)
								citizen->AttackComponent->SetComponentTickEnabled(true);

							witness->AttackComponent->bShowMercy = false;
							citizen->AttackComponent->bShowMercy = false;
						}
						else {
							CreatePoliceReport(&faction, witness, citizen, EReportType::Vandalism, index);
						}
					}
				}
			}
		}
	});

	Async(EAsyncExecution::TaskGraph, [this]() {
		FScopeTryLock lock(&ReportInteractionsLock);
		if (!lock.IsLocked())
			return;

		for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
			for (int32 i = 0; i < faction.Police.PoliceReports.Num(); i++) {
				FPoliceReport report = faction.Police.PoliceReports[i];

				FOverlapsStruct requestedOverlaps;
				requestedOverlaps.bCitizens = true;

				TArray<AActor*> actors = Camera->Grid->AIVisualiser->GetOverlaps(Camera, report.Team1.Instigator, report.Team1.Instigator->InitialRange, requestedOverlaps, EFactionType::Same, &faction, report.Location);

				for (AActor* actor : actors) {
					ACitizen* citizen = Cast<ACitizen>(actor);

					if (!IsValid(citizen) || citizen->HealthComponent->GetHealth() <= 0 || faction.Police.Arrested.Contains(citizen) || IsCarelessWitness(citizen) || !citizen->AttackComponent->OverlappingEnemies.IsEmpty() || IsInAPoliceReport(citizen, &faction))
						continue;

					ACitizen* citizenToAttack = nullptr;

					int32 count1 = 0;
					int32 count2 = 0;

					float citizenAggressiveness = 0;
					float c1Aggressiveness = 0;
					float c2Aggressiveness = 0;

					PersonalityComparison(citizen, report.Team1.Instigator, count1, citizenAggressiveness, c1Aggressiveness);

					if (citizenAggressiveness >= 1.0f) {
						PersonalityComparison(citizen, report.Team2.Instigator, count2, citizenAggressiveness, c2Aggressiveness);

						if (count1 > 1 && count2 < 1)
							citizenToAttack = report.Team2.Instigator;
						else if (count1 < 1 && count2 > 1)
							citizenToAttack = report.Team1.Instigator;
					}

					if (IsValid(citizenToAttack)) {
						if (!citizen->AttackComponent->OverlappingEnemies.Contains(citizenToAttack))
							citizen->AttackComponent->OverlappingEnemies.Add(citizenToAttack);

						if (!citizenToAttack->AttackComponent->OverlappingEnemies.Contains(citizen))
							citizenToAttack->AttackComponent->OverlappingEnemies.Add(citizen);

						if (citizen->AttackComponent->OverlappingEnemies.Num() == 1)
							citizen->AttackComponent->SetComponentTickEnabled(true);

						citizen->AttackComponent->bShowMercy = citizenToAttack->AttackComponent->bShowMercy;

						if (faction.Police.PoliceReports[i].Team1.Instigator == citizenToAttack)
							faction.Police.PoliceReports[i].Team2.Assistors.Add(citizen);
						else
							faction.Police.PoliceReports[i].Team1.Assistors.Add(citizen);
					}
					else {
						CreatePoliceReport(&faction, citizen, report.Team1.Instigator, EReportType::Fighting, i);
					}
				}
			}
		}
	});
}

void UCitizenManager::CalculateFighting()
{
	Async(EAsyncExecution::TaskGraph, [this]() {
		FScopeTryLock lock(&FightLock);
		if (!lock.IsLocked())
			return;

		bool bEnemies = false;

		TMap<FString, TArray<AAI*>> ais;

		for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
			if (!ais.Contains("Citizens"))
				ais.Add("Citizens");

			TArray<AAI*>* ai = ais.Find("Citizens");
			ai->Append(faction.Citizens);

			if (!ais.Contains("Clones"))
				ais.Add("Clones");

			ai = ais.Find("Clones");
			ai->Append(faction.Clones);

			if (!ais.Contains("Rebels"))
				ais.Add("Rebels");

			ai = ais.Find("Rebels");
			ai->Append(faction.Rebels);

			if (!faction.Rebels.IsEmpty())
				bEnemies = true;
		}

		ais.Add("Enemies", Enemies);

		if (!Enemies.IsEmpty())
			bEnemies = true;

		if (!bEnemies)
			return;

		for (auto& element : ais) {
			for (AAI* ai : element.Value) {
				if (!IsValid(ai) || ai->HealthComponent->GetHealth() <= 0)
					continue;

				FOverlapsStruct requestedOverlaps;

				if (element.Key == "Citizens" || element.Key == "Clones")
					requestedOverlaps.GetCitizenEnemies();
				else if (element.Key == "Rebels")
					requestedOverlaps.GetRebelsEnemies();
				else
					requestedOverlaps.GetEnemyEnemies();

				TArray<AActor*> actors = Camera->Grid->AIVisualiser->GetOverlaps(Camera, ai, ai->Range, requestedOverlaps, EFactionType::Both);

				for (AActor* actor : actors) {
					if (!IsValid(actor) || ai->AttackComponent->OverlappingEnemies.Contains(actor))
						continue;

					UHealthComponent* healthComp = actor->GetComponentByClass<UHealthComponent>();

					if ((healthComp && healthComp->GetHealth() <= 0) || (!*ai->AttackComponent->ProjectileClass && !ai->AIController->CanMoveTo(Camera->GetTargetActorLocation(actor))))
						continue;

					ai->AttackComponent->OverlappingEnemies.Add(actor);

					if (ai->AttackComponent->OverlappingEnemies.Num() == 1)
						ai->AttackComponent->SetComponentTickEnabled(true);
				}
			}
		}
	});
}

void UCitizenManager::CreateTimer(FString Identifier, AActor* Actor, float Time, FName FunctionName, TArray<FTimerParameterStruct> Params, bool Repeat, bool OnGameThread)
{
	FScopeLock lock(&TimerLock);

	FTimerStruct timer;
	timer.CreateTimer(Identifier, Actor, Time, FunctionName, Params, Repeat, OnGameThread);
	timer.LastUpdateTime = GetWorld()->GetTimeSeconds();

	Timers.AddTail(timer);
}

FTimerStruct* UCitizenManager::FindTimer(FString ID, AActor* Actor)
{
	FScopeLock lock(&TimerLock);

	if (!IsValid(Actor))
		return nullptr;
	
	FTimerStruct timer;
	timer.ID = ID;
	timer.Actor = Actor;

	auto node = Timers.FindNode(timer);

	if (node == nullptr)
		return nullptr;

	return &node->GetValue();
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

float UCitizenManager::GetElapsedPercentage(FString ID, AActor* Actor)
{
	FTimerStruct* timer = FindTimer(ID, Actor);

	if (timer == nullptr)
		return 0.0f;

	return (timer->Target - timer->Timer) / timer->Target;
}

bool UCitizenManager::DoesTimerExist(FString ID, AActor* Actor)
{
	FTimerStruct* timer = FindTimer(ID, Actor);

	if (timer == nullptr)
		return false;

	return true;
}

#define SET_OBJECT(object) \
	function = object->FindFunction(Timer->FuncName); \
	if (function != nullptr) \
		return TTuple<UObject*, UFunction*>(object, function);

TTuple<UObject*, UFunction*> UCitizenManager::GetFunction(FTimerStruct* Timer)
{
	UFunction* function = nullptr;

	SET_OBJECT(Timer->Actor);

	if (Timer->Actor->IsA<AAI>()) {
		AAI* ai = Cast<AAI>(Timer->Actor);

		SET_OBJECT(ai->AIController);

		if (ai->IsA<ACitizen>() && IsValid(Cast<ACitizen>(ai)->Building.Orphanage))
			SET_OBJECT(Cast<ACitizen>(ai)->Building.Orphanage);
	}

	UHealthComponent* healthComp = Timer->Actor->FindComponentByClass<UHealthComponent>();
	if (healthComp)
		SET_OBJECT(healthComp);

	if (Timer->Actor->IsA<AGrid>()) {
		AGrid* grid = Cast<AGrid>(Timer->Actor);

		SET_OBJECT(grid->AtmosphereComponent);
		SET_OBJECT(grid->AtmosphereComponent->Clouds);
		SET_OBJECT(grid->AtmosphereComponent->NaturalDisasterComponent);
	}

	if (Timer->Actor->IsA<ACamera>()) {
		SET_OBJECT(Camera->SaveGameComponent);
		SET_OBJECT(Camera->ConquestManager);
		SET_OBJECT(Camera->ResourceManager);
	}

	SET_OBJECT(this);

	UE_LOGFMT(LogTemp, Fatal, "Function '{name}' not found with '{actor}'", Timer->FuncName.ToString(), Timer->Actor->GetName());
}

#define SET_TYPE(type_) \
	if (type == #type_) \
		*functionProperty->ContainerPtrToValuePtr<type_>(buffer) = GetParameter<type_>(Timer, count);

void UCitizenManager::CallTimerFunction(FTimerStruct* Timer)
{
	TTuple<UObject*, UFunction*> objFunc = GetFunction(Timer);

	int32 count = 0;

	uint8* buffer = (uint8*)FMemory_Alloca(objFunc.Value->ParmsSize);
	FMemory::Memzero(buffer, objFunc.Value->ParmsSize);

	for (TFieldIterator<FProperty> it(objFunc.Value); it && it->HasAnyPropertyFlags(CPF_Parm); ++it) {
		FProperty* functionProperty = *it;
		FString type = functionProperty->GetCPPType();

		// Actors
		SET_TYPE(AActor*);
		SET_TYPE(ABuilding*);
		SET_TYPE(ACitizen*);

		// Objects
		SET_TYPE(USoundBase*);
		SET_TYPE(UHierarchicalInstancedStaticMeshComponent*);

		// Rest
		SET_TYPE(FVector);
		SET_TYPE(TArray<FVector>);
		SET_TYPE(FLinearColor);
		SET_TYPE(bool);
		SET_TYPE(FFactionStruct);
		SET_TYPE(int32);
		SET_TYPE(float);
		SET_TYPE(FString);
		SET_TYPE(FGuid);
		SET_TYPE(FLawStruct);
		SET_TYPE(TSubclassOf<AResource>);

		count++;
	}

	FFrame stack(objFunc.Key, objFunc.Value, buffer, NULL, objFunc.Value->ChildProperties);

	const bool bHasReturnParam = objFunc.Value->ReturnValueOffset != MAX_uint16;
	uint8* ReturnValueAddress = bHasReturnParam ? (buffer + objFunc.Value->ReturnValueOffset) : nullptr;

	objFunc.Value->Invoke(objFunc.Key, stack, nullptr);

	Timer->bDone = true;
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
void UCitizenManager::UpdateRent(FString FactionName, TSubclassOf<class AHouse> HouseType, int32 NewRent)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	for (ABuilding* building : faction->Buildings) {
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
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", Citizen);

	for (FPartyStruct& party : faction->Politics.Parties) {
		if (!party.Members.Contains(Citizen))
			continue;

		if (party.Leader == Citizen)
			SelectNewLeader(&party);

		party.Members.Remove(Citizen);

		break;
	}

	for (ACitizen* citizen : Citizen->GetLikedFamily(false))
		citizen->SetDecayHappiness(&citizen->FamilyDeathHappiness, -12);

	FOverlapsStruct requestedOverlaps;
	requestedOverlaps.GetCitizenInteractions(false, false);

	TArray<AActor*> actors = Camera->Grid->AIVisualiser->GetOverlaps(Camera, Citizen, Citizen->Range, requestedOverlaps, EFactionType::Same);

	for (AActor* actor : actors) {
		ACitizen* citizen = Cast<ACitizen>(actor);

		if (citizen->HealthComponent->GetHealth() <= 0 || citizen->IsHidden())
			continue;

		bool bIsCruel = false;

		for (FPersonality* personality : GetCitizensPersonalities(citizen))
			if (personality->Trait == "Cruel")
				bIsCruel = true;

		int32 happinessValue = -6;

		if (bIsCruel)
			happinessValue = 6;

		citizen->SetDecayHappiness(&citizen->WitnessedDeathHappiness, happinessValue);
	}

	for (FPersonality* personality : GetCitizensPersonalities(Citizen))
		if (personality->Citizens.Contains(Citizen))
			personality->Citizens.Remove(Citizen);

	Camera->ConquestManager->RemoveFromArmy(Citizen);
}

//
// Work
//
void UCitizenManager::CheckWorkStatus(int32 Hour)
{
	for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
		for (ACitizen* citizen : faction.Citizens) {
			for (auto element : citizen->HoursWorked) {
				if (IsValid(citizen->Building.Employment) && citizen->Building.Employment->IsWorking(citizen, Hour))
					if (!citizen->HoursWorked.Contains(Hour))
						citizen->HoursWorked.Add(Hour, citizen->Building.Employment->WagePerHour);
					else if (citizen->HoursWorked.Contains(Hour))
						citizen->HoursWorked.Remove(Hour);
			}
		}

		for (ABuilding* building : faction.Buildings) {
			if (!IsValid(building) || !building->IsA<AWork>())
				continue;

			Cast<AWork>(building)->CheckWorkStatus(Hour);
		}
	}
}

ERaidPolicy UCitizenManager::GetRaidPolicyStatus(ACitizen* Citizen)
{
	ERaidPolicy policy = ERaidPolicy::Default;

	if (!Enemies.IsEmpty())
		policy = ERaidPolicy(GetLawValue(Camera->ConquestManager->GetCitizenFaction(Citizen).Name, "Raid Policy"));

	return policy;
}

//
// Citizen
//
void UCitizenManager::CheckUpkeepCosts()
{
	for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
		for (ACitizen* citizen : faction.Citizens) {
			float amount = 0.0f;

			for (auto element : citizen->HoursWorked)
				amount += element.Value;

			citizen->Balance += FMath::RoundHalfFromZero(amount);
			Camera->ResourceManager->TakeUniversalResource(&faction, Camera->ResourceManager->Money, amount, -100000);

			if (IsValid(citizen->Building.House))
				citizen->Building.House->GetRent(citizen);
		}
	}
}

void UCitizenManager::CheckCitizenStatus(int32 Hour)
{
	for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
		for (ACitizen* citizen : faction.Citizens) {
			if (citizen->HoursSleptToday.Contains(Hour))
				citizen->HoursSleptToday.Remove(Hour);

			if (citizen->bSleep)
				citizen->HoursSleptToday.Add(Hour);

			if (citizen->HoursSleptToday.Num() < citizen->IdealHoursSlept && !citizen->bSleep && (!IsValid(citizen->Building.Employment) || !citizen->Building.Employment->IsWorking(citizen)) && IsValid(citizen->Building.House) && citizen->Building.BuildingAt == citizen->Building.House) {
				citizen->bSleep = true;

				citizen->Snore(true);
			}
			else if (citizen->bSleep && citizen->HoursSleptToday.Num() >= citizen->IdealHoursSlept) {
				citizen->bSleep = false;

				FTimerStruct* timer = FindTimer("Snore", citizen);

				if (timer != nullptr)
					timer->Actor = nullptr;
			}

			citizen->DecayHappiness();
			citizen->IncrementHoursTogetherWithPartner();
		}
	}

	CheckForWeddings(Hour);
}

void UCitizenManager::CheckForWeddings(int32 Hour)
{
	for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
		TArray<ACitizen*> citizens = faction.Citizens;
		TArray<ACitizen*> checked;

		for (int32 i = citizens.Num() - 1; i > -1; i--) {
			ACitizen* citizen = citizens[i];

			if (citizen->BioStruct.Partner == nullptr || citizen->BioStruct.bMarried || checked.Contains(citizen))
				continue;

			ACitizen* partner = Cast<ACitizen>(citizen->BioStruct.Partner);

			checked.Add(citizen);
			checked.Add(partner);

			if (IsAttendingEvent(citizen) || IsAttendingEvent(partner))
				continue;

			TArray<FPersonality*> personalities = GetCitizensPersonalities(citizen);
			personalities.Append(GetCitizensPersonalities(partner));

			int32 likelihood = 0;

			for (FPersonality* personality : personalities)
				if (personality->Affects.Contains("Marriage"))
					likelihood += *personality->Affects.Find("Marriage");

			int32 chance = citizen->Camera->Grid->Stream.RandRange(80, 150);
			int32 likelihoodChance = likelihood * 10 + citizen->BioStruct.HoursTogetherWithPartner * 5;

			if (likelihoodChance < chance)
				continue;

			TArray<FString> faiths;

			if (citizen->Spirituality.Faith != "Atheist")
				faiths.Add(citizen->Spirituality.Faith);

			if (partner->Spirituality.Faith != "Atheist")
				faiths.Add(partner->Spirituality.Faith);

			if (faiths.IsEmpty())
				faiths.Append({ "Chicken", "Egg" });

			int32 index = citizen->Camera->Grid->Stream.RandRange(0, faiths.Num() - 1);
			FString chosenFaith = faiths[index];

			ABooster* chosenChurch = nullptr;
			ACitizen* priest = nullptr;

			TArray<int32> weddingHours = { Hour };
			for (int32 j = 0; j < 2; j++) {
				int32 hour = Hour + j;

				if (hour > 23)
					hour -= 24;

				weddingHours.Add(hour);
			}

			for (ABuilding* building : faction.Buildings) {
				if (!building->IsA<ABooster>())
					continue;

				ABooster* church = Cast<ABooster>(building);

				church->BuildingsToBoost.GenerateValueArray(faiths);

				if (!church->bHolyPlace || faiths[0] != chosenFaith || church->GetOccupied().IsEmpty())
					continue;

				bool bAvailable = false;

				for (ACitizen* p : church->GetOccupied()) {
					if (IsAttendingEvent(p))
						continue;

					bool bWithinHours = true;
					for (int32 hour : weddingHours)
						if (!church->IsWorking(p, hour))
							bWithinHours = false;

					if (!bWithinHours)
						continue;

					bAvailable = true;
					priest = p;

					break;
				}

				if (!bAvailable)
					continue;

				if (chosenChurch == nullptr) {
					chosenChurch = church;

					continue;
				}

				double magnitude = citizen->AIController->GetClosestActor(400.0f, Camera->GetTargetActorLocation(citizen), chosenChurch->GetActorLocation(), church->GetActorLocation());

				if (magnitude <= 0.0f)
					continue;

				chosenChurch = church;
			}

			if (!IsValid(chosenChurch))
				continue;

			for (FEventStruct event : faction.Events) {
				bool bContainsWeddingHour = false;

				for (int32 hour : weddingHours) {
					if (!event.Hours.Contains(hour))
						continue;

					bContainsWeddingHour = true;

					break;
				}

				if (!bContainsWeddingHour)
					continue;

				if (chosenChurch->IsA(event.Building))
					return;
			}

			TArray<ACitizen*> whitelist = { citizen, partner, priest };
			whitelist.Append(citizen->GetLikedFamily(false));
			whitelist.Append(partner->GetLikedFamily(false));

			FCalendarStruct calendar = citizen->Camera->Grid->AtmosphereComponent->Calendar;

			CreateEvent(faction.Name, EEventType::Marriage, nullptr, chosenChurch, "", calendar.Days[calendar.Index], weddingHours, false, whitelist);
		}
	}
}

float UCitizenManager::GetAggressiveness(ACitizen* Citizen)
{
	TArray<FPersonality*> citizenPersonalities = GetCitizensPersonalities(Citizen);
	float citizenAggressiveness = 0;

	for (FPersonality* personality : citizenPersonalities)
		citizenAggressiveness += personality->Aggressiveness / citizenPersonalities.Num();

	return citizenAggressiveness;
}

void UCitizenManager::PersonalityComparison(ACitizen* Citizen1, ACitizen* Citizen2, int32& Likeness, float& Citizen1Aggressiveness, float& Citizen2Aggressiveness)
{
	TArray<FPersonality*> citizen1Personalities = GetCitizensPersonalities(Citizen1);
	TArray<FPersonality*> citizen2Personalities = GetCitizensPersonalities(Citizen2);

	for (FPersonality* personality : citizen1Personalities) {
		Citizen1Aggressiveness += personality->Aggressiveness / citizen1Personalities.Num();

		for (FPersonality* p : citizen2Personalities) {
			Citizen2Aggressiveness += p->Aggressiveness / citizen2Personalities.Num();

			if (personality->Trait == p->Trait)
				Likeness += 2;
			else if (personality->Likes.Contains(p->Trait))
				Likeness++;
			else if (personality->Dislikes.Contains(p->Trait))
				Likeness--;
		}
	}
}

void UCitizenManager::PersonalityComparison(ACitizen* Citizen1, ACitizen* Citizen2, int32& Likeness)
{
	TArray<FPersonality*> citizen1Personalities = GetCitizensPersonalities(Citizen1);
	TArray<FPersonality*> citizen2Personalities = GetCitizensPersonalities(Citizen2);

	for (FPersonality* personality : citizen1Personalities) {
		for (FPersonality* p : citizen2Personalities) {
			if (personality->Trait == p->Trait)
				Likeness += 2;
			else if (personality->Likes.Contains(p->Trait))
				Likeness++;
			else if (personality->Dislikes.Contains(p->Trait))
				Likeness--;
		}
	}
}

//
// Police
//
USoundBase* UCitizenManager::GetConversationSound(ACitizen* Citizen)
{
	Citizen->bConversing = true;
	Citizen->AIController->StopMovement();

	int32 index = Camera->Grid->Stream.RandRange(0, Citizen->NormalConversations.Num() - 1);
	USoundBase* convo = Citizen->NormalConversations[index];

	for (FPersonality* personality : GetCitizensPersonalities(Citizen)) {
		if (personality->Trait != "Inept")
			continue;

		index = Camera->Grid->Stream.RandRange(0, Citizen->IneptIdiotConversations.Num() - 1);

		convo = Citizen->IneptIdiotConversations[index];

		break;
	}

	return convo;
}

void UCitizenManager::StartConversation(FFactionStruct* Faction, ACitizen* Citizen1, ACitizen* Citizen2, bool bInterrogation)
{
	Citizen1->MovementComponent->ActorToLookAt = Citizen2;
	Citizen2->MovementComponent->ActorToLookAt = Citizen1;

	USoundBase* convo1 = GetConversationSound(Citizen1);
	USoundBase* convo2 = GetConversationSound(Citizen2);
	
	Async(EAsyncExecution::TaskGraphMainTick, [this, Faction, Citizen1, Citizen2, bInterrogation, convo1, convo2]() {
		TArray<FTimerParameterStruct> params;
		SetParameter(*Faction, params);
		SetParameter(Citizen1, params);
		SetParameter(Citizen2, params);

		if (bInterrogation)
			CreateTimer("Interrogate", Citizen1, 6.0f, "InterrogateWitnesses", params, false);
		else
			CreateTimer("Interact", Citizen1, 6.0f, "Interact", params, false);

		Camera->PlayAmbientSound(Citizen1->AmbientAudioComponent, convo1, Citizen1->VoicePitch);
		Camera->PlayAmbientSound(Citizen2->AmbientAudioComponent, convo2, Citizen2->VoicePitch);
	});
}

void UCitizenManager::Interact(FFactionStruct Faction, ACitizen* Citizen1, ACitizen* Citizen2)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(Faction.Name);

	Citizen1->bConversing = false;
	Citizen2->bConversing = false;

	int32 count = 0;

	float citizen1Aggressiveness = 0;
	float citizen2Aggressiveness = 0;

	PersonalityComparison(Citizen1, Citizen2, count, citizen1Aggressiveness, citizen2Aggressiveness);

	int32 chance = Camera->Grid->Stream.RandRange(0, 100);
	int32 positiveConversationLikelihood = 50 + count * 25;

	int32 happinessValue = 12;

	if (chance > positiveConversationLikelihood) {
		happinessValue = -12;

		FVector midPoint = (Camera->GetTargetActorLocation(Citizen1) + Camera->GetTargetActorLocation(Citizen2)) / 2;
		float distance = 1000;

		for (ABuilding* building : faction->Buildings) {
			if (!building->IsA(PoliceStationClass) || building->GetCitizensAtBuilding().IsEmpty())
				continue;

			float dist = FVector::Dist(midPoint, building->GetActorLocation());

			if (distance > dist)
				distance = dist;
		}

		ACitizen* aggressor = Citizen1;

		if (citizen1Aggressiveness < citizen2Aggressiveness)
			aggressor = Citizen2;

		FOverlapsStruct requestedOverlaps;
		requestedOverlaps.GetCitizenInteractions(false, false);

		TArray<AActor*> actors = Camera->Grid->AIVisualiser->GetOverlaps(Camera, aggressor, aggressor->Range, requestedOverlaps, EFactionType::Same);

		chance = Camera->Grid->Stream.RandRange(0, 100);
		int32 fightChance = 25 * citizen1Aggressiveness * citizen2Aggressiveness - FMath::RoundHalfFromZero(300 / distance) - (10 * actors.Num());

		if (fightChance > chance) {
			if (!Citizen1->AttackComponent->OverlappingEnemies.Contains(Citizen2))
				Citizen1->AttackComponent->OverlappingEnemies.Add(Citizen2);

			if (!Citizen2->AttackComponent->OverlappingEnemies.Contains(Citizen1))
				Citizen2->AttackComponent->OverlappingEnemies.Add(Citizen1);

			if (Citizen1->AttackComponent->OverlappingEnemies.Num() == 1)
				Citizen1->AttackComponent->SetComponentTickEnabled(true);

			if (Citizen2->AttackComponent->OverlappingEnemies.Num() == 1)
				Citizen2->AttackComponent->SetComponentTickEnabled(true);

			if (fightChance < 66) {
				Citizen1->AttackComponent->bShowMercy = true;
				Citizen2->AttackComponent->bShowMercy = true;
			}

			int32 index = INDEX_NONE;

			CreatePoliceReport(faction, nullptr, Citizen1, EReportType::Fighting, index);
		}
	}

	Citizen1->SetDecayHappiness(&Citizen1->ConversationHappiness, happinessValue);
	Citizen2->SetDecayHappiness(&Citizen2->ConversationHappiness, happinessValue);

	if (!Citizen1->AttackComponent->IsComponentTickEnabled()) {
		Citizen1->AIController->DefaultAction();
		Citizen2->AIController->DefaultAction();
	}
}

bool UCitizenManager::IsCarelessWitness(ACitizen* Citizen)
{
	for (FPersonality* personality : GetCitizensPersonalities(Citizen))
		if (personality->Trait == "Careless")
			return true;

	return false;
}

void UCitizenManager::CreatePoliceReport(FFactionStruct* Faction, ACitizen* Witness, ACitizen* Accused, EReportType ReportType, int32& Index)
{
	if (Index == INDEX_NONE) {
		FPoliceReport report;

		if (ReportType == EReportType::Vandalism) {
			report.Type = EReportType::Vandalism;

			report.Team1.Instigator = Accused;

			report.Location = Camera->GetTargetActorLocation(report.Team1.Instigator);
		}
		else {
			report.Type = EReportType::Fighting;

			for (int32 i = 0; i < Accused->AttackComponent->OverlappingEnemies.Num(); i++) {
				ACitizen* fighter1 = Cast<ACitizen>(Accused->AttackComponent->OverlappingEnemies[i]);

				if (i == 0) {
					report.Team2.Instigator = fighter1;

					for (int32 j = 0; j < fighter1->AttackComponent->OverlappingEnemies.Num(); j++) {
						ACitizen* fighter2 = Cast<ACitizen>(Accused->AttackComponent->OverlappingEnemies[i]);

						if (i == 0)
							report.Team1.Instigator = fighter2;
						else
							report.Team1.Assistors.Add(fighter2);
					}
				}
				else {
					report.Team2.Assistors.Add(fighter1);
				}
			}

			report.Location = (Camera->GetTargetActorLocation(report.Team1.Instigator) + Camera->GetTargetActorLocation(report.Team2.Instigator)) / 2;
		}

		Faction->Police.PoliceReports.Add(report);

		Index = Faction->Police.PoliceReports.Num() - 1;
	}

	if (!IsValid(Witness))
		return;

	float distance = FVector::Dist(Camera->GetTargetActorLocation(Witness), Camera->GetTargetActorLocation(Accused));
	float dist = 1000000000;

	if (ReportType == EReportType::Fighting) {
		if (Faction->Police.PoliceReports[Index].Witnesses.Contains(Witness))
			dist = *Faction->Police.PoliceReports[Index].Witnesses.Find(Witness);

		GetCloserToFight(Witness, Accused, Faction->Police.PoliceReports[Index].Location);
	}

	if (distance < dist)
		Faction->Police.PoliceReports[Index].Witnesses.Add(Witness, distance);
}

bool UCitizenManager::IsInAPoliceReport(ACitizen* Citizen, FFactionStruct* Faction)
{
	for (FPoliceReport& report : Faction->Police.PoliceReports) {
		if (!report.Witnesses.Contains(Citizen) && report.RespondingOfficer != Citizen && !report.AcussesTeam1.Contains(Citizen) && !report.AcussesTeam2.Contains(Citizen))
			continue;

		return true;
	}

	return false;
}

void UCitizenManager::ChangeReportToMurder(ACitizen* Citizen)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", Citizen);

	for (FPoliceReport& report : faction->Police.PoliceReports) {
		if (!report.Team1.GetTeam().Contains(Citizen) && !report.Team2.GetTeam().Contains(Citizen))
			continue;

		report.Type = EReportType::Murder;
	}
}

void UCitizenManager::GetCloserToFight(ACitizen* Citizen, ACitizen* Target, FVector MidPoint)
{
	FVector location = MidPoint;
	location += FRotator(0.0f, Camera->Grid->Stream.RandRange(0, 360), 0.0f).Vector() * Camera->Grid->Stream.RandRange(100.0f, 400.0f);

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	FNavLocation navLoc;
	nav->ProjectPointToNavigation(location, navLoc, FVector(400.0f, 400.0f, 200.0f));

	Citizen->AIController->AIMoveTo(Target, navLoc.Location);
}

void UCitizenManager::StopFighting(ACitizen* Citizen)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", Citizen);

	for (FPoliceReport& report : faction->Police.PoliceReports) {
		if (!report.Team1.HasCitizen(Citizen) && !report.Team2.HasCitizen(Citizen))
			continue;

		for (ACitizen* citizen : report.Team2.GetTeam()) {
			report.Team1.Instigator->AttackComponent->OverlappingEnemies.Remove(citizen);

			for (ACitizen* assistor : report.Team1.Assistors)
				assistor->AttackComponent->OverlappingEnemies.Remove(citizen);
		}

		for (ACitizen* citizen : report.Team1.GetTeam()) {
			report.Team2.Instigator->AttackComponent->OverlappingEnemies.Remove(citizen);

			for (ACitizen* assistor : report.Team2.Assistors)
				assistor->AttackComponent->OverlappingEnemies.Remove(citizen);
		}

		break;
	}
}

void UCitizenManager::InterrogateWitnesses(FFactionStruct Faction, ACitizen* Officer, ACitizen* Citizen)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(Faction.Name);

	for (FPoliceReport& report : faction->Police.PoliceReports) {
		if (report.RespondingOfficer != Officer)
			continue;

		float distance = *report.Witnesses.Find(Citizen);

		float impartial = 50.0f + distance / 40.0f;
		float accuseTeam1 = 0.0f;
		float accuseTeam2 = 0.0f;

		for (ACitizen* accused : report.Team1.GetTeam())
			accuseTeam1 += 50.0f * GetAggressiveness(accused);

		for (ACitizen* accused : report.Team1.GetTeam())
			accuseTeam2 += 50.0f * GetAggressiveness(accused);

		if (report.Type == EReportType::Vandalism)
			accuseTeam1 += 100.0f;

		float tally = impartial + accuseTeam1 + accuseTeam2;

		int32 choice = Camera->Grid->Stream.FRandRange(0.0f, tally);

		if (choice <= impartial)
			report.Impartial.Add(Citizen);
		else if (choice <= impartial + accuseTeam1)
			report.AcussesTeam1.Add(Citizen);
		else
			report.AcussesTeam2.Add(Citizen);

		for (auto& element : report.Witnesses) {
			if (report.Impartial.Contains(element.Key) || report.AcussesTeam1.Contains(element.Key) || report.AcussesTeam2.Contains(element.Key))
				continue;

			Officer->AIController->AIMoveTo(element.Key);

			return;
		}

		if (report.Type == EReportType::Protest || ((report.Impartial.Num() < report.AcussesTeam1.Num() || report.Impartial.Num() < report.AcussesTeam2.Num()) && (report.AcussesTeam1.Num() > report.AcussesTeam2.Num() + 1 || report.AcussesTeam2.Num() > report.AcussesTeam1.Num() + 1)))
			GotoClosestWantedMan(Officer);

		for (auto& element : report.Witnesses)
			element.Key->AIController->DefaultAction();

		break;
	}
}

void UCitizenManager::GotoClosestWantedMan(ACitizen* Officer)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", Officer);

	for (FPoliceReport report : faction->Police.PoliceReports) {
		if (report.RespondingOfficer != Officer)
			continue;

		TArray<ACitizen*> wanted;

		if (report.AcussesTeam1.Num() >= report.AcussesTeam2.Num())
			wanted = report.Team1.GetTeam();
		else
			wanted = report.Team2.GetTeam();

		ACitizen* target = nullptr;

		for (ACitizen* criminal : wanted) {
			if (faction->Police.Arrested.Contains(criminal))
				continue;

			if (!IsValid(target)) {
				target = criminal;

				continue;
			}

			double magnitude = Officer->AIController->GetClosestActor(Officer->Range, Camera->GetTargetActorLocation(Officer), Camera->GetTargetActorLocation(target), Camera->GetTargetActorLocation(criminal));

			if (magnitude < 0.0f)
				continue;

			target = criminal;
		}

		if (IsValid(target)) {
			Officer->AIController->AIMoveTo(target);
		}
		else {
			faction->Police.PoliceReports.Remove(report);

			Camera->Grid->AIVisualiser->ToggleOfficerLights(Officer, 0.0f);

			Officer->AIController->DefaultAction();
		}
	}
}

void UCitizenManager::Arrest(ACitizen* Officer, ACitizen* Citizen)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", Citizen);

	Infectible.Remove(Citizen);

	if (Infected.Contains(Citizen) || Injured.Contains(Citizen))
		Cure(Citizen);

	if (IsValid(Citizen->Building.Employment))
		Citizen->Building.Employment->RemoveCitizen(Citizen);

	if (IsValid(Citizen->Building.House)) {
		ACitizen* occupant = Citizen->Building.House->GetOccupant(Citizen);
		if (occupant != Citizen)
			Citizen->Building.House->RemoveVisitor(occupant, Citizen);
		else
			Citizen->Building.House->RemoveCitizen(Citizen);
	}
	
	if (IsValid(Citizen->Building.Orphanage))
		Citizen->Building.Orphanage->RemoveVisitor(Citizen->Building.Orphanage->GetOccupant(Citizen), Citizen);
	
	if (IsValid(Citizen->Building.School))
		Citizen->Building.School->RemoveVisitor(Citizen->Building.Orphanage->GetOccupant(Citizen), Citizen);

	Officer->AIController->StopMovement();
	Citizen->AIController->StopMovement();

	for (FPoliceReport report : faction->Police.PoliceReports) {
		if (report.RespondingOfficer != Officer)
			continue;

		FString law = "Fighting Length";

		if (report.Type == EReportType::Murder)
			law = "Murder Length";
		else if (report.Type == EReportType::Vandalism)
			law = "Vandalism Length";
		else if (report.Type == EReportType::Protest)
			law = "Protest Length";

		if (GetLawValue(faction->Name, law) == 0) {
			faction->Police.PoliceReports.Remove(report);

			Officer->AIController->DefaultAction();
			Citizen->AIController->DefaultAction();

			return;
		}
	}

	UNiagaraFunctionLibrary::SpawnSystemAttached(ArrestSystem, Citizen->GetRootComponent(), "", FVector::Zero(), FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true, false);

	TArray<FTimerParameterStruct> params;
	SetParameter(*faction, params);
	SetParameter(Officer, params);
	SetParameter(Citizen, params);

	CreateTimer("Arrest", Citizen, 2.0f, "SetInNearestJail", params, false);
}

void UCitizenManager::SetInNearestJail(FFactionStruct Faction, ACitizen* Officer, ACitizen* Citizen)
{	
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(Faction.Name);

	ABuilding* target = nullptr;

	for (ABuilding* building : faction->Buildings) {
		if (!building->IsA(PoliceStationClass))
			continue;

		if (!IsValid(target)) {
			target = building;

			continue;
		}

		double magnitude = Citizen->AIController->GetClosestActor(20.0f, Camera->GetTargetActorLocation(Citizen), target->GetActorLocation(), building->GetActorLocation());

		if (magnitude < 0.0f)
			continue;

		target = building;
	}

	Citizen->MovementComponent->Transform.SetLocation(target->GetActorLocation());

	Citizen->Building.BuildingAt = target;

	Citizen->AIController->ChosenBuilding = target;
	Citizen->AIController->Idle(faction, Citizen);

	FPartyStruct* party = GetMembersParty(Citizen);

	if (party != nullptr) {
		if (party->Leader == Citizen)
			SelectNewLeader(party);

		party->Members.Remove(Citizen);
	}

	if (faction->Politics.Representatives.Contains(Citizen))
		faction->Politics.Representatives.Remove(Citizen);

	if (!IsValid(Officer))
		return;

	for (FPoliceReport report : faction->Police.PoliceReports) {
		if (report.RespondingOfficer != Officer)
			continue;

		FString law = "Fighting Length";

		if (report.Type == EReportType::Murder)
			law = "Murder Length";
		else if (report.Type == EReportType::Vandalism)
			law = "Vandalism Length";
		else if (report.Type == EReportType::Protest)
			law = "Protest Length";

		faction->Police.Arrested.Add(Citizen, GetLawValue(faction->Name, law));

		break;
	}

	GotoClosestWantedMan(Officer);
}

void UCitizenManager::ItterateThroughSentences()
{
	for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
		TArray<ACitizen*> served;

		for (auto& element : faction.Police.Arrested) {
			element.Value--;

			if (element.Value == 0)
				served.Add(element.Key);
		}

		Camera->ResourceManager->AddUniversalResource(&faction, Camera->ResourceManager->Money, faction.Police.Arrested.Num());

		for (ACitizen* citizen : served) {
			faction.Police.Arrested.Remove(citizen);
			Infectible.Add(citizen);

			citizen->MovementComponent->Transform.SetLocation(citizen->Building.BuildingAt->BuildingMesh->GetSocketLocation("Entrance"));
		}
	}
}

void UCitizenManager::CeaseAllInternalFighting(FFactionStruct* Faction)
{
	for (ACitizen* citizen : Faction->Citizens) {
		if (citizen->AttackComponent->OverlappingEnemies.IsEmpty()) {
			citizen->AIController->DefaultAction();

			continue;
		}

		for (int32 i = citizen->AttackComponent->OverlappingEnemies.Num() - 1; i > -1; i--) {
			AActor* actor = citizen->AttackComponent->OverlappingEnemies[i];

			if (Faction->Citizens.Contains(actor))
				citizen->AttackComponent->OverlappingEnemies.RemoveAt(i);
		}
	}
}

int32 UCitizenManager::GetPoliceReportIndex(ACitizen* Citizen)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", Citizen);

	for (int32 i = 0; i < faction->Police.PoliceReports.Num(); i++) {
		if (!faction->Police.PoliceReports[i].Contains(Citizen))
			continue;

		return i;
	}

	return INDEX_NONE;
}

//
// Health
//
void UCitizenManager::StartDiseaseTimer()
{
	int32 timeToCompleteDay = 360 / (24 * Camera->Grid->AtmosphereComponent->Speed);

	CreateTimer("Disease", Camera, Camera->Grid->Stream.RandRange(timeToCompleteDay / 2, timeToCompleteDay * 3), "SpawnDisease", {}, false);
}

void UCitizenManager::SpawnDisease()
{
	int32 index = Camera->Grid->Stream.RandRange(0, Infectible.Num() - 1);
	ACitizen* citizen = Infectible[index];

	index = Camera->Grid->Stream.RandRange(0, Diseases.Num() - 1);
	citizen->HealthIssues.Add(Diseases[index]);

	Camera->NotifyLog("Bad", citizen->BioStruct.Name + " is infected with " + Diseases[index].Name, Camera->ConquestManager->GetCitizenFaction(citizen).Name);

	Infect(citizen);

	StartDiseaseTimer();
}

void UCitizenManager::Infect(ACitizen* Citizen)
{
	Async(EAsyncExecution::TaskGraphMainTick, [this, Citizen]() {
		Infected.Add(Citizen);

		UpdateHealthText(Citizen);
	});
}

void UCitizenManager::Injure(ACitizen* Citizen, int32 Odds)
{
	int32 index = Camera->Grid->Stream.RandRange(1, 10);

	if (index < Odds)
		return;

	TArray<FConditionStruct> conditions = Injuries;

	for (FConditionStruct condition : Citizen->HealthIssues)
		conditions.Remove(condition);

	index = Camera->Grid->Stream.RandRange(0, conditions.Num() - 1);
	Citizen->HealthIssues.Add(conditions[index]);

	for (FAffectStruct affect : conditions[index].Affects) {
		if (affect.Affect == EAffect::Movement)
			Citizen->ApplyToMultiplier("Speed", affect.Amount);
		else if (affect.Affect == EAffect::Damage)
			Citizen->ApplyToMultiplier("Damage", affect.Amount);
		else
			Citizen->ApplyToMultiplier("Health", affect.Amount);
	}

	Camera->NotifyLog("Bad", Citizen->BioStruct.Name + " is injured with " + conditions[index].Name, Camera->ConquestManager->GetCitizenFaction(Citizen).Name);

	Injured.Add(Citizen);

	UpdateHealthText(Citizen);
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

	Infected.Remove(Citizen);
	Injured.Remove(Citizen);

	Camera->NotifyLog("Good", Citizen->BioStruct.Name + " has been healed", Camera->ConquestManager->GetCitizenFaction(Citizen).Name);

	UpdateHealthText(Citizen);
}

void UCitizenManager::UpdateHealthText(ACitizen* Citizen)
{
	if (Camera->WidgetComponent->IsAttachedTo(Citizen->GetRootComponent()))
		Async(EAsyncExecution::TaskGraphMainTick, [this, Citizen]() { Camera->UpdateHealthIssues(); });
}

TArray<ACitizen*> UCitizenManager::GetAvailableHealers(FFactionStruct* Faction, TArray<ACitizen*>& Ill, ACitizen* Target)
{
	TArray<ACitizen*> healers;

	for (ABuilding* building : Faction->Buildings) {
		if (!building->IsA<AClinic>())
			continue;

		AClinic* clinic = Cast<AClinic>(building);

		for (ACitizen* citizen : clinic->GetOccupied()) {
			if (!clinic->IsWorking(citizen) || Camera->ConquestManager->IsCitizenInAnArmy(citizen))
				continue;

			AActor* goal = citizen->AIController->MoveRequest.GetGoalActor();
			if (goal->IsA<ACitizen>() && Ill.Contains(Cast<ACitizen>(goal)))
				Ill.Remove(Cast<ACitizen>(goal));

			if (IsValid(Target) && Target != citizen)
				continue;

			healers.Add(citizen);
		}
	}

	return healers;
}

void UCitizenManager::PairCitizenToHealer(FFactionStruct* Faction, ACitizen* Healer)
{
	TArray<ACitizen*> ill;
	ill.Append(Infected);
	ill.Append(Injured);

	TArray<ACitizen*> healers;
	healers.Append(GetAvailableHealers(Faction, ill, Healer));

	for (ACitizen* healer : healers) {
		ACitizen* chosenPatient = nullptr;
		FVector location = Camera->GetTargetActorLocation(healer);

		for (ACitizen* citizen : ill) {
			if (!citizen->AIController->CanMoveTo(location))
				continue;

			if (!IsValid(chosenPatient)) {
				chosenPatient = citizen;

				continue;
			}

			int32 curValue = Infected.Contains(chosenPatient) ? 3.0f : 1.0f;
			int32 newValue = Infected.Contains(citizen) ? 3.0f : 1.0f;

			double magnitude = healer->AIController->GetClosestActor(50.0f, Camera->GetTargetActorLocation(healer), Camera->GetTargetActorLocation(chosenPatient), Camera->GetTargetActorLocation(citizen), true, curValue, newValue);

			if (magnitude > 0.0f)
				chosenPatient = citizen;
		}

		if (IsValid(chosenPatient)) {
			healer->AIController->AIMoveTo(chosenPatient);

			ill.Remove(chosenPatient);
		}
		else
			healer->AIController->DefaultAction();
	}
}

//
// Event
//
void UCitizenManager::CreateEvent(FString FactionName, EEventType Type, TSubclassOf<class ABuilding> Building, class ABuilding* Venue, FString Period, int32 Day, TArray<int32> Hours, bool bRecurring, TArray<ACitizen*> Whitelist, bool bFireFestival)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);
	
	FEventStruct event;
	event.Type = Type;
	event.Venue = Venue;
	event.Period = Period;
	event.Day = Day;
	event.Hours = Hours;
	event.bRecurring = bRecurring;
	event.bFireFestival = bFireFestival;
	event.Building = Building;
	event.Whitelist = Whitelist;

	if (event.Type == EEventType::Protest) {
		if (faction->Buildings.IsEmpty())
			return;

		ABuilding* building = faction->Buildings[Camera->Grid->Stream.RandRange(0, faction->Buildings.Num() - 1)];

		UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
		const ANavigationData* navData = nav->GetDefaultNavDataInstance();

		FNavLocation location;
		nav->GetRandomPointInNavigableRadius(building->GetActorLocation(), 400, location);

		event.Location = location;
	}

	faction->Events.Add(event);
}

void UCitizenManager::ExecuteEvent(FString Period, int32 Day, int32 Hour)
{
	for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
		for (FEventStruct& event : faction.Events) {
			FString command = "";

			if (event.Period != "" && event.Day != 0 && event.Period != Period && event.Day != Day)
				continue;

			if (event.Hours.Contains(Hour) && !event.bStarted)
				command = "start";
			else if (event.Hours.Contains(Hour) && event.bStarted)
				command = "end";

			event.Hours.Remove(Hour);

			if (command == "")
				continue;

			if (command == "start")
				StartEvent(&faction, &event, Hour);
			else
				EndEvent(&faction, &event, Hour);
		}
	}
}

bool UCitizenManager::IsAttendingEvent(ACitizen* Citizen)
{
	if (Camera->ConquestManager->IsCitizenInAnArmy(Citizen))
		return true;

	for (auto &element : OngoingEvents())
		for (FEventStruct* event : element.Value)
			if (event->Attendees.Contains(Citizen))
				return true;

	return false;
}

void UCitizenManager::RemoveFromEvent(ACitizen* Citizen)
{
	for (auto& element : OngoingEvents()) {
		for (FEventStruct* event : element.Value) {
			if (!event->Attendees.Contains(Citizen))
				continue;

			event->Attendees.Remove(Citizen);

			return;
		}
	}
}

TMap<FFactionStruct*, TArray<FEventStruct*>> UCitizenManager::OngoingEvents()
{
	TMap<FFactionStruct*, TArray<FEventStruct*>> factionEvents;
	
	for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
		for (FEventStruct& event : faction.Events) {
			UAtmosphereComponent* atmosphere = Camera->Grid->AtmosphereComponent;

			if (!event.bStarted)
				continue;

			if (factionEvents.Contains(&faction)) {
				TArray<FEventStruct*>* events = factionEvents.Find(&faction);
				events->Add(&event);
			}
			else
				factionEvents.Add(&faction, { &event });
		}
	}

	return factionEvents;
}

void UCitizenManager::GotoEvent(ACitizen* Citizen, FEventStruct* Event, FFactionStruct* Faction)
{
	if (IsAttendingEvent(Citizen) || Citizen->bSleep || (Event->Type != EEventType::Protest && IsValid(Citizen->Building.Employment) && !Citizen->Building.Employment->bCanAttendEvents && Citizen->Building.Employment->IsWorking(Citizen)) || (Event->Type == EEventType::Mass && Cast<ABooster>(Event->Building->GetDefaultObject())->DoesPromoteFavouringValues(Citizen) && Citizen->BioStruct.Age >= 18))
		return;

	if (Event->Type == EEventType::Holliday || Event->Type == EEventType::Festival) {
		Citizen->SetHolliday(true);
	}
	else if (Event->Type == EEventType::Protest) {
		if (Citizen->GetHappiness() >= 35)
			return;

		UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
		const ANavigationData* navData = nav->GetDefaultNavDataInstance();

		UNavigationPath* path = nav->FindPathToLocationSynchronously(GetWorld(), Camera->GetTargetActorLocation(Citizen), Event->Location, Citizen, Citizen->NavQueryFilter);

		Citizen->MovementComponent->SetPoints(path->PathPoints);

		return;
	}
	else if (Event->Type == EEventType::Mass) {
		if (!IsValid(Citizen->Building.Employment) || !IsValid(Citizen->Building.BuildingAt) || Citizen->Building.Employment->IsA(Event->Building)) {
			Citizen->AIController->AIMoveTo(Citizen->Building.Employment);

			Event->Attendees.Add(Citizen);

			return;
		}

		ABooster* church = Cast<ABooster>(Event->Building->GetDefaultObject());

		TArray<FString> faiths;
		church->BuildingsToBoost.GenerateValueArray(faiths);

		bool validReligion = (Citizen->Spirituality.Faith == faiths[0]);

		if (Citizen->BioStruct.Age < 18 && (Citizen->BioStruct.Mother != nullptr && faiths[0] != Citizen->BioStruct.Mother->Spirituality.Faith || Citizen->BioStruct.Father != nullptr && faiths[0] != Citizen->BioStruct.Father->Spirituality.Faith))
			validReligion = true;

		if (!validReligion)
			return;
	}

	ABuilding* chosenBuilding = nullptr;
	ACitizen* chosenOccupant = nullptr;

	TArray<AActor*> actors;

	if (IsValid(Event->Building)) {
		if (Faction == nullptr)
			Faction = Camera->ConquestManager->GetFaction("", Citizen);

		for (ABuilding* building : Faction->Buildings)
			if (building->IsA(Event->Building))
				actors.Add(building);
	}
	else if (IsValid(Event->Venue))
		actors.Add(Event->Venue);

	for (AActor* actor : actors) {
		ABuilding* building = Cast<ABuilding>(actor);
		ACitizen* occupant = nullptr;

		if (!Citizen->AIController->CanMoveTo(building->GetActorLocation()) || (Event->Type == EEventType::Mass && building->GetOccupied().IsEmpty()) || (Event->Type == EEventType::Festival && !Cast<AFestival>(building)->bCanHostFestival))
			continue;

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

		if (chosenBuilding == nullptr) {
			chosenBuilding = building;
			chosenOccupant = occupant;

			continue;
		}

		double magnitude = Citizen->AIController->GetClosestActor(400.0f, Camera->GetTargetActorLocation(Citizen), chosenBuilding->GetActorLocation(), building->GetActorLocation());

		if (magnitude <= 0.0f)
			continue;

		chosenBuilding = building;
		chosenOccupant = occupant;
	}

	if (chosenBuilding != nullptr) {
		Event->Attendees.Add(Citizen);

		chosenBuilding->AddVisitor(chosenOccupant, Citizen);

		Citizen->AIController->AIMoveTo(chosenBuilding);
	}
}

void UCitizenManager::StartEvent(FFactionStruct* Faction, FEventStruct* Event, int32 Hour)
{
	Event->bStarted = true;
	
	if (Event->Type != EEventType::Protest) {
		for (ABuilding* building : Faction->Buildings) {
			if (building->IsA<AWork>())
				Cast<AWork>(building)->CheckWorkStatus(Hour);
			else if (building->IsA<AFestival>() && Event->Type == EEventType::Festival)
				Cast<AFestival>(building)->StartFestival(Event->bFireFestival);
		}
	}

	TArray<ACitizen*> citizens = Faction->Citizens;
	if (!Event->Whitelist.IsEmpty())
		citizens = Event->Whitelist;
	
	for (ACitizen* citizen : citizens)
		GotoEvent(citizen, Event, Faction);

	if (Event->Type == EEventType::Protest && GetLawValue(Faction->Name, "Protest Length") > 0) {
		FPoliceReport report;
		report.Type = EReportType::Protest;
		report.Team1.Instigator = Event->Attendees[0];
		report.Team1.Assistors = Event->Attendees;
		report.Team1.Assistors.RemoveAt(0);

		Faction->Police.PoliceReports.Add(report);
	}

	Camera->DisplayEvent("Event", EnumToString<EEventType>(Event->Type));
}

void UCitizenManager::EndEvent(FFactionStruct* Faction, FEventStruct* Event, int32 Hour)
{
	Event->Attendees.Empty();
	Event->bStarted = false;

	if (Event->Type == EEventType::Holliday || Event->Type == EEventType::Festival) {
		for (ABuilding* building : Faction->Buildings) {
			if (building->IsA<AWork>())
				Cast<AWork>(building)->CheckWorkStatus(Hour);
			else if (building->IsA<AFestival>() && Event->Type == EEventType::Festival)
				Cast<AFestival>(building)->StopFestival();
		}
	}

	for (ACitizen* citizen : Faction->Citizens) {
		if (Event->Type == EEventType::Holliday || Event->Type == EEventType::Festival) {
			citizen->SetHolliday(false);

			if (Event->Type == EEventType::Festival) {
				if (IsValid(citizen->Building.BuildingAt) && citizen->Building.BuildingAt->IsA(Event->Building))
					citizen->SetAttendStatus(EAttendStatus::Attended, false);
				else
					citizen->SetAttendStatus(EAttendStatus::Missed, false);
			}
		}
		else if (Event->Type == EEventType::Mass) {
			ABooster* church = Cast<ABooster>(Event->Building->GetDefaultObject());

			TArray<FString> faiths;
			church->BuildingsToBoost.GenerateValueArray(faiths);

			if (faiths[0] != citizen->Spirituality.Faith)
				continue;

			if (citizen->bWorshipping)
				citizen->SetAttendStatus(EAttendStatus::Attended, true);
			else
				citizen->SetAttendStatus(EAttendStatus::Missed, true);
		}

		if (!IsValid(Event->Building))
			citizen->AIController->DefaultAction();
	}

	if (!Event->bRecurring && Event->Hours.IsEmpty())
		Faction->Events.Remove(*Event);
}

bool UCitizenManager::UpcomingProtest(FFactionStruct* Faction)
{
	for (FEventStruct& event : Faction->Events) {
		if (event.Type != EEventType::Protest)
			continue;

		return true;
	}

	return false;
}

//
// Politics
//
FPartyStruct* UCitizenManager::GetMembersParty(ACitizen* Citizen)
{
	FPartyStruct* partyStruct = nullptr;
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", Citizen);

	if (faction != nullptr) {
		for (FPartyStruct& party : faction->Politics.Parties) {
			if (!party.Members.Contains(Citizen))
				continue;

			partyStruct = &party;

			break;
		}
	}

	return partyStruct;
}

FString UCitizenManager::GetCitizenParty(ACitizen* Citizen)
{
	FPartyStruct* partyStruct = GetMembersParty(Citizen);

	if (partyStruct == nullptr)
		return "Undecided";

	return partyStruct->Party;
}

void UCitizenManager::SelectNewLeader(FPartyStruct* Party)
{
	TArray<ACitizen*> candidates;

	for (auto &element : Party->Members) {
		if (!IsValid(element.Key) || element.Key->bHasBeenLeader)
			continue;

		if (candidates.Num() < 3)
			candidates.Add(element.Key);
		else {
			ACitizen* lowest = nullptr;

			for (ACitizen* candidate : candidates)
				if (lowest == nullptr || Party->Members.Find(lowest) > Party->Members.Find(candidate))
					lowest = candidate;

			if (Party->Members.Find(element.Key) > Party->Members.Find(lowest)) {
				candidates.Remove(lowest);

				candidates.Add(element.Key);
			}
		}
	}

	if (candidates.IsEmpty())
		return;

	auto value = Async(EAsyncExecution::TaskGraph, [this, candidates]() { return Camera->Grid->Stream.RandRange(0, candidates.Num() - 1); });

	ACitizen* chosen = candidates[value.Get()];

	Party->Leader = chosen;

	chosen->bHasBeenLeader = true;
	Party->Members.Emplace(chosen, ESway::Radical);
}

void UCitizenManager::StartElectionTimer(FFactionStruct* Faction)
{
	RemoveTimer(Faction->Name + " Election", GetOwner());
	
	int32 timeToCompleteDay = 360 / (24 * Camera->Grid->AtmosphereComponent->Speed); 
	
	TArray<FTimerParameterStruct> params;
	SetParameter(*Faction, params);

	CreateTimer(Faction->Name + " Election", Camera, timeToCompleteDay * GetLawValue(Faction->Name, "Election Timer"), "Election", params, false);
}

void UCitizenManager::Election(FFactionStruct Faction)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(Faction.Name);

	faction->Politics.Representatives.Empty();

	TMap<FString, TArray<ACitizen*>> tally;

	int32 representativeType = GetLawValue(faction->Name, "Representative Type");
	int32 representatives = GetLawValue(faction->Name, "Representatives");

	for (FPartyStruct party : faction->Politics.Parties) {
		TArray<ACitizen*> citizens;

		for (TPair<ACitizen*, TEnumAsByte<ESway>> pair : party.Members) {
			if ((representativeType == 1 && pair.Key->Building.Employment == nullptr) || (representativeType == 2 && pair.Key->Balance < 15))
				continue;

			citizens.Add(pair.Key);
		}

		tally.Add(party.Party, citizens);
	}

	for (TPair<FString, TArray<ACitizen*>>& pair : tally) {
		int32 number = FMath::RoundHalfFromZero(pair.Value.Num() / (float)faction->Citizens.Num() * 100.0f / representatives);

		if (number == 0 || faction->Politics.Representatives.Num() == representatives)
			continue;

		number -= 1;

		FPartyStruct partyStruct;
		partyStruct.Party = pair.Key;

		int32 index = faction->Politics.Parties.Find(partyStruct);

		FPartyStruct* party = &faction->Politics.Parties[index];

		faction->Politics.Representatives.Add(party->Leader);

		pair.Value.Remove(party->Leader);

		for (int32 i = 0; i < number; i++) {
			if (pair.Value.IsEmpty())
				continue;

			auto value = Async(EAsyncExecution::TaskGraph, [this, pair]() { return Camera->Grid->Stream.RandRange(0, pair.Value.Num() - 1); });

			ACitizen* citizen = pair.Value[value.Get()];

			faction->Politics.Representatives.Add(citizen);

			pair.Value.Remove(citizen);

			if (faction->Politics.Representatives.Num() == representatives)
				break;
		}
	}

	StartElectionTimer(faction);
}

void UCitizenManager::Bribe(class ACitizen* Representative, bool bAgree)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", Representative);

	if (faction->Politics.BribeValue.IsEmpty())
		return;

	int32 index = faction->Politics.Representatives.Find(Representative);

	int32 bribe = faction->Politics.BribeValue[index];

	bool bPass = Camera->ResourceManager->TakeUniversalResource(faction, Camera->ResourceManager->Money, bribe, 0);

	if (!bPass) {
		Camera->ShowWarning("Cannot afford");

		return;
	}

	if (faction->Politics.Votes.For.Contains(Representative))
		faction->Politics.Votes.For.Remove(Representative);
	else if (faction->Politics.Votes.Against.Contains(Representative))
		faction->Politics.Votes.For.Remove(Representative);

	Representative->Balance += bribe;

	if (bAgree)
		faction->Politics.Votes.For.Add(Representative);
	else
		faction->Politics.Votes.Against.Add(Representative);
}

void UCitizenManager::ProposeBill(FString FactionName, FLawStruct Bill)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	int32 index = faction->Politics.Laws.Find(Bill);

	if (faction->Politics.Laws[index].Cooldown != 0) {
		FString string = "You must wait " + faction->Politics.Laws[index].Cooldown;

		Camera->ShowWarning(string + " seconds");

		return;
	}

	faction->Politics.ProposedBills.Add(Bill);

	int32 timeToCompleteDay = 360 / (24 * Camera->Grid->AtmosphereComponent->Speed);

	faction->Politics.Laws[index].Cooldown = timeToCompleteDay;

	Camera->DisplayNewBill();

	if (faction->Politics.ProposedBills.Num() > 1)
		return;

	SetupBill(faction);
}

void UCitizenManager::SetElectionBillLeans(FFactionStruct* Faction, FLawStruct* Bill)
{
	if (Bill->BillType != "Election")
		return;

	for (FPartyStruct party : Faction->Politics.Parties) {
		int32 representativeCount = 0;

		for (ACitizen* citizen : Faction->Politics.Representatives)
			if (party.Members.Contains(citizen))
				representativeCount++;

		float representPerc = representativeCount / Faction->Citizens.Num() * 100.0f;
		float partyPerc = party.Members.Num() / Faction->Citizens.Num() * 100.0f;

		FLeanStruct lean;
		lean.Party = party.Party;

		if (partyPerc > representPerc)
			lean.ForRange.Append({ 0, 0 });
		else if (representPerc > partyPerc)
			lean.AgainstRange.Append({ 0, 0 });

		Bill->Lean.Add(lean);
	}
}

void UCitizenManager::SetupBill(FFactionStruct* Faction)
{
	Faction->Politics.Votes.Clear();

	Faction->Politics.BribeValue.Empty();

	if (Faction->Politics.ProposedBills.IsEmpty())
		return;

	SetElectionBillLeans(Faction, &Faction->Politics.ProposedBills[0]);

	for (ACitizen* citizen : Faction->Politics.Representatives)
		GetVerdict(Faction, citizen, Faction->Politics.ProposedBills[0], true, false);

	for (ACitizen* citizen : Faction->Politics.Representatives) {
		int32 bribe = Async(EAsyncExecution::TaskGraph, [this]() { return Camera->Grid->Stream.RandRange(2, 20); }).Get();

		if (Faction->Politics.Votes.For.Contains(citizen) || Faction->Politics.Votes.Against.Contains(citizen))
			bribe *= 4;

		bribe *= (uint8)*GetMembersParty(citizen)->Members.Find(citizen);

		Faction->Politics.BribeValue.Add(bribe);
	}

	TArray<FTimerParameterStruct> params;
	SetParameter(*Faction, params);
	SetParameter(Faction->Politics.ProposedBills[0], params);

	CreateTimer("Bill", Camera, 60, "MotionBill", params, false);
}

void UCitizenManager::MotionBill(FFactionStruct Faction, FLawStruct Bill)
{
	Async(EAsyncExecution::TaskGraphMainTick, [this, Faction, Bill]() {
		FFactionStruct* faction = Camera->ConquestManager->GetFaction(Faction.Name);

		int32 count = 1;

		for (ACitizen* citizen : faction->Politics.Representatives) {
			if (faction->Politics.Votes.For.Contains(citizen) || faction->Politics.Votes.Against.Contains(citizen))
				continue;

			FTimerHandle verdictTimer;
			GetWorld()->GetTimerManager().SetTimer(verdictTimer, FTimerDelegate::CreateUObject(this, &UCitizenManager::GetVerdict, faction, citizen, Bill, false, false), 0.1f * count, false);

			count++;
		}

		FTimerHandle tallyTimer;
		GetWorld()->GetTimerManager().SetTimer(tallyTimer, FTimerDelegate::CreateUObject(this, &UCitizenManager::TallyVotes, faction, Bill), 0.1f * count + 0.1f, false);
	});
}

bool UCitizenManager::IsInRange(TArray<int32> Range, int32 Value)
{
	if (Value == 255 || (Range[0] > Range[1] && Value >= Range[0] || Value <= Range[1]) || (Range[0] <= Range[1] && Value >= Range[0] && Value <= Range[1]))
		return true;

	return false;
}

void UCitizenManager::GetVerdict(FFactionStruct* Faction, ACitizen* Representative, FLawStruct Bill, bool bCanAbstain, bool bPrediction)
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

	;

	if (Bill.BillType.Contains("Cost")) {
		int32 leftoverMoney = 0;

		if (IsValid(Representative->Building.Employment))
			leftoverMoney += Representative->Building.Employment->GetWage(Representative);

		if (IsValid(Representative->Building.House))
			leftoverMoney -= Representative->Building.House->Rent;

		if (leftoverMoney < Bill.Value)
			verdict.Append({ "Opposing", "Opposing", "Opposing" });
	}
	else if (Bill.BillType.Contains("Age")) {
		index = Faction->Politics.Laws.Find(Bill);

		if (Faction->Politics.Laws[index].Value <= Representative->BioStruct.Age) {
			if (Bill.Value > Representative->BioStruct.Age)
				verdict.Append({ "Opposing", "Opposing", "Opposing" });
			else
				verdict.Append({ "Abstaining", "Abstaining", "Abstaining" });
		}
		else if (Bill.Value <= Representative->BioStruct.Age)
			verdict.Append({ "Agreeing", "Agreeing", "Agreeing" });
	}
	else if (Bill.BillType == "Representative Type") {
		if (Bill.Value == 1 && !IsValid(Representative->Building.Employment))
			verdict.Append({ "Opposing", "Opposing", "Opposing" });
		else if (Bill.Value == 2 && Representative->Balance < 15)
			verdict.Append({ "Opposing", "Opposing", "Opposing" });
	}

	auto value = Async(EAsyncExecution::TaskGraph, [this, verdict]() { return Camera->Grid->Stream.RandRange(0, verdict.Num() - 1); });

	FString result = verdict[value.Get()];

	if (bPrediction) {
		if (result == "Agreeing")
			Faction->Politics.Predictions.For.Add(Representative);
		else if (result == "Opposing")
			Faction->Politics.Predictions.Against.Add(Representative);
	}
	else {
		if (result == "Agreeing")
			Faction->Politics.Votes.For.Add(Representative);
		else if (result == "Opposing")
			Faction->Politics.Votes.Against.Add(Representative);
	}
}

void UCitizenManager::TallyVotes(FFactionStruct* Faction, FLawStruct Bill)
{
	bool bPassed = false;

	if (Faction->Politics.Votes.For.Num() > Faction->Politics.Votes.Against.Num()) {
		int32 index = Faction->Politics.Laws.Find(Bill);

		if (Faction->Politics.Laws[index].BillType == "Abolish") {
			TArray<FTimerParameterStruct> params;
			SetParameter(1000, params);
			SetParameter(Camera, params);

			CreateTimer("Abolish", Faction->EggTimer, 6.0f, "TakeHealth", params, false);
		}
		else if (Faction->Politics.Laws[index].BillType == "Election") {
			Election(*Faction);
		}
		else {
			Faction->Politics.Laws[index].Value = Bill.Value;

			if (Faction->Politics.Laws[index].BillType.Contains("Age")) {
				for (ACitizen* citizen : Faction->Citizens) {
					if (IsValid(citizen->Building.Employment) && !citizen->CanWork(citizen->Building.Employment))
						citizen->Building.Employment->RemoveCitizen(citizen);

					if (IsValid(citizen->Building.School) && (citizen->BioStruct.Age >= GetLawValue(Faction->Name, "Work Age") || citizen->BioStruct.Age < GetLawValue(Faction->Name, "Education Age")))
						citizen->Building.School->RemoveVisitor(citizen->Building.School->GetOccupant(citizen), citizen);

					if (citizen->BioStruct.Age >= Faction->Politics.Laws[index].Value)
						continue;

					FPartyStruct* party = GetMembersParty(citizen);

					if (party == nullptr)
						continue;

					party->Members.Remove(citizen);
				}
			}
		}

		if (Faction->Politics.Laws[index].BillType == "Representatives" && Camera->ParliamentUIInstance->IsInViewport())
			Camera->RefreshRepresentatives();

		bPassed = true;
	}

	if (Faction->Name == Camera->ColonyName) {
		FDescriptionStruct descriptionStruct;

		for (FDescriptionStruct desc : Bill.Description) {
			if (desc.Min > Bill.Value || desc.Max < Bill.Value)
				continue;

			descriptionStruct = desc;

			break;
		}

		FString value = FString::FromInt(Bill.Value);
		FString description = descriptionStruct.Description.Replace(TEXT("X"), *value);

		Camera->LawPassed(bPassed, description, Faction->Politics.Votes.For.Num(), Faction->Politics.Votes.Against.Num());

		Camera->LawPassedUIInstance->AddToViewport();
	}

	Faction->Politics.ProposedBills.Remove(Bill);

	SetupBill(Faction);
}

FString UCitizenManager::GetBillPassChance(FString FactionName, FLawStruct Bill)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	faction->Politics.Predictions.Clear();

	TArray<int32> results = { 0, 0, 0 };
	
	SetElectionBillLeans(faction, &Bill);

	for (int32 i = 0; i < 10; i++) {
		for (ACitizen* citizen : faction->Politics.Representatives)
			GetVerdict(faction, citizen, Bill, true, true);

		int32 abstainers = (faction->Politics.Representatives.Num() - faction->Politics.Predictions.For.Num() - faction->Politics.Predictions.Against.Num());

		if (faction->Politics.Predictions.For.Num() > faction->Politics.Predictions.Against.Num() + abstainers)
			results[0]++;
		else if (faction->Politics.Predictions.Against.Num() > faction->Politics.Predictions.For.Num() + abstainers)
			results[1]++;
		else
			results[2]++;

		faction->Politics.Predictions.Clear();
	}

	if (results[0] > results[1] + results[2])
		return "High";
	else if (results[1] > results[0] + results[2])
		return "Low";
	else
		return "Random";
}

int32 UCitizenManager::GetLawValue(FString FactionName, FString BillType)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	FLawStruct lawStruct;
	lawStruct.BillType = BillType;

	int32 index = faction->Politics.Laws.Find(lawStruct);
	
	return faction->Politics.Laws[index].Value;
}

int32 UCitizenManager::GetCooldownTimer(FString FactionName, FLawStruct Law)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	int32 index = faction->Politics.Laws.Find(Law);

	if (index == INDEX_NONE)
		return 0;

	return faction->Politics.Laws[index].Cooldown;
}

void UCitizenManager::IssuePensions(int32 Hour)
{
	if (Hour != IssuePensionHour)
		return;

	for (FFactionStruct& faction : Camera->ConquestManager->Factions)
		for (ACitizen* citizen : faction.Citizens)
			if (citizen->BioStruct.Age >= GetLawValue(faction.Name, "Pension Age"))
				citizen->Balance += GetLawValue(faction.Name, "Pension");
	}

//
// Rebel
//
void UCitizenManager::Overthrow(FFactionStruct* Faction)
{
	Faction->RebelCooldownTimer = 1500;

	CeaseAllInternalFighting(Faction);

	for (ACitizen* citizen : Faction->Citizens) {
		if (GetMembersParty(citizen) == nullptr || GetMembersParty(citizen)->Party != "Shell Breakers")
			return;

		if (Faction->Politics.Representatives.Contains(citizen))
			Faction->Politics.Representatives.Remove(citizen);

		SetupRebel(Faction, citizen);
	}

	for (ASpecial* building : Camera->Grid->SpecialBuildings) {
		if (!building->IsA<ACloneLab>())
			continue;

		Cast<ACloneLab>(building)->StartCloneLab();

		break;
	}
}

void UCitizenManager::SetupRebel(FFactionStruct* Faction, ACitizen* Citizen)
{
	Citizen->Energy = 100;
	Citizen->Hunger = 100;

	UAIVisualiser* aiVisualiser = Camera->Grid->AIVisualiser;
	aiVisualiser->RemoveInstance(aiVisualiser->HISMCitizen, Faction->Citizens.Find(Citizen));
	Faction->Citizens.Remove(Citizen);

	Faction->Rebels.Add(Citizen);
	aiVisualiser->AddInstance(Citizen, aiVisualiser->HISMRebel, Citizen->MovementComponent->Transform);

	Citizen->MoveToBroch();
}

bool UCitizenManager::IsRebellion(FFactionStruct* Faction)
{
	return !Faction->Rebels.IsEmpty();
}

//
// Genetics
//
void UCitizenManager::Pray(FString FactionName)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	bool bPass = Camera->ResourceManager->TakeUniversalResource(faction, Camera->ResourceManager->Money, GetPrayCost(FactionName), 0);

	if (!bPass) {
		Camera->ShowWarning("Cannot afford");

		return;
	}

	IncrementPray(*faction, "Good", 1);

	int32 timeToCompleteDay = 360 / (24 * Camera->Grid->AtmosphereComponent->Speed);

	TArray<FTimerParameterStruct> params;
	SetParameter(*faction, params);
	SetParameter("Good", params);
	SetParameter(-1, params);

	CreateTimer("Pray", Camera, timeToCompleteDay, "IncrementPray", params, false);
}

void UCitizenManager::IncrementPray(FFactionStruct Faction, FString Type, int32 Increment)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(Faction.Name);

	if (Type == "Good")
		faction->PrayStruct.Good = FMath::Max(faction->PrayStruct.Good + Increment, 0);
	else
		faction->PrayStruct.Bad = FMath::Max(faction->PrayStruct.Bad + Increment, 0);
}

int32 UCitizenManager::GetPrayCost(FString FactionName)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	int32 cost = 50;

	for (int32 i = 0; i < faction->PrayStruct.Good; i++)
		cost *= 1.15;

	return cost;
}

void UCitizenManager::Sacrifice(FString FactionName)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	if (faction->Citizens.IsEmpty()) {
		Camera->ShowWarning("Cannot afford");

		return;
	}

	int32 index = Camera->Grid->Stream.RandRange(0, faction->Citizens.Num() - 1);
	ACitizen* citizen = faction->Citizens[index];

	RemoveTimer("Energy", citizen);
	RemoveTimer("Eat", citizen);

	citizen->AIController->StopMovement();
	citizen->MovementComponent->SetMaxSpeed(0.0f);

	UNiagaraComponent* component = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), SacrificeSystem, citizen->MovementComponent->Transform.GetLocation());

	TArray<FTimerParameterStruct> params1;
	SetParameter(1000, params1);
	SetParameter(Camera, params1);
	SetParameter(citizen->AttackComponent->OnHitSound, params1);

	CreateTimer("Sacrifice", citizen, 4.0f, "TakeHealth", params1, false);

	IncrementPray(*faction, "Bad", 1);

	int32 timeToCompleteDay = 360 / (24 * Camera->Grid->AtmosphereComponent->Speed);

	TArray<FTimerParameterStruct> params2;
	SetParameter(*faction, params2);
	SetParameter("Bad", params2);
	SetParameter(-1, params2);

	CreateTimer("Pray", Camera, timeToCompleteDay, "IncrementPray", params2, false);
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