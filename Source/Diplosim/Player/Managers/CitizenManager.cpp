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
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
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

	if (DeltaTime < 0.005f || DeltaTime > 1.0f)
		return;

	Async(EAsyncExecution::Thread, [this, DeltaTime]() {
		if (!IsValid(this))
			return;

		FScopeTryLock lock(&TickLock);
		if (!lock.IsLocked())
			return;

		Loop();

		TArray<AActor*> actors;

		for (ACitizen* citizen : Citizens) {
			if (!IsValid(this))
				return;

			if (citizen->HealthComponent->GetHealth() <= 0 || citizen->IsHidden() || Arrested.Contains(citizen))
				continue;

			actors = Camera->Grid->AIVisualiser->GetOverlaps(Camera, citizen, citizen->Range);

			int32 reach = citizen->Range / 15.0f;

			for (AActor* actor : actors) {
				if (!IsValid(this))
					return;

				UHealthComponent* healthComp = actor->GetComponentByClass<UHealthComponent>();

				if (healthComp && healthComp->GetHealth() <= 0)
					continue;

				if (actor->IsA<AAI>()) {
					AAI* ai = Cast<AAI>(actor);

					if (Citizens.Contains(ai) || Rebels.Contains(ai)) {
						ACitizen* c = Cast<ACitizen>(ai);

						if (citizen->CanReach(c, reach)) {
							if (!Infected.IsEmpty()) {
								if (IsValid(citizen->Building.Employment) && citizen->Building.Employment->IsA<AClinic>() && FindTimer("Healing", citizen) == nullptr && Citizens.Contains(c)) {
									citizen->CitizenHealing = c;

									CreateTimer("Healing", citizen, 2.0f / citizen->GetProductivity(), FTimerDelegate::CreateUObject(citizen, &ACitizen::Heal, c), false);
								}
								else if (!IsValid(c->Building.Employment) || !c->Building.Employment->IsA<AClinic>()) {
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

							if (citizen->AIController->MoveRequest.GetGoalActor() == c && citizen->Building.Employment->IsA(PoliceStationClass) && !DoesTimerExist("Interrogate", citizen)) {
								bool bInterrogate = true;

								for (FPoliceReport report : PoliceReports) {
									if (report.RespondingOfficer != citizen)
										continue;

									if (report.Witnesses.Num() == report.Impartial.Num() + report.AcussesTeam1.Num() + report.AcussesTeam2.Num())
										bInterrogate = false;

									break;
								}

								if (bInterrogate) {
									if (!c->AttackComponent->OverlappingEnemies.IsEmpty())
										StopFighting(c);

									StartConversation(citizen, c, true);
								}
								else {
									Arrest(citizen, c);
								}
							}
							else if (Citizens.Contains(c) && !IsInAPoliceReport(citizen) && Enemies.IsEmpty() && !citizen->bConversing && !citizen->bSleep && citizen->AttackComponent->OverlappingEnemies.IsEmpty() && !c->bConversing && !c->bSleep && c->AttackComponent->OverlappingEnemies.IsEmpty()) {
								int32 chance = Camera->Grid->Stream.RandRange(0, 1000);

								if (chance < 1000)
									continue;

								StartConversation(citizen, c, false);
							}
						}

						if (Enemies.IsEmpty() && Rebels.IsEmpty() && c->AttackComponent->IsComponentTickEnabled()) {
							FCollisionQueryParams params;
							params.AddIgnoredActor(citizen);

							if (IsValid(citizen->Building.BuildingAt))
								params.AddIgnoredActor(citizen->Building.BuildingAt);

							FHitResult hit(ForceInit);

							if (GetWorld()->LineTraceSingleByChannel(hit, Camera->GetTargetActorLocation(citizen), Camera->GetTargetActorLocation(c), ECollisionChannel::ECC_Visibility, params) && (hit.GetActor() == c || c->AttackComponent->OverlappingEnemies.Contains(hit.GetActor()))) {
								ACitizen* c2 = Cast<ACitizen>(c->AttackComponent->OverlappingEnemies[0]);

								int32 count1 = 0;
								int32 count2 = 0;

								float citizenAggressiveness = 0;
								float c1Aggressiveness = 0;
								float c2Aggressiveness = 0;

								PersonalityComparison(citizen, c, count1, citizenAggressiveness, c1Aggressiveness);
								PersonalityComparison(citizen, c2, count2, citizenAggressiveness, c2Aggressiveness);

								ACitizen* citizenToAttack = nullptr;

								if (citizenAggressiveness >= 1.0f) {
									if (count1 > 1 && count2 < 1)
										citizenToAttack = c2;
									else if (count1 < 1 && count2 > 1)
										citizenToAttack = c;
								}

								int32 index = GetPoliceReportIndex(c);

								if (IsValid(citizenToAttack)) {
									if (!citizen->AttackComponent->OverlappingEnemies.Contains(citizenToAttack))
										citizen->AttackComponent->OverlappingEnemies.Add(citizenToAttack);

									if (!citizenToAttack->AttackComponent->OverlappingEnemies.Contains(citizen))
										citizenToAttack->AttackComponent->OverlappingEnemies.Add(citizen);

									if (citizen->AttackComponent->OverlappingEnemies.Num() == 1)
										citizen->AttackComponent->SetComponentTickEnabled(true);

									citizen->AttackComponent->bShowMercy = citizenToAttack->AttackComponent->bShowMercy;

									if (index != INDEX_NONE) {
										if (PoliceReports[index].Team1.Instigator == citizenToAttack)
											PoliceReports[index].Team2.Assistors.Add(citizen);
										else
											PoliceReports[index].Team1.Assistors.Add(citizen);
									}
								}
								else if (!IsCarelessWitness(citizen)) {
									if (index == INDEX_NONE) {
										FPoliceReport report;
										report.Type = EReportType::Fighting;

										for (int32 i = 0; i < c->AttackComponent->OverlappingEnemies.Num(); i++) {
											ACitizen* fighter1 = Cast<ACitizen>(c->AttackComponent->OverlappingEnemies[i]);

											if (i == 0) {
												report.Team2.Instigator = fighter1;

												for (int32 j = 0; j < fighter1->AttackComponent->OverlappingEnemies.Num(); j++) {
													ACitizen* fighter2 = Cast<ACitizen>(c->AttackComponent->OverlappingEnemies[i]);

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

										PoliceReports.Add(report);

										index = PoliceReports.Num() - 1; 
									}

									float distance = FVector::Dist(Camera->GetTargetActorLocation(citizen), hit.Location);
									float dist = 1000000000;

									if (PoliceReports[index].Witnesses.Contains(citizen))
										dist = *PoliceReports[index].Witnesses.Find(citizen);

									GetCloserToFight(citizen, c, PoliceReports[index].Location);

									if (distance < dist)
										PoliceReports[index].Witnesses.Add(citizen, distance);
								}
							}
						}
					}

					if (!IsValid(this))
						return;
					
					if ((*citizen->AttackComponent->ProjectileClass || citizen->AIController->CanMoveTo(Camera->GetTargetActorLocation(ai))) && !Citizens.Contains(ai)) {
						if (!citizen->AttackComponent->OverlappingEnemies.Contains(ai))
							citizen->AttackComponent->OverlappingEnemies.Add(ai);

						if (citizen->AttackComponent->OverlappingEnemies.Num() == 1)
							citizen->AttackComponent->SetComponentTickEnabled(true);
					}
				}
				else if (citizen->AIController->MoveRequest.GetGoalActor() == actor && citizen->CanReach(actor, reach, citizen->AIController->MoveRequest.GetGoalInstance())) {
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
				else if (actor->IsA<ABuilding>() && !actor->IsA<ABroch>() && citizen->CanReach(actor, reach) && !IsValid(citizen->Building.BuildingAt)) {
					FPartyStruct* partyStruct = GetMembersParty(citizen);

					if (partyStruct == nullptr || partyStruct->Party != "Shell Breakers")
						continue;

					int32 aggressiveness = 0;
					TArray<FPersonality*> personalities = GetCitizensPersonalities(citizen);

					for (FPersonality* personality : personalities)
						aggressiveness += personality->Aggressiveness / personalities.Num();

					int32 max = (1000 + (citizen->GetHappiness() - 50) * 16) / aggressiveness;

					if (Camera->Grid->Stream.RandRange(1, max) != max)
						continue;

					Camera->Grid->AtmosphereComponent->SetOnFire(actor);

					for (AActor* a : actors) {
						if (!Citizens.Contains(a))
							continue;

						ACitizen* witness = Cast<ACitizen>(a);

						FCollisionQueryParams params;
						params.AddIgnoredActor(witness);

						FHitResult hit(ForceInit);

						if (GetWorld()->LineTraceSingleByChannel(hit, Camera->GetTargetActorLocation(witness), Camera->GetTargetActorLocation(citizen), ECollisionChannel::ECC_Visibility, params) && hit.GetActor() == citizen) {
							int32 index = GetPoliceReportIndex(citizen);

							float distance = FVector::Dist(Camera->GetTargetActorLocation(witness), hit.Location);

							if (index == INDEX_NONE) {
								FPoliceReport report;
								report.Type = EReportType::Vandalism;

								report.Team1.Instigator = citizen;

								PoliceReports.Add(report);

								index = PoliceReports.Num() - 1;
							}

							PoliceReports[index].Witnesses.Add(witness, distance);
						}
					}
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

		TArray<AAI*> ais;
		ais.Append(Clones);
		ais.Append(Rebels);
		ais.Append(Enemies);

		for (AAI* ai : ais) {
			if (ai->HealthComponent->GetHealth() <= 0)
				continue;

			actors = Camera->Grid->AIVisualiser->GetOverlaps(Camera, ai, ai->Range);

			int32 reach = ai->InitialRange / 15.0f;

			for (AActor* actor : actors) {
				UHealthComponent* healthComp = actor->GetComponentByClass<UHealthComponent>();

				if ((healthComp && healthComp->GetHealth() <= 0) || actor->IsA<AResource>() || actor->GetClass() == ai->GetClass() || (ai->IsA<AClone>() && Citizens.Contains(actor)))
					continue;

				if (!*ai->AttackComponent->ProjectileClass && !ai->AIController->CanMoveTo(Camera->GetTargetActorLocation(actor)))
					continue;

				ai->AttackComponent->OverlappingEnemies.Add(actor);

				if (ai->AttackComponent->OverlappingEnemies.Num() == 1)
					ai->AttackComponent->SetComponentTickEnabled(true);
			}

			for (int32 i = ai->AttackComponent->OverlappingEnemies.Num() - 1; i > -1; i--) {
				AActor* actor = ai->AttackComponent->OverlappingEnemies[i];

				if (actors.Contains(actor))
					continue;

				ai->AttackComponent->OverlappingEnemies.RemoveAt(i);
			}
		}

		for (AAI* ai : AIPendingRemoval) {
			if (!IsValid(ai))
				continue;

			TTuple<class UHierarchicalInstancedStaticMeshComponent*, int32> info = Camera->Grid->AIVisualiser->GetAIHISM(ai);

			Camera->Grid->AIVisualiser->RemoveInstance(info.Key, info.Value);

			if (ai->IsA<ACitizen>()) {
				ACitizen* citizen = Cast<ACitizen>(ai);

				if (Citizens.Contains(citizen))
					Citizens.Remove(citizen);
				else
					Rebels.Remove(citizen);
			}
			else if (ai->IsA<AClone>())
				Clones.Remove(ai);
			else
				Enemies.Remove(ai);
		}

		AIPendingRemoval.Empty();

		if (!Enemies.IsEmpty()) {
			for (ABuilding* building : Buildings) {
				if (!building->IsA<ACloneLab>())
					continue;

				if (!DoesTimerExist("Internal", Cast<ACloneLab>(building)) && Enemies[0]->AIController->CanMoveTo(building->GetActorLocation()))
					Cast<ACloneLab>(building)->SetTimer();

				break;
			}
		}
	});
}

void UCitizenManager::Loop()
{
	for (auto node = Timers.GetHead(); node != nullptr; node = node->GetNextNode()) {
		FTimerStruct &timer = node->GetValue();

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

		if ((timer.ID == "Harvest" || timer.ID == "Internal")) {
			TArray<ACitizen*> citizens;

			if (timer.Actor->IsA<ACitizen>())
				citizens.Add(Cast<ACitizen>(timer.Actor));
			else
				citizens = Cast<AWork>(timer.Actor)->GetCitizensAtBuilding();

			for (ACitizen* citizen : Citizens) {
				if (citizen->HarvestVisualTimer < timer.Timer || citizen->HarvestVisualResource == nullptr)
					continue;

				citizen->HarvestVisualTimer += citizen->HarvestVisualTargetTimer;

				citizen->SetHarvestVisuals(citizen->HarvestVisualResource);
			}

		}

		if (timer.Timer >= timer.Target && !timer.bModifying) {
			if (timer.bOnGameThread) {
				timer.bModifying = true;

				AsyncTask(ENamedThreads::GameThread, [this, &timer]() {
					timer.Delegate.ExecuteIfBound();
					
					timer.bDone = true;
				});
			}
			else {
				timer.bModifying = true;

				timer.Delegate.ExecuteIfBound();

				timer.bDone = true;
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

		int32 timeToCompleteDay = 360 / (24 * Camera->Grid->AtmosphereComponent->Speed);

		for (ACitizen* citizen : Citizens) {
			if (!IsValid(citizen))
				continue;

			for (FConditionStruct& condition : citizen->HealthIssues) {
				condition.Level++;

				if (condition.Level == condition.DeathLevel)
					citizen->HealthComponent->TakeHealth(1000, citizen);
			}
			
			citizen->AllocatedBuildings.Empty(); 
			citizen->AllocatedBuildings = { citizen->Building.School, citizen->Building.Employment, citizen->Building.House };

			if (citizen->CanFindAnything(timeToCompleteDay)) {
				for (ABuilding* building : Buildings) {
					if (!IsValid(building) || !citizen->AIController->CanMoveTo(building->GetActorLocation()) || citizen->AllocatedBuildings.Contains(building) || building->HealthComponent->GetHealth() == 0)
						continue;

					if (building->IsA<ASchool>())
						citizen->FindEducation(Cast<ASchool>(building), timeToCompleteDay);
					else if (building->IsA<AWork>())
						citizen->FindJob(Cast<AWork>(building), timeToCompleteDay);
					else if (building->IsA<AHouse>())
						citizen->FindHouse(Cast<AHouse>(building), timeToCompleteDay);
				}

				AsyncTask(ENamedThreads::GameThread, [citizen, timeToCompleteDay]() { citizen->SetJobHouseEducation(timeToCompleteDay); });
			}

			citizen->SetHappiness();

			if (GetMembersParty(citizen) != nullptr && GetMembersParty(citizen)->Party == "Shell Breakers")
				rebelCount++;

			if (!IsValid(citizen->Building.Employment))
				continue;

			if (citizen->Building.Employment->IsA<AWall>())
				Cast<AWall>(citizen->Building.Employment)->SetEmergency(!Enemies.IsEmpty());
			else if (citizen->Building.Employment->IsA<AClinic>())
				Cast<AClinic>(citizen->Building.Employment)->SetEmergency(!Infected.IsEmpty());
		}

		if (!PoliceReports.IsEmpty()) {
			for (int32 i = PoliceReports.Num() - 1; i > -1; i--) {
				FPoliceReport report = PoliceReports[i];

				if (IsValid(report.RespondingOfficer)) {
					if (report.Witnesses.IsEmpty()) {
						PoliceReports.RemoveAt(i);

						report.RespondingOfficer->AIController->DefaultAction();
					}

					continue;
				}

				if (report.Witnesses.IsEmpty()) {
					PoliceReports.RemoveAt(i);

					continue;
				}

				float distance = 1000;
				ACitizen* officer = nullptr;

				for (ABuilding* building : Buildings) {
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
					PoliceReports[i].RespondingOfficer = officer;

					ToggleOfficerLights(officer, 1.0f);

					officer->AIController->AIMoveTo(witnesses[0]);
				}
			}
		}

		if (!Citizens.IsEmpty() && (rebelCount / Citizens.Num()) * 100 > 33 && !IsRebellion()) {
			CooldownTimer--;

			if (CooldownTimer < 1) {
				auto value = Async(EAsyncExecution::TaskGraphMainThread, [this]() { return Camera->Grid->Stream.RandRange(1, 3); });

				if (value.Get() == 3) {
					Overthrow();
				}
				else {
					FLawStruct lawStruct;
					lawStruct.BillType = "Abolish";

					int32 index = Laws.Find(lawStruct);

					ProposeBill(Laws[index]);
				}
			}
		}
	}

	AsyncTask(ENamedThreads::GameThread, [this]() { Camera->UpdateUI(); });
}

void UCitizenManager::CreateTimer(FString Identifier, AActor* Caller, float Time, FTimerDelegate TimerDelegate, bool Repeat, bool OnGameThread)
{
	FScopeLock lock(&TimerLock);

	FTimerStruct timer;
	timer.CreateTimer(Identifier, Caller, Time, TimerDelegate, Repeat, OnGameThread);
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

	for (ACitizen* citizen : Citizen->GetLikedFamily(false))
		citizen->SetDecayHappiness(&citizen->FamilyDeathHappiness, -12);

	TArray<TEnumAsByte<EObjectTypeQuery>> objects;

	TArray<AActor*> ignore;
	ignore.Add(Camera->Grid);

	for (FResourceHISMStruct resourceStruct : Camera->Grid->MineralStruct)
		ignore.Add(resourceStruct.Resource);

	for (FResourceHISMStruct resourceStruct : Camera->Grid->FlowerStruct)
		ignore.Add(resourceStruct.Resource);

	TArray<AActor*> actors;

	int32 reach = Citizen->Range / 15.0f;
	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), Camera->GetTargetActorLocation(Citizen), Citizen->Range, objects, nullptr, ignore, actors);

	for (AActor* actor : actors) {
		if (!actor->IsA<ACitizen>())
			continue;

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
			if (!work->IsWorking(citizen, Hour))
				continue;

			FWorkHours workHours;
			workHours.Work = work;

			int32 index = citizen->HoursWorked.Find(workHours);

			if (index == INDEX_NONE) {
				workHours.Hours.Add(Hour);

				citizen->HoursWorked.Add(workHours);
			}
			else {
				if (citizen->HoursWorked[index].Hours.Contains(Hour))
					citizen->HoursWorked[index].Hours.Remove(Hour);

				citizen->HoursWorked[index].Hours.Add(Hour);
			}
		}

		work->CheckWorkStatus(Hour);
	}
}

ERaidPolicy UCitizenManager::GetRaidPolicyStatus()
{
	ERaidPolicy policy = ERaidPolicy::Default;

	if (!Enemies.IsEmpty())
		policy = ERaidPolicy(GetLawValue("Raid Policy"));

	return policy;
}

//
// Citizen
//
void UCitizenManager::CheckUpkeepCosts()
{
	for (ACitizen* citizen : Citizens) {
		int32 amount = 0;

		for (FWorkHours workHours : citizen->HoursWorked)
			amount += FMath::RoundHalfFromZero(workHours.Work->WagePerHour * workHours.Hours.Num());

		citizen->Balance += amount;
		citizen->Camera->ResourceManager->TakeUniversalResource(Money, amount, -100000);

		if (IsValid(citizen->Building.House))
			citizen->Building.House->GetRent(citizen);
	}
}

void UCitizenManager::CheckCitizenStatus(int32 Hour)
{
	for (ACitizen* citizen : Citizens) {
		if (citizen->HoursSleptToday.Contains(Hour))
			citizen->HoursSleptToday.Remove(Hour);

		if (citizen->bSleep)
			citizen->HoursSleptToday.Add(Hour);

		if (citizen->HoursSleptToday.Num() < citizen->IdealHoursSlept && !citizen->bSleep && (!IsValid(citizen->Building.Employment) || !citizen->Building.Employment->IsWorking(citizen)) && IsValid(citizen->Building.House) && citizen->Building.BuildingAt == citizen->Building.House)
			citizen->bSleep = true;
		else if (citizen->bSleep)
			citizen->bSleep = false;

		citizen->DecayHappiness();
		citizen->IncrementHoursTogetherWithPartner();
	}

	CheckForWeddings(Hour);
}

void UCitizenManager::CheckForWeddings(int32 Hour)
{
	TArray<ACitizen*> citizens = Citizens;
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

		for (ABuilding* building : Buildings) {
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

		for (FEventStruct event : Events) {
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

		CreateEvent(EEventType::Marriage, nullptr, chosenChurch, "", calendar.Days[calendar.Index], weddingHours, false, whitelist);
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

void UCitizenManager::StartConversation(ACitizen* Citizen1, ACitizen* Citizen2, bool bInterrogation)
{
	Citizen1->bConversing = true;
	Citizen2->bConversing = true;
	
	AsyncTask(ENamedThreads::GameThread, [this, Citizen1, Citizen2, bInterrogation]() {
		Citizen1->MovementComponent->Transform.SetRotation((Citizen2->MovementComponent->Transform.GetLocation() - Citizen1->MovementComponent->Transform.GetLocation()).Rotation().Quaternion());
		Citizen2->MovementComponent->Transform.SetRotation((Citizen1->MovementComponent->Transform.GetLocation() - Citizen2->MovementComponent->Transform.GetLocation()).Rotation().Quaternion());

		Citizen1->AIController->StopMovement();
		Citizen2->AIController->StopMovement();

		if (bInterrogation)
			CreateTimer("Interrogate", Citizen1, 6.0f, FTimerDelegate::CreateUObject(this, &UCitizenManager::InterrogateWitnesses, Citizen1, Citizen2), false);
		else
			CreateTimer("Interact", Citizen1, 6.0f, FTimerDelegate::CreateUObject(this, &UCitizenManager::Interact, Citizen1, Citizen2), false);

		int32 conversationIndex = Camera->Grid->Stream.RandRange(0, Citizen1->Conversations.Num() - 1);

		TArray<USoundBase*> keys;
		Citizen1->Conversations.GenerateKeyArray(keys);

		TArray<USoundBase*> values;
		Citizen1->Conversations.GenerateValueArray(values);

		Camera->PlayAmbientSound(Citizen1->AmbientAudioComponent, keys[conversationIndex], Citizen1->VoicePitch);
		Camera->PlayAmbientSound(Citizen2->AmbientAudioComponent, values[conversationIndex], Citizen2->VoicePitch);
	});
}

void UCitizenManager::Interact(ACitizen* Citizen1, ACitizen* Citizen2)
{
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

		for (ABuilding* building : Buildings) {
			if (!building->IsA(PoliceStationClass) || building->GetCitizensAtBuilding().IsEmpty())
				continue;

			float dist = FVector::Dist(midPoint, building->GetActorLocation());

			if (distance > dist)
				distance = dist;
		}

		TArray<TEnumAsByte<EObjectTypeQuery>> objects;

		TArray<AActor*> ignore;
		ignore.Add(Camera->Grid);

		for (FResourceHISMStruct resourceStruct : Camera->Grid->MineralStruct)
			ignore.Add(resourceStruct.Resource);

		for (FResourceHISMStruct resourceStruct : Camera->Grid->FlowerStruct)
			ignore.Add(resourceStruct.Resource);

		ignore.Append(Buildings);

		TArray<AActor*> actors;

		ACitizen* aggressor = Citizen1;

		if (citizen1Aggressiveness < citizen2Aggressiveness)
			aggressor = Citizen2;

		UKismetSystemLibrary::SphereOverlapActors(GetWorld(), Camera->GetTargetActorLocation(aggressor), aggressor->Range, objects, nullptr, ignore, actors);

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

bool UCitizenManager::IsInAPoliceReport(ACitizen* Citizen)
{
	for (FPoliceReport& report : PoliceReports) {
		if (!report.Witnesses.Contains(Citizen))
			continue;

		if (report.RespondingOfficer != Citizen) {
			if (Citizen->Building.Employment->IsA(PoliceStationClass))
				continue;

			report.RespondingOfficer = nullptr;

			return false;
		}

		if (report.Witnesses.Contains(Citizen) && (report.Impartial.Contains(Citizen) || report.AcussesTeam1.Contains(Citizen) || report.AcussesTeam2.Contains(Citizen)))
			return false;

		return true;
	}

	return false;
}

void UCitizenManager::ChangeReportToMurder(ACitizen* Citizen)
{
	for (FPoliceReport& report : PoliceReports) {
		if (!report.Team1.GetTeam().Contains(Citizen) && !report.Team2.GetTeam().Contains(Citizen))
			continue;

		report.Type = EReportType::Murder;
	}
}

void UCitizenManager::GetCloserToFight(ACitizen* Citizen, ACitizen* Target, FVector MidPoint)
{
	while (true) {
		FNavLocation location;

		UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
		nav->GetRandomReachablePointInRadius(MidPoint, 600.0f, location);

		if (FVector::Dist(location.Location, MidPoint) < 400.0f)
			continue;

		Citizen->AIController->AIMoveTo(Target, location.Location);

		return;
	}
}

void UCitizenManager::StopFighting(ACitizen* Citizen)
{
	for (FPoliceReport report : PoliceReports) {
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

void UCitizenManager::InterrogateWitnesses(ACitizen* Officer, ACitizen* Citizen)
{
	for (FPoliceReport& report : PoliceReports) {
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
	for (FPoliceReport report : PoliceReports) {
		if (report.RespondingOfficer != Officer)
			continue;

		TArray<ACitizen*> wanted;

		if (report.AcussesTeam1.Num() >= report.AcussesTeam2.Num())
			wanted = report.Team1.GetTeam();
		else
			wanted = report.Team2.GetTeam();

		ACitizen* target = nullptr;

		for (ACitizen* criminal : wanted) {
			if (Arrested.Contains(criminal))
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
			PoliceReports.Remove(report);

			ToggleOfficerLights(Officer, 0.0f);

			Officer->AIController->DefaultAction();
		}
	}
}

void UCitizenManager::Arrest(ACitizen* Officer, ACitizen* Citizen)
{
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

	for (FPoliceReport report : PoliceReports) {
		if (report.RespondingOfficer != Officer)
			continue;

		FString law = "Fighting Length";

		if (report.Type == EReportType::Murder)
			law = "Murder Length";
		else if (report.Type == EReportType::Vandalism)
			law = "Vandalism Length";
		else if (report.Type == EReportType::Protest)
			law = "Protest Length";

		if (GetLawValue(law) == 0) {
			PoliceReports.Remove(report);

			Officer->AIController->DefaultAction();
			Citizen->AIController->DefaultAction();

			return;
		}
	}

	UNiagaraFunctionLibrary::SpawnSystemAttached(ArrestSystem, Citizen->GetRootComponent(), "", FVector::Zero(), FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true, false);

	CreateTimer("Arrest", Citizen, 2.0f, FTimerDelegate::CreateUObject(this, &UCitizenManager::SetInNearestJail, Officer, Citizen), false);
}

void UCitizenManager::SetInNearestJail(ACitizen* Officer, ACitizen* Citizen)
{
	ABuilding* target = nullptr;

	for (ABuilding* building : Buildings) {
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
	Citizen->AIController->Idle(Citizen);

	FPartyStruct* party = GetMembersParty(Citizen);

	if (party != nullptr) {
		if (party->Leader == Citizen)
			SelectNewLeader(party->Party);

		party->Members.Remove(Citizen);
	}

	if (Representatives.Contains(Citizen))
		Representatives.Remove(Citizen);

	if (!IsValid(Officer))
		return;

	for (FPoliceReport report : PoliceReports) {
		if (report.RespondingOfficer != Officer)
			continue;

		FString law = "Fighting Length";

		if (report.Type == EReportType::Murder)
			law = "Murder Length";
		else if (report.Type == EReportType::Vandalism)
			law = "Vandalism Length";
		else if (report.Type == EReportType::Protest)
			law = "Protest Length";

		Arrested.Add(Citizen, GetLawValue(law));

		break;
	}

	GotoClosestWantedMan(Officer);
}

void UCitizenManager::ItterateThroughSentences()
{
	TArray<ACitizen*> served;

	for (auto& element : Arrested) {
		element.Value--;

		if (element.Value == 0)
			served.Add(element.Key);
	}

	Camera->ResourceManager->AddUniversalResource(Money, Arrested.Num());

	for (ACitizen* citizen : served) {
		Arrested.Remove(citizen);

		citizen->MovementComponent->Transform.SetLocation(citizen->Building.BuildingAt->BuildingMesh->GetSocketLocation("Entrance"));
	}
}

void UCitizenManager::ToggleOfficerLights(ACitizen* Officer, float Value)
{
	Officer->HatMesh->SetCustomPrimitiveDataFloat(1, Value);
}

void UCitizenManager::CeaseAllInternalFighting()
{
	for (ACitizen* citizen : Citizens) {
		if (citizen->AttackComponent->OverlappingEnemies.IsEmpty()) {
			citizen->AIController->DefaultAction();

			continue;
		}

		for (int32 i = citizen->AttackComponent->OverlappingEnemies.Num() - 1; i > -1; i--) {
			AActor* actor = citizen->AttackComponent->OverlappingEnemies[i];

			if (Citizens.Contains(actor))
				citizen->AttackComponent->OverlappingEnemies.RemoveAt(i);
		}
	}
}

int32 UCitizenManager::GetPoliceReportIndex(ACitizen* Citizen)
{
	for (int32 i = 0; i < PoliceReports.Num(); i++) {
		if (!PoliceReports[i].Contains(Citizen))
			continue;

		return i;
	}

	return INDEX_NONE;
}

//
// Disease
//
void UCitizenManager::StartDiseaseTimer()
{
	int32 timeToCompleteDay = 360 / (24 * Camera->Grid->AtmosphereComponent->Speed);

	CreateTimer("Disease", GetOwner(), Camera->Grid->Stream.RandRange(timeToCompleteDay / 2, timeToCompleteDay * 3), FTimerDelegate::CreateUObject(this, &UCitizenManager::SpawnDisease), false);
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
	AsyncTask(ENamedThreads::GameThread, [this, Citizen]() {
		if (Infected.IsEmpty())
			Camera->Grid->AIVisualiser->DiseaseNiagaraComponent->Activate();

		Infected.Add(Citizen);

		UpdateHealthText(Citizen);

		GetClosestHealer(Citizen);
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

	Infected.Remove(Citizen);
	Injured.Remove(Citizen);
	Healing.Remove(Citizen);

	if (Infected.IsEmpty())
		Camera->Grid->AIVisualiser->DiseaseNiagaraComponent->Deactivate();

	Camera->NotifyLog("Good", Citizen->BioStruct.Name + " has been healed", Camera->ConquestManager->GetCitizenFaction(Citizen).Name);

	UpdateHealthText(Citizen);
}

void UCitizenManager::UpdateHealthText(ACitizen* Citizen)
{
	AsyncTask(ENamedThreads::GameThread, [this, Citizen]() {
		if (Camera->WidgetComponent->IsAttachedTo(Citizen->GetRootComponent()))
			Camera->UpdateHealthIssues();
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

			for (ACitizen* h : clinic->GetCitizensAtBuilding()) {
				if (!h->AIController->CanMoveTo(Camera->GetTargetActorLocation(Citizen)) || h->AIController->MoveRequest.GetGoalActor() != nullptr)
					continue;

				if (healer == nullptr) {
					healer = h;

					continue;
				}

				double magnitude = h->AIController->GetClosestActor(50.0f, Camera->GetTargetActorLocation(Citizen), Camera->GetTargetActorLocation(healer), Camera->GetTargetActorLocation(h));

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

			double magnitude = Healer->AIController->GetClosestActor(50.0f, Camera->GetTargetActorLocation(Healer), Camera->GetTargetActorLocation(Citizen), Camera->GetTargetActorLocation(citizen));

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
void UCitizenManager::CreateEvent(EEventType Type, TSubclassOf<class ABuilding> Building, class ABuilding* Venue, FString Period, int32 Day, TArray<int32> Hours, bool bRecurring, TArray<ACitizen*> Whitelist, bool bFireFestival)
{
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
		ABuilding* building = Buildings[Camera->Grid->Stream.RandRange(0, Buildings.Num() - 1)];

		UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
		const ANavigationData* navData = nav->GetDefaultNavDataInstance();

		FNavLocation location;
		nav->GetRandomPointInNavigableRadius(building->GetActorLocation(), 400, location);

		event.Location = location;
	}

	Events.Add(event);
}

void UCitizenManager::ExecuteEvent(FString Period, int32 Day, int32 Hour)
{
	for (FEventStruct& event : Events) {
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
			StartEvent(event, Hour);
		else
			EndEvent(event, Hour);
	}
}

bool UCitizenManager::IsAttendingEvent(ACitizen* Citizen)
{
	for (FEventStruct event : OngoingEvents())
		if (event.Attendees.Contains(Citizen))
			return true;

	return false;
}

void UCitizenManager::RemoveFromEvent(ACitizen* Citizen)
{
	for (FEventStruct event : OngoingEvents()) {
		if (!event.Attendees.Contains(Citizen))
			continue;

		int32 index = Events.Find(event);

		Events[index].Attendees.Remove(Citizen);

		return;
	}
}

TArray<FEventStruct> UCitizenManager::OngoingEvents()
{
	TArray<FEventStruct> events;
	
	for (FEventStruct event : Events) {
		UAtmosphereComponent* atmosphere = Camera->Grid->AtmosphereComponent;

		if (!event.bStarted)
			continue;

		events.Add(event);
	}

	return events;
}

void UCitizenManager::GotoEvent(ACitizen* Citizen, FEventStruct Event)
{
	if (IsAttendingEvent(Citizen) || Citizen->bSleep || (Event.Type != EEventType::Protest && IsValid(Citizen->Building.Employment) && !Citizen->Building.Employment->bCanAttendEvents && Citizen->Building.Employment->IsWorking(Citizen)) || (Event.Type == EEventType::Mass && Cast<ABooster>(Event.Building->GetDefaultObject())->DoesPromoteFavouringValues(Citizen) && Citizen->BioStruct.Age >= 18))
		return;

	int32 index = Events.Find(Event);

	if (Event.Type == EEventType::Holliday || Event.Type == EEventType::Festival) {
		Citizen->SetHolliday(true);
	}
	else if (Event.Type == EEventType::Protest) {
		if (Citizen->GetHappiness() >= 35)
			return;

		UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
		const ANavigationData* navData = nav->GetDefaultNavDataInstance();

		UNavigationPath* path = nav->FindPathToLocationSynchronously(GetWorld(), Camera->GetTargetActorLocation(Citizen), Event.Location, Citizen, Citizen->NavQueryFilter);

		Citizen->MovementComponent->SetPoints(path->PathPoints);

		return;
	}
	else if (Event.Type == EEventType::Mass) {
		if (!IsValid(Citizen->Building.Employment) || !IsValid(Citizen->Building.BuildingAt) || Citizen->Building.Employment->IsA(Event.Building)) {
			Citizen->AIController->AIMoveTo(Citizen->Building.Employment);

			Events[index].Attendees.Add(Citizen);

			return;
		}

		ABooster* church = Cast<ABooster>(Event.Building->GetDefaultObject());

		TArray<FString> faiths;
		church->BuildingsToBoost.GenerateValueArray(faiths);

		bool validReligion = (Citizen->Spirituality.Faith == faiths[0]);

		if (Citizen->BioStruct.Age < 18 && (Citizen->BioStruct.Mother->IsValidLowLevelFast() && faiths[0] != Citizen->BioStruct.Mother->Spirituality.Faith || Citizen->BioStruct.Father->IsValidLowLevelFast() && faiths[0] != Citizen->BioStruct.Father->Spirituality.Faith))
			validReligion = true;

		if (!validReligion)
			return;
	}

	AsyncTask(ENamedThreads::GameThread, [this, Citizen, Event, index]() {
		ABuilding* chosenBuilding = nullptr;
		ACitizen* chosenOccupant = nullptr;

		TArray<AActor*> actors;

		if (IsValid(Event.Building))
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), Event.Building, actors);
		else if (IsValid(Event.Venue))
			actors.Add(Event.Venue);

		for (AActor* actor : actors) {
			ABuilding* building = Cast<ABuilding>(actor);
			ACitizen* occupant = nullptr;

			if (!Citizen->AIController->CanMoveTo(building->GetActorLocation()) || (Event.Type == EEventType::Mass && building->GetOccupied().IsEmpty()) || (Event.Type == EEventType::Festival && !Cast<AFestival>(building)->bCanHostFestival))
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
			Events[index].Attendees.Add(Citizen);

			chosenBuilding->AddVisitor(chosenOccupant, Citizen);

			Citizen->AIController->AIMoveTo(chosenBuilding);
		}
	});
}

void UCitizenManager::StartEvent(FEventStruct Event, int32 Hour)
{
	int32 index = Events.Find(Event);

	Events[index].bStarted = true;
	
	if (Event.Type != EEventType::Protest) {
		for (ABuilding* building : Buildings) {
			if (building->IsA<AWork>())
				Cast<AWork>(building)->CheckWorkStatus(Hour);
			else if (building->IsA<AFestival>() && Event.Type == EEventType::Festival)
				Cast<AFestival>(building)->StartFestival(Event.bFireFestival);
		}
	}

	TArray<ACitizen*> citizens = Citizens;
	if (!Event.Whitelist.IsEmpty())
		citizens = Event.Whitelist;
	
	for (ACitizen* citizen : citizens)
		GotoEvent(citizen, Event);

	if (Event.Type == EEventType::Protest && GetLawValue("Protest Length") > 0) {
		FPoliceReport report;
		report.Type = EReportType::Protest;
		report.Team1.Instigator = Events[index].Attendees[0];
		report.Team1.Assistors = Events[index].Attendees;
		report.Team1.Assistors.RemoveAt(0);

		PoliceReports.Add(report);
	}

	Camera->DisplayEvent("Event", EnumToString<EEventType>(Event.Type));
}

void UCitizenManager::EndEvent(FEventStruct Event, int32 Hour)
{
	Event.Attendees.Empty();
	Event.bStarted = false;

	if (Event.Type == EEventType::Holliday || Event.Type == EEventType::Festival) {
		for (ABuilding* building : Buildings) {
			if (building->IsA<AWork>())
				Cast<AWork>(building)->CheckWorkStatus(Hour);
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
			ABooster* church = Cast<ABooster>(Event.Building->GetDefaultObject());

			TArray<FString> faiths;
			church->BuildingsToBoost.GenerateValueArray(faiths);

			if (faiths[0] != citizen->Spirituality.Faith)
				continue;

			if (citizen->bWorshipping)
				citizen->SetAttendStatus(EAttendStatus::Attended, true);
			else
				citizen->SetAttendStatus(EAttendStatus::Missed, true);
		}

		if (!IsValid(Event.Building))
			citizen->AIController->DefaultAction();
	}

	int32 index = Events.Find(Event);

	if (!Event.bRecurring && Event.Hours.IsEmpty())
		Events.RemoveAt(index);
}

bool UCitizenManager::UpcomingProtest()
{
	for (FEventStruct event : Events) {
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

	for (FPartyStruct &party : Parties) {
		TEnumAsByte<ESway>* sway = party.Members.Find(Citizen);

		if (sway == nullptr)
			continue;

		partyStruct = &party;

		break;
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

void UCitizenManager::SelectNewLeader(FString Party)
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

	auto value = Async(EAsyncExecution::TaskGraph, [this, candidates]() { return Camera->Grid->Stream.RandRange(0, candidates.Num() - 1); });

	ACitizen* chosen = candidates[value.Get()];

	party->Leader = chosen;

	chosen->bHasBeenLeader = true;
	party->Members.Emplace(chosen, ESway::Radical);
}

void UCitizenManager::StartElectionTimer()
{
	RemoveTimer("Election", GetOwner());
	
	int32 timeToCompleteDay = 360 / (24 * Camera->Grid->AtmosphereComponent->Speed);

	CreateTimer("Election", GetOwner(), timeToCompleteDay * GetLawValue("Election Timer"), FTimerDelegate::CreateUObject(this, &UCitizenManager::Election), false);
}

void UCitizenManager::Election()
{
	Representatives.Empty();

	TMap<FString, TArray<ACitizen*>> tally;

	int32 representativeType = GetLawValue("Representative Type");

	for (FPartyStruct party : Parties) {
		TArray<ACitizen*> citizens;

		for (TPair<ACitizen*, TEnumAsByte<ESway>> pair : party.Members) {
			if ((representativeType == 1 && pair.Key->Building.Employment == nullptr) || (representativeType == 2 && pair.Key->Balance < 15))
				continue;

			citizens.Add(pair.Key);
		}

		tally.Add(party.Party, citizens);
	}

	for (TPair<FString, TArray<ACitizen*>>& pair : tally) {
		int32 number = FMath::RoundHalfFromZero(pair.Value.Num() / (float)Citizens.Num() * 100.0f / GetLawValue("Representatives"));

		if (number == 0 || Representatives.Num() == GetLawValue("Representatives"))
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

			auto value = Async(EAsyncExecution::TaskGraph, [this, pair]() { return Camera->Grid->Stream.RandRange(0, pair.Value.Num() - 1); });

			ACitizen* citizen = pair.Value[value.Get()];

			Representatives.Add(citizen);

			pair.Value.Remove(citizen);

			if (Representatives.Num() == GetLawValue("Representatives"))
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

	bool bPass = Camera->ResourceManager->TakeUniversalResource(Money, bribe, 0);

	if (!bPass) {
		Camera->ShowWarning("Cannot afford");

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

		Camera->ShowWarning(string + " seconds");

		return;
	}

	ProposedBills.Add(Bill);

	int32 timeToCompleteDay = 360 / (24 * Camera->Grid->AtmosphereComponent->Speed);

	Laws[index].Cooldown = timeToCompleteDay;

	Camera->DisplayNewBill();

	if (ProposedBills.Num() > 1)
		return;

	SetupBill();
}

void UCitizenManager::SetElectionBillLeans(FLawStruct* Bill)
{
	if (Bill->BillType != "Election")
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
		int32 bribe = Async(EAsyncExecution::TaskGraph, [this]() { return Camera->Grid->Stream.RandRange(2, 20); }).Get();

		if (Votes.For.Contains(citizen) || Votes.Against.Contains(citizen))
			bribe *= 4;

		bribe *= (uint8)*GetMembersParty(citizen)->Members.Find(citizen);

		BribeValue.Add(bribe);
	}

	CreateTimer("Bill", GetOwner(), 60, FTimerDelegate::CreateUObject(this, &UCitizenManager::MotionBill, ProposedBills[0]), false);
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

		if (Laws[index].BillType == "Abolish") {
			ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());

			CreateTimer("Abolish", GetOwner(), 6, FTimerDelegate::CreateUObject(gamemode->Broch->HealthComponent, &UHealthComponent::TakeHealth, 1000, GetOwner()), false);
		}
		else if (Laws[index].BillType == "Election") {
			Election();
		}
		else {
			Laws[index].Value = Bill.Value;

			if (Laws[index].BillType.Contains("Age")) {
				for (ACitizen* citizen : Citizens) {
					if (IsValid(citizen->Building.Employment) && !citizen->CanWork(citizen->Building.Employment))
						citizen->Building.Employment->RemoveCitizen(citizen);

					if (IsValid(citizen->Building.School) && (citizen->BioStruct.Age >= GetLawValue("Work Age") || citizen->BioStruct.Age < GetLawValue("Education Age")))
						citizen->Building.School->RemoveVisitor(citizen->Building.School->GetOccupant(citizen), citizen);

					if (citizen->BioStruct.Age >= Laws[index].Value)
						continue;

					FPartyStruct* party = GetMembersParty(citizen);

					if (party == nullptr)
						continue;

					party->Members.Remove(citizen);
				}
			}
		}

		if (Laws[index].BillType == "Representatives" && Camera->ParliamentUIInstance->IsInViewport())
			Camera->RefreshRepresentatives();

		bPassed = true;
	}

	Camera->LawPassed(bPassed, Votes.For.Num(), Votes.Against.Num());

	Camera->LawPassedUIInstance->AddToViewport();

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

int32 UCitizenManager::GetLawValue(FString BillType)
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
		if (citizen->BioStruct.Age >= GetLawValue("Pension Age"))
			citizen->Balance += GetLawValue("Pension");
}

//
// Rebel
//
void UCitizenManager::Overthrow()
{
	CooldownTimer = 1500;

	CeaseAllInternalFighting();

	for (ACitizen* citizen : Citizens) {
		if (GetMembersParty(citizen) == nullptr || GetMembersParty(citizen)->Party != "Shell Breakers")
			return;

		if (Representatives.Contains(citizen))
			Representatives.Remove(citizen);

		SetupRebel(citizen);
	}
}

void UCitizenManager::SetupRebel(ACitizen* Citizen)
{
	Citizen->Energy = 100;
	Citizen->Hunger = 100;

	UAIVisualiser* aiVisualiser = Camera->Grid->AIVisualiser;
	aiVisualiser->RemoveInstance(aiVisualiser->HISMCitizen, Citizens.Find(Citizen));
	Citizens.Remove(Citizen);

	Rebels.Add(Citizen);
	aiVisualiser->AddInstance(Citizen, aiVisualiser->HISMRebel, Citizen->MovementComponent->Transform);

	Citizen->MoveToBroch();
}

bool UCitizenManager::IsRebellion()
{
	return !Rebels.IsEmpty();
}

//
// Genetics
//
void UCitizenManager::Pray()
{
	bool bPass = Camera->ResourceManager->TakeUniversalResource(Money, GetPrayCost(), 0);

	if (!bPass) {
		Camera->ShowWarning("Cannot afford");

		return;
	}

	IncrementPray("Good", 1);

	int32 timeToCompleteDay = 360 / (24 * Camera->Grid->AtmosphereComponent->Speed);

	CreateTimer("Pray", GetOwner(), timeToCompleteDay, FTimerDelegate::CreateUObject(this, &UCitizenManager::IncrementPray, FString("Good"), -1), false);
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
		Camera->ShowWarning("Cannot afford");

		return;
	}

	int32 index = Camera->Grid->Stream.RandRange(0, Citizens.Num() - 1);
	ACitizen* citizen = Citizens[index];

	citizen->AIController->StopMovement();
	citizen->MovementComponent->SetMaxSpeed(0.0f);

	UNiagaraComponent* component = UNiagaraFunctionLibrary::SpawnSystemAttached(SacrificeSystem, citizen->GetRootComponent(), NAME_None, FVector::Zero(), FRotator(0.0f), EAttachLocation::SnapToTarget, true);

	CreateTimer("Sacrifice", GetOwner(), 4.0f, FTimerDelegate::CreateUObject(citizen->HealthComponent, &UHealthComponent::TakeHealth, 1000, GetOwner()), false);

	IncrementPray("Bad", 1);

	int32 timeToCompleteDay = 360 / (24 * Camera->Grid->AtmosphereComponent->Speed);

	CreateTimer("Pray", GetOwner(), timeToCompleteDay, FTimerDelegate::CreateUObject(this, &UCitizenManager::IncrementPray, FString("Bad"), -1), false);
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